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

#define GPM_MAX_GPIO_TO_MONITOR	10

typedef enum
{
	GPM_BUTTON_ONE_SHORT_PRESS,
	GPM_BUTTON_ONE_LONG_PRESS,
	GPM_BUTTON_TWO_SHORT_PRESS,
	GPM_BUTTON_TWO_LONG_PRESS,
	GPM_BOTH_BUTTON_LONG_PRESS
}gpm_buttonEvents_t;

/*	FOR FUTURE REVISIONS
typedef struct  
{
	drv_gpio_pins_t buttonToMonitor;
	drv_gpio_interrupt_t defaultInterruptState;
	bool registerForShortPress;
	bool registerForLongPress;
	bool hasTwoButtonCombo;
	drv_gpio_pins_t twoButtonComboPartner;
}gpm_buttonConfig_t;

typedef struct  
{
	uint8_t numberOfButtonsToMonitor;
	gpm_buttonConfig_t array_buttonsConfig[];
}gpm_buttonsToMonitorConfig_t;
*/

void gpm_gpioManagerTask(void* pvParameters);

#endif /* GPM_GPIOMANAGER_H_ */