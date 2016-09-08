/*
 * gpm_gpioManager.h
 *
 * Created: 9/8/2016 10:07:34 AM
 *  Author: Hriday Mehta
 */ 


#ifndef GPM_GPIOMANAGER_H_
#define GPM_GPIOMANAGER_H_

#include <asf.h>
#include "common.h"
#include "drv_gpio.h"

typedef enum
{
	GPM_ONE_BUTTON_SHORT_PRESS,
	GPM_ONE_BUTTON_LONG_PRESS,
	GPM_TWO_BUTTON_SHORT_PRESS,
	GPM_TWO_BUTTON_LONG_PRESS
}gpm_buttonEvents_t;

void gpm_gpioManagerTask(void* pvParameters);

#endif /* GPM_GPIOMANAGER_H_ */