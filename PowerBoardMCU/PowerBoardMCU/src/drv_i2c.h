/*
 * drv_i2c.h
 *
 * Created: 2/12/2016 10:21:16 AM
 *  Author: Hriday Mehta
 */ 
/**
 * @file  drv_i2c.h
 * @brief Low level I2C driver
 * @author Heddoko
 * @note This driver can be used on TWI0 and TWI1
 *
 * Copyright Heddoko(TM) 2015, all rights reserved
 */

#include "asf.h"
#include "twi.h"

#ifndef DRV_I2C_H_
#define DRV_I2C_H_

#define FIFO_BUFFER_SIZE 512
#define MAX_TWI_BLOCK_TIME	(100UL / portTICK_RATE_MS)
#define CONF_TWI_TIMEOUT	1500

//#define USE_FREE_RTOS_DRV
#ifndef USE_FREE_RTOS_DRV
//#define USE_BUFFERED_DRV
#endif

//Disable the two defines below if using NPA-201 or MS5611
#define ENABLE_TWO_BYTE_WRITE
#define ENABLE_MULTI_BYTE_READ

typedef struct
{
	uint8_t data_buf[FIFO_BUFFER_SIZE];
	uint8_t addr_buf[FIFO_BUFFER_SIZE];
	uint16_t i_first;
	uint16_t i_last;
	uint16_t num_bytes;
}twi_fifo_typedef;

typedef struct
{
	Twi* p_i2c;
	twi_options_t twi_options;
	IRQn_Type IRQtype;
	int mem_index;
}drv_twi_config_t;

typedef struct
{
	int id;
	int address;
	drv_twi_config_t* drv_twi_options;
}slave_twi_config_t;


int drv_i2c_init(drv_twi_config_t*  twi_config);
int drv_i2c_write(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t data);
int drv_i2c_read(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t* data, uint8_t length);
int drv_i2c_enable_interrupt(drv_twi_config_t* twi_config, uint32_t ul_sources);
int drv_i2c_disable_interrupt(drv_twi_config_t* twi_config, uint32_t ul_sources);

#endif /* DRV_I2C_H_ */