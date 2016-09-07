/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
/*
 * drv_haptic.h
 *
 * Created: 9/7/2016 9:48:41 AM
 *  Author: Hriday Mehta
 */ 


#ifndef DRV_HAPTIC_H_
#define DRV_HAPTIC_H_

#include <asf.h>
#include "common.h"
#include "drv_gpio.h"

#define DRV_HAPTIC_BUZZ_RATE	250		// the default on-off buzz rate 

typedef struct
{
	drv_gpio_pins_t hapticGpio;
	drv_gpio_pin_state_t onState;
	drv_gpio_pin_state_t offState;
	// add more stuff as needed
}drv_haptic_config_t;

typedef struct
{
	uint32_t onTime;	// motor-on duration in ms, cannot be nulls
	uint32_t offTime;	// motor-off duration in ms, cannot be nulls
}drv_haptic_patternElement_t;

status_t drv_haptic_init(drv_haptic_config_t *hapticConfig);
status_t drv_haptic_playPattern(drv_haptic_patternElement_t *patternElementArray, uint32_t totalElements);
status_t drv_haptic_stopPlayback();

#endif /* DRV_HAPTIC_H_ */