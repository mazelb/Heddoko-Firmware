/*
 * drv_i2c.h
 *
 * Created: 2/15/2016 4:45:40 PM
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

#ifndef DRV_I2C_H_
#define DRV_I2C_H_

#include "asf.h"

#define MAX_TWI_BLOCK_TIME	(100UL / portTICK_RATE_MS)

#define USE_FREE_RTOS_DRV

typedef struct
{
	Twi* p_i2c;
	twi_options_t twi_options;
	const freertos_peripheral_options_t freeRtos_options;
	freertos_twi_if freertos_twi;
}drv_twi_config_t;

typedef struct
{
	int emId;
	int address;
	drv_twi_config_t* drv_twi_options;
}slave_twi_config_t;

int drv_i2c_init(drv_twi_config_t*  twi_config);
int drv_i2c_write(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t data);
int drv_i2c_read(slave_twi_config_t* slave_twi_config, uint8_t reg, uint8_t* data, uint8_t length);
int drv_i2c_checkIfTwi1(drv_twi_config_t* twi_config);

#endif /* DRV_I2C_H_ */