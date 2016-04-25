/*
 * LTC2941_1.h
 *
 * Created: 2016-03-23 4:56:19 PM
 *  Author: Hriday Mehta
 */ 


#ifndef LTC2941-1_H_
#define LTC2941-1_H_

#include "common.h"
#include "drv_i2c.h"

#define ENABLE_DEBUG_PRINTS

#define LTC_2941_STATUS_REG_ADDR				0x00
#define LTC_2941_CONTROL_REG_ADDR				0x01
#define LTC_2941_ACC_CHRG_REG_ADDR				0x02
#define LTC_2941_CHRG_THRESHOLD_HIGH_REG_ADDR	0x04
#define LTC_2941_CHRG_THRESHOLD_LOW_REG_ADDR	0x06

status_t ltc2941Init(slave_twi_config_t* slaveConfig);
status_t ltc2941SetCharge(slave_twi_config_t* slaveConfig, uint16_t chargeLevel);
uint16_t ltc2941GetCharge(slave_twi_config_t* slaveConfig);
uint32_t getCalculatedPercentage(slave_twi_config_t* slaveConfig);
uint32_t getRegValueForPercent(uint32_t percent);
status_t ltc2941SetChargeComplete(slave_twi_config_t* slaveConfig);
uint8_t ltc2941GetStatus(slave_twi_config_t* slaveConfig);

#endif /* LTC2941-1_H_ */