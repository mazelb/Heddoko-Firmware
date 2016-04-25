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

extern unsigned long sgSysTickCount;

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
	
	status = i2c_master_init(&twi_config->module, twi_config->p_i2c, &twi_config->twi_options);
	if (status != STATUS_OK)
	{
		//puts("Failed to initialize as master.\r\n");
		return STATUS_FAIL;	//return error
	}
	
	i2c_master_enable(&twi_config->module);
	
	return STATUS_PASS;
}

int drv_i2c_write(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t data)
{
	int status = STATUS_FAIL;
	//Write one byte to desired register
	uint8_t dataPacket[2] = {0};
	struct i2c_master_packet packet;
	dataPacket[0] = reg;
	dataPacket[1] = data;
	packet.data = &dataPacket[0];
	packet.data_length = 2;
	packet.address = (uint32_t) slave_twi_config->address;
	packet.ten_bit_address = false;
	packet.high_speed = false;	
	
	status = i2c_master_write_packet_wait(&slave_twi_config->drv_twi_options->module, &packet);
	if (status != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	return STATUS_PASS;
}

int drv_i2c_read(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t* data, uint8_t length)
{
	int status = STATUS_FAIL;
	//Write the address of register to read from
	uint8_t dataPacket[2] = {0};
	struct i2c_master_packet packet;
	dataPacket[0] = reg;
	packet.data = &dataPacket[0];
	packet.data_length = 1;
	packet.address = (uint32_t) slave_twi_config->address;
	packet.ten_bit_address = false;
	packet.high_speed = false;
	
	status = i2c_master_write_packet_wait(&slave_twi_config->drv_twi_options->module, &packet);
	if (status != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	//initialize the read sequence
	packet.data = data;
	packet.data_length = length;
	status = i2c_master_read_packet_wait(&slave_twi_config->drv_twi_options->module, &packet);
	if (status != STATUS_OK)
	{
		return STATUS_FAIL;
	}
	
	return STATUS_PASS;
}
