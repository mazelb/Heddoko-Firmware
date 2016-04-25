/*
 * drv_i2c.c
 *
 * Created: 2/15/2016 4:46:53 PM
 * Copyright Heddoko(TM) 2015, all rights reserved
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

#define TWI0_DATA_GPIO   PIO_PA3_IDX
#define TWI0_DATA_FLAGS  (PIO_PERIPH_A | PIO_DEFAULT)
#define TWI0_CLK_GPIO    PIO_PA4_IDX
#define TWI0_CLK_FLAGS   (PIO_PERIPH_A | PIO_DEFAULT)

#define TWI1_DATA_GPIO   PIO_PB4_IDX
#define TWI1_DATA_FLAGS  (PIO_PERIPH_B | PIO_DEFAULT)
#define TWI1_CLK_GPIO    PIO_PB5_IDX
#define TWI1_CLK_FLAGS   (PIO_PERIPH_B | PIO_DEFAULT)

extern unsigned long sgSysTickCount;
static freertos_twi_if freertos_twi;

int drv_i2c_init(drv_twi_config_t* twi_config)
{
	int status = STATUS_PASS;
	
	if (twi_config->p_i2c == TWI0)
	{
		gpio_configure_pin(TWI0_DATA_GPIO, TWI0_DATA_FLAGS);	//TODO: use drv_gpio to do this.
		gpio_configure_pin(TWI0_CLK_GPIO, TWI0_CLK_FLAGS);
		//sysclk_enable_peripheral_clock(ID_TWI0);
	}
	else if (twi_config->p_i2c == TWI1)
	{
		gpio_configure_pin(TWI1_DATA_GPIO, TWI1_DATA_FLAGS);
		gpio_configure_pin(TWI1_CLK_GPIO, TWI1_CLK_FLAGS);
		//sysclk_enable_peripheral_clock(ID_TWI1);
	}
	else
	{
		return STATUS_FAIL;	//return error
	}
	
	//update the master clock value to current cpu_hz before initializing
	twi_config->twi_options.master_clk = sysclk_get_cpu_hz();
	
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
	//Write one byte to desired register
	uint8_t status, dataPacket[2];
	twi_packet_t packet;
	dataPacket[0] = reg;
	dataPacket[1] = data;
	packet.buffer = &dataPacket[0];
	packet.length = 2;
	packet.chip = (uint32_t) slave_twi_config->address;
	packet.addr[0] = 0x00;	//no internal register should be assigned
	packet.addr_length = 0;
	if (gpio_pin_is_low(TWI0_DATA_GPIO))
	{
		pio_configure_pin(TWI0_CLK_GPIO, (PIO_TYPE_PIO_OUTPUT_0));
		for (int i =0; i<11; i++)
		{
			pio_set_pin_low(TWI0_CLK_GPIO);
			delay_us(2);
			pio_set_pin_high(TWI0_CLK_GPIO);
			delay_us(2);
		}
		pio_configure_pin(TWI0_CLK_GPIO, PIO_DEFAULT);
		drv_i2c_init(slave_twi_config->drv_twi_options);
		delay_us(10);
		if (gpio_pin_is_low(TWI0_DATA_GPIO))
		{
			return STATUS_FAIL;
		}
	}
	
	#ifdef USE_FREE_RTOS_DRV
	if(freertos_twi_write_packet(slave_twi_config->drv_twi_options->freertos_twi, &packet, MAX_TWI_BLOCK_TIME) != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	#else
	status = twi_master_write(slave_twi_config->drv_twi_options->p_i2c, &packet);
	if (status != TWI_SUCCESS)
	{
		return STATUS_FAIL;
	}
	#endif	
	
	return STATUS_PASS;
}

int drv_i2c_read(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t* data, uint8_t length)
{
	//Write the address of register to read from
	uint8_t status, dataPacket[2];
	twi_packet_t packet;
	dataPacket[0] = reg;
	packet.buffer = &dataPacket[0];
	packet.length = 1;
	packet.chip = (uint32_t) slave_twi_config->address;
	packet.addr[0] = 0x00;
	packet.addr_length = 0;
	if (gpio_pin_is_low(TWI0_DATA_GPIO))
	{
		pio_configure_pin(TWI0_CLK_GPIO, (PIO_TYPE_PIO_OUTPUT_0));
		for (int i =0; i<11; i++)
		{
			pio_set_pin_low(TWI0_CLK_GPIO);
			delay_us(2);
			pio_set_pin_high(TWI0_CLK_GPIO);
			delay_us(2);
		}
		pio_configure_pin(TWI0_CLK_GPIO, PIO_DEFAULT);
		drv_i2c_init(slave_twi_config->drv_twi_options);
		delay_us(10);
		if (gpio_pin_is_low(TWI0_DATA_GPIO))
		{
			return STATUS_FAIL;
		}
	}
	
	#ifdef USE_FREE_RTOS_DRV
	if(freertos_twi_write_packet(slave_twi_config->drv_twi_options->freertos_twi, &packet, MAX_TWI_BLOCK_TIME) != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	#else
	status = twi_master_write(slave_twi_config->drv_twi_options->p_i2c, &packet);
	if (status != TWI_SUCCESS)
	{
		return STATUS_FAIL;
	}
	#endif
	
	//initialize the read sequence
	packet.buffer = data;
	packet.length = length;
	if (gpio_pin_is_low(TWI0_DATA_GPIO))
	{
		pio_configure_pin(TWI0_CLK_GPIO, (PIO_TYPE_PIO_OUTPUT_0));
		for (int i =0; i<11; i++)
		{
			pio_set_pin_low(TWI0_CLK_GPIO);
			delay_us(2);
			pio_set_pin_high(TWI0_CLK_GPIO);
			delay_us(2);
		}
		pio_configure_pin(TWI0_CLK_GPIO, PIO_DEFAULT);
		drv_i2c_init(slave_twi_config->drv_twi_options);
		delay_us(10);
		if (gpio_pin_is_low(TWI0_DATA_GPIO))
		{
			return STATUS_FAIL;
		}
	}
	
	#ifdef USE_FREE_RTOS_DRV
	if (freertos_twi_read_packet(slave_twi_config->drv_twi_options->freertos_twi, &packet, MAX_TWI_BLOCK_TIME) != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	#else
	status = twi_master_read(slave_twi_config->drv_twi_options->p_i2c, &packet);
	if (status != TWI_SUCCESS)
	{
		return STATUS_FAIL;
	}
	#endif
	
	return STATUS_PASS;
}

int drv_i2c_checkIfTwi1(drv_twi_config_t* twi_config)
{
	//returns 1 is TWI1, 0 otherwise
	return (twi_config->p_i2c == TWI1) ? 1 : 0;
}