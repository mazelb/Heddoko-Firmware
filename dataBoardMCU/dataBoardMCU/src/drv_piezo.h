/*
 * drv_piezo.h
 *
 * Created: 8/31/2016 4:21:47 PM
 *  Author: Hriday Mehta
 */ 


#ifndef DRV_PIEZO_H_
#define DRV_PIEZO_H_

#include <asf.h>
#include "drv_tc.h"
#include "drv_gpio.h"

typedef struct  
{
	drv_gpio_pins_t gpioPin;	// pin connected to the piezo buzzer
	// add more stuff as required
}drv_piezo_config_t;

typedef struct 
{
	uint16_t noteFrequency;	// NOTE: use 0Hz for no tone
	uint16_t postNoteDelay;	// length of the note (how long should it play!)
}drv_piezo_noteElement_t;

typedef struct  
{
	drv_piezo_noteElement_t* p_noteElementArray;
	uint16_t totalArrayElements;
}drv_piezo_notePattern_t;

void drv_piezo_init(drv_piezo_config_t* peizoConfig);
void drv_piezo_playTone(uint16_t noteFrequency);
void drv_piezo_playPattern(drv_piezo_notePattern_t notePattern);

#endif /* DRV_PIEZO_H_ */