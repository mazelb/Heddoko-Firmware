/**
* @file drv_piezo.h
* @brief Driver for the piezo electric speaker
* @author Hriday Mehta
* @date August 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/

#ifndef DRV_PIEZO_H_
#define DRV_PIEZO_H_

#include <asf.h>
#include "drv_tc.h"
#include "drv_gpio.h"

typedef struct  
{
	drv_gpio_pins_t gpioPin;	// pin connected to the piezo buzzer
}drv_piezo_config_t;

typedef struct 
{
	uint16_t noteFrequency;	// NOTE: use 0Hz for no tone
	uint16_t postNoteDelay;	// length of the note (how long should it play!)
}drv_piezo_noteElement_t;

void drv_piezo_init(drv_piezo_config_t* peizoConfig);
void drv_piezo_playTone(uint16_t noteFrequency);
void drv_piezo_stopPlaying();
void drv_piezo_playPattern(drv_piezo_noteElement_t *notePattern, uint16_t totalElements);
void drv_piezo_togglePiezo(bool piezoState);
#endif /* DRV_PIEZO_H_ */