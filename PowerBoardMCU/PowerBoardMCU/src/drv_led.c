/*
 * drv_led.c
 *
 * Created: 10/27/2015 10:45:28 AM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 

#include "drv_led.h"

//local structure to store led configuration
drv_led_config_t ledConfig;

xTimerHandle LedTimer;	//timer handle for the led timer task
volatile drv_led_color_type_t vledColor;	//global variable to store led color, used in timer callback
volatile drv_led_state_type_t vledState; //global variable to store led state, used in timer callback

//Led Timer callback function
void vLedTimerCallback( xTimerHandle xTimer )
{
	if(vledState == DRV_LED_FLASH)	//perform the action only if state is flashing
	{
		//check which led color is set and toggle the pin state
		switch(vledColor)
		{
			case DRV_LED_RED:
				drv_gpio_togglePin(ledConfig.redLed);
				break;
			case DRV_LED_BLUE:
				drv_gpio_togglePin(ledConfig.blueLed);
				break;
			case DRV_LED_GREEN:
				drv_gpio_togglePin(ledConfig.greenLed);
				break;
			case DRV_LED_YELLOW:
				drv_gpio_togglePin(ledConfig.redLed);
				drv_gpio_togglePin(ledConfig.greenLed);
				break;
			case DRV_LED_WHITE:
				drv_gpio_togglePin(ledConfig.redLed);
				drv_gpio_togglePin(ledConfig.greenLed);
				drv_gpio_togglePin(ledConfig.blueLed);
				break;
			case DRV_LED_TURQUOISE:
				drv_gpio_togglePin(ledConfig.greenLed);
				drv_gpio_togglePin(ledConfig.blueLed);
				break;
			case DRV_LED_PURPLE:
				drv_gpio_togglePin(ledConfig.redLed);
				drv_gpio_togglePin(ledConfig.blueLed);
				break;
			default:
				break;
			
		}
	}
	else
	{
		//usually shouldn't end up here
		//if it does the timer was not stopped while initializing led to SOLID state
		xTimerStop(LedTimer, 0);
	}
}
int timerId = 0;
//store the led configuration
status_t drv_led_init(drv_led_config_t* configLed)
{
	status_t result = STATUS_PASS;
	//store the led configuration
	ledConfig.redLed = configLed->redLed;
	ledConfig.blueLed = configLed->blueLed;
	ledConfig.greenLed = configLed->greenLed;
	
	//create LedTimer task
	LedTimer = xTimerCreate("Led Timer", (LED_BLINK_RATE/portTICK_RATE_MS), pdTRUE, &timerId, vLedTimerCallback);
	if (LedTimer == NULL)
	{
		result = STATUS_FAIL;
	}
	return result;
}

status_t drv_led_set(drv_led_color_type_t ledColor, drv_led_state_type_t ledState)
{
	status_t result = STATUS_PASS;
	if(LedTimer != NULL)
	{
		if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
		{
			//timer is active, stop it
			xTimerStop(LedTimer, 0);		
		}
	}
	//switch off all LEDs
	drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_HIGH);
	
	vledColor = ledColor;	//copy the color to a global variable used in timer callback
	vledState = ledState;	//copy the state to a global variable used in timer callback
	switch (ledColor)
	{
		case DRV_LED_RED:
			drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_LOW);
			break;
		case DRV_LED_BLUE:
			drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_LOW);
			break;
		case DRV_LED_GREEN:
			drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_LOW);
			break;
		case DRV_LED_YELLOW:
			drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_LOW);
			drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_LOW);
			break;
		case DRV_LED_WHITE:
			drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_LOW);
			drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_LOW);
			drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_LOW);
			break;
		case DRV_LED_TURQUOISE:
			drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_LOW);
			drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_LOW);
			break;
		case DRV_LED_PURPLE:
			drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_LOW);
			drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_LOW);
			break;
		case DRV_LED_OFF:
			drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_HIGH);
			break;
		default:
			break;
	}
	
	if ((ledState == DRV_LED_FLASH) & (ledColor != DRV_LED_OFF))
	{
		if(LedTimer != NULL)
		{
			xTimerReset(LedTimer, 0);	
		}
		
	}
	else
	{
		if(LedTimer != NULL)
		{				
			//stop the timer
			if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
			{
				xTimerStop(LedTimer, 0);
			}
		}
	}
	return result;
}

void drv_led_activate_timer()
{
	
	xTimerStart(LedTimer, 0);
}