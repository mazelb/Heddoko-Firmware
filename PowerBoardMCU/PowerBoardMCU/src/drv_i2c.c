/*
 * drv_i2c.c
 *
 * Created: 2/12/2016 10:21:36 AM
 *  Author: Hriday Mehta
 */ 
/**
 * @file  drv_i2c.c
 * @brief Low level i2c driver
 * @author Heddoko
 * @note This driver can be used on TWI0 and TWI1
 *
 * @copy Heddoko (2015)
 */
#include "drv_i2c.h"
#include "common.h" 
#include "twi.h"
 
#define TWI0_DATA_GPIO   PIO_PA3_IDX
#define TWI0_DATA_FLAGS  (PIO_PERIPH_A | PIO_DEFAULT)
#define TWI0_CLK_GPIO    PIO_PA4_IDX
#define TWI0_CLK_FLAGS   (PIO_PERIPH_A | PIO_DEFAULT)

#define TWI1_DATA_GPIO   PIO_PB4_IDX
#define TWI1_DATA_FLAGS  (PIO_PERIPH_B | PIO_DEFAULT)
#define TWI1_CLK_GPIO    PIO_PB5_IDX
#define TWI1_CLK_FLAGS   (PIO_PERIPH_B | PIO_DEFAULT)

extern unsigned long sgSysTickCount;

static uint32_t twi_mk_addr(const uint8_t *addr, int len);
uint32_t twi_write(Twi *p_twi, twi_packet_t *p_packet);
uint32_t twi_read(Twi *p_twi, twi_packet_t *p_packet);

typedef struct
{
	twi_fifo_typedef rx_fifo; 
	uint8_t isinit; 
	uint8_t readReq;
	uint32_t readReqDataLength;
	uint8_t twi_rx_fifo_not_empty_flag; // this flag is automatically set and cleared by the software buffer
	uint8_t twi_rx_fifo_full_flag;      // this flag is automatically set and cleared by the software buffer
	uint8_t twi_rx_fifo_ovf_flag;       // this flag is not automatically cleared by the software buffer	
	uint32_t twi_rx_fifo_dropped_bytes; 
	twi_fifo_typedef tx_fifo; 
	uint8_t twi_tx_fifo_not_empty_flag; // this flag is automatically set and cleared by the software buffer
	uint8_t twi_tx_fifo_full_flag;      // this flag is automatically set and cleared by the software buffer
	uint8_t twi_tx_fifo_ovf_flag;       // this flag is not automatically cleared by the software buffer	
	uint32_t twi_tx_fifo_dropped_bytes; 
		
}drv_i2c_memory_buf_t;

//global variables
volatile drv_i2c_memory_buf_t twiMemBuf[2]; //2 TWIs, 2 buffers
 
int drv_i2c_init(drv_twi_config_t* twi_config)
{
	int status = STATUS_PASS;
	
	if (twi_config->p_i2c == TWI0)
	{
		twi_config->mem_index = 0;
 		gpio_configure_pin(TWI0_DATA_GPIO, TWI0_DATA_FLAGS);	//TODO: use drv_gpio to do this.
 		gpio_configure_pin(TWI0_CLK_GPIO, TWI0_CLK_FLAGS);
	}
	else if (twi_config->p_i2c == TWI1)
	{
		twi_config->mem_index = 1;
 		gpio_configure_pin(TWI1_DATA_GPIO, TWI1_DATA_FLAGS);
 		gpio_configure_pin(TWI1_CLK_GPIO, TWI1_CLK_FLAGS);
	}
	else
	{
		return STATUS_FAIL;	//return error
	}
	
	//update the master clock value to current cpu_hz before initializing
	twi_config->twi_options.master_clk = sysclk_get_cpu_hz();
	//twi_config->twi_options.master_clk = 120000000;
	
	#ifdef USE_FREE_RTOS_DRV
	//initialize TWI in master mode
	twi_config->freertos_twi = freertos_twi_master_init(twi_config->p_i2c, &twi_config->freeRtos_options);
	if (twi_config->freertos_twi == NULL)
	{
		return STATUS_FAIL;
	}
	/* Configure the TWI bus parameters.  Do this after calling freertos_twi_master_init(). */
	twi_set_speed(twi_config->p_i2c, twi_config->twi_options.speed, twi_config->twi_options.master_clk);
	
	#else
	pmc_enable_periph_clk(twi_config->p_i2c);	//enable clock for I2C
	status = twi_master_init(twi_config->p_i2c, &twi_config->twi_options);	//can also use twi_master_setup
	if (status != TWI_SUCCESS)
	{
		puts("Failed to initialize as master.\r\n");
		return STATUS_FAIL;	//return error
	}
	#endif
	
	return STATUS_PASS;
}

int drv_i2c_write(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t data)
{
	status_t status = STATUS_FAIL;
	//Write one byte to desired register
	uint8_t dataPacket[2] = {0};
	twi_packet_t packet;
	dataPacket[0] = reg;
	#ifdef ENABLE_TWO_BYTE_WRITE
	dataPacket[1] = data;
	packet.length = 2;
	#else
	packet.length = 1;
	#endif
	packet.buffer = &dataPacket[0];
	packet.chip = (uint32_t) slave_twi_config->address;
	packet.addr[0] = 0x00;	//no internal register should be assigned
	packet.addr[1] = 0x00;
	packet.addr[2] = 0x00;
	packet.addr_length = 0;	
	
	#ifdef USE_FREE_RTOS_DRV
	if(freertos_twi_write_packet(slave_twi_config->drv_twi_options->freertos_twi, &packet, MAX_TWI_BLOCK_TIME) != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	#else
	//status = twi_master_write(slave_twi_config->drv_twi_options->p_i2c, &packet);
	//if (status != TWI_SUCCESS)
	//{
		//return STATUS_FAIL;
	//}
	status = twi_write(slave_twi_config->drv_twi_options->p_i2c, &packet);
	if (status != TWI_SUCCESS)
	{
		return STATUS_FAIL;
	}
	#endif
	
	return STATUS_PASS;
}

int drv_i2c_read(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t* data, uint8_t length)
{
	uint8_t status = STATUS_FAIL;
	//Write the address of register to read from
	uint8_t dataPacket[2] = {0};
	twi_packet_t packet;
	dataPacket[0] = reg;
	packet.buffer = dataPacket;
	packet.length = 1;
	packet.chip = (uint32_t) slave_twi_config->address;
	packet.addr[0] = 0x00;
	packet.addr[1] = 0x00;
	packet.addr[2] = 0x00;
	packet.addr_length = 0;
	//delay_ms(1);
	//#ifdef USE_FREE_RTOS_DRV
	//if(freertos_twi_write_packet(slave_twi_config->drv_twi_options->freertos_twi, &packet, MAX_TWI_BLOCK_TIME) != STATUS_OK)
	//{
		//return STATUS_FAIL;
	//}
	//
	//#else
	//status = twi_master_write(slave_twi_config->drv_twi_options->p_i2c, &packet);
	//if (status != TWI_SUCCESS)
	//{
		//return STATUS_FAIL;
	//}
	//status = twi_write(slave_twi_config->drv_twi_options->p_i2c, &packet);
	//if (status != TWI_SUCCESS)
	//{
		//return STATUS_FAIL;
	//}
	//#endif
	
	//delay_ms(1);
	//initialize the read sequence
	packet.buffer = data;
	packet.length = length;
	packet.addr[0] = reg;
	packet.addr[1] = 0x00;
	packet.addr[2] = 0x00;
	packet.addr_length = 1;
	#ifdef USE_FREE_RTOS_DRV
	if (freertos_twi_read_packet(slave_twi_config->drv_twi_options->freertos_twi, &packet, MAX_TWI_BLOCK_TIME) != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	#else
	#ifdef ENABLE_MULTI_BYTE_READ 
	status = twi_read(slave_twi_config->drv_twi_options->p_i2c, &packet);
	if (status != TWI_SUCCESS)
	{
		return STATUS_FAIL;
	}
	#else
	for (;length>0;length--)
	{
		packet.buffer = data++;
		packet.length = 1;
		packet.addr[0] = 0x00;
		packet.addr[1] = 0x00;
		packet.addr[2] = 0x00;
		status = twi_read(slave_twi_config->drv_twi_options->p_i2c, &packet);
		if (status != TWI_SUCCESS)
		{
			return STATUS_FAIL;
		}
	}
	#endif
	#endif
	return STATUS_PASS;
}

int drv_i2c_enable_interrupt(drv_twi_config_t* twi_config, uint32_t ul_sources)
{
	NVIC_DisableIRQ(twi_config->IRQtype);
	NVIC_ClearPendingIRQ(twi_config->IRQtype);
	NVIC_SetPriority(twi_config->IRQtype, 0);
	NVIC_EnableIRQ(twi_config->IRQtype);
	twi_enable_interrupt(twi_config->p_i2c, ul_sources);
	return STATUS_PASS;
}

int drv_i2c_disable_interrupt(drv_twi_config_t* twi_config, uint32_t ul_sources)
{
	NVIC_DisableIRQ(twi_config->IRQtype);
	NVIC_ClearPendingIRQ(twi_config->IRQtype);
	twi_disable_interrupt(twi_config->p_i2c, ul_sources);
	return STATUS_PASS;
}

static uint32_t twi_mk_addr(const uint8_t *addr, int len)
{
	uint32_t val;

	if (len == 0)
	return 0;

	val = addr[0];
	if (len > 1) {
		val <<= 8;
		val |= addr[1];
	}
	if (len > 2) {
		val <<= 8;
		val |= addr[2];
	}
	return val;
}

uint32_t twi_write(Twi *p_twi, twi_packet_t *p_packet)
{
	uint32_t status;
	uint32_t cnt = p_packet->length;
	uint8_t *buffer = p_packet->buffer;
	uint32_t timeout = TWI_TIMEOUT;

	/* Check argument */
	if (cnt == 0) {
		return TWI_INVALID_ARGUMENT;
	}

	/* Set write mode, slave address and 3 internal address byte lengths */
	p_twi->TWI_MMR = 0;
	p_twi->TWI_MMR = TWI_MMR_DADR(p_packet->chip) |
			((p_packet->addr_length << TWI_MMR_IADRSZ_Pos) &
			TWI_MMR_IADRSZ_Msk);

	/* Set internal address for remote chip */
	p_twi->TWI_IADR = 0;
	p_twi->TWI_IADR = twi_mk_addr(p_packet->addr, p_packet->addr_length);

	/* Send one bytes */
		status = p_twi->TWI_SR;
		if (status & TWI_SR_NACK) {
			return TWI_RECEIVE_NACK;
		}
		p_twi->TWI_THR = *buffer++;
		while (!(p_twi->TWI_SR & TWI_SR_TXRDY))
		{
			if (!(timeout--))
			{
				break;
			}
		}
		if (cnt>1)
		{
			status = p_twi->TWI_SR;
			if (status & TWI_SR_NACK) {
				return TWI_RECEIVE_NACK;
			}
			p_twi->TWI_THR = *buffer++;		
		}
		p_twi->TWI_CR = TWI_CR_STOP;
		
	timeout = TWI_TIMEOUT;
	while (1) {
		status = p_twi->TWI_SR;
		if (status & TWI_SR_NACK) {
			return TWI_RECEIVE_NACK;
		}

		if (status & TWI_SR_TXRDY) {
			break;
		}
		if (!(timeout--))
		{
			break;
		}
	}
	
	timeout = TWI_TIMEOUT;
	while (!(p_twi->TWI_SR & TWI_SR_TXCOMP)) {
		if (!(timeout--))
		{
			break;
		}
	}

	return TWI_SUCCESS;
}

uint32_t twi_read(Twi *p_twi, twi_packet_t *p_packet)
{
	uint32_t status;
	uint32_t cnt = p_packet->length;
	uint8_t *buffer = p_packet->buffer;
	uint8_t stop_sent = 0;
	uint32_t timeout = TWI_TIMEOUT;
	
	/* Check argument */
	if (cnt == 0) {
		return TWI_INVALID_ARGUMENT;
	}

	/* Set read mode, slave address and 3 internal address byte lengths */
	p_twi->TWI_MMR = 0;
	p_twi->TWI_MMR = TWI_MMR_MREAD | TWI_MMR_DADR(p_packet->chip) |
			((p_packet->addr_length << TWI_MMR_IADRSZ_Pos) &
			TWI_MMR_IADRSZ_Msk);

	/* Set internal address for remote chip */
	p_twi->TWI_IADR = 0;
	p_twi->TWI_IADR = twi_mk_addr(p_packet->addr, p_packet->addr_length);

	/* Send a START condition */
	if (cnt == 1) {
		p_twi->TWI_CR = TWI_CR_START | TWI_CR_STOP;
		stop_sent = 1;
	} else {
		p_twi->TWI_CR = TWI_CR_START;
		stop_sent = 0;
	}

	while (cnt > 0) {
		status = p_twi->TWI_SR;
		if (status & TWI_SR_NACK) {
			return TWI_RECEIVE_NACK;
		}

		if (!timeout--) {
			return TWI_ERROR_TIMEOUT;
		}
				
		/* Last byte ? */
		if (cnt == 1  && !stop_sent) {
			p_twi->TWI_CR = TWI_CR_STOP;
			stop_sent = 1;
		}

		if (!(status & TWI_SR_RXRDY)) {
			if (!(timeout--))
			{
				break;
			}
			continue;
		}
		timeout = TWI_TIMEOUT;
		*buffer++ = p_twi->TWI_RHR;

		cnt--;
	}

	timeout = TWI_TIMEOUT;
	while (!(p_twi->TWI_SR & TWI_SR_TXCOMP)) {
		if (!(timeout--))
		{
			break;
		}
	}

	p_twi->TWI_SR;

	return TWI_SUCCESS;
}