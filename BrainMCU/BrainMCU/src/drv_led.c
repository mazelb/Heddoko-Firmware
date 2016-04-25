/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
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
drv_led_color_type_t vledColor, vAlternateColor;	//global variables to store led color, used in timer callback
drv_led_state_type_t vledState; //global variable to store led state, used in timer callback
uint8_t ledBlinkCount = 0;	//global count to sequence the LED blink patterns
bool vtoggleLedBlinkRate = FALSE, vEnableAlternateBlink = FALSE;

//local static function declarations
static void set_led_color(drv_led_color_type_t ledColor);
static void toggle_led_color(drv_led_color_type_t ledColor);

/**
 * @brief: Timer callback function. Used for the blink functionality of LEDs
 **/
void vLedTimerCallback( xTimerHandle xTimer )
{
	if((vledState != DRV_LED_SOLID) && (vEnableAlternateBlink == FALSE))	//perform the action only if state is flashing
	{
		//check which led color is set and toggle the pin state
		toggle_led_color(vledColor);
		
		//set multiple blink patterns here
		if ((vledState != DRV_LED_FLASH) && (vledState != DRV_LED_SOLID))
		{
			if (ledBlinkCount > 0)
			{
				if (vtoggleLedBlinkRate == TRUE)
				{
					if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
					{
						//timer is active, stop it
						xTimerStop(LedTimer, 0);
					}
					portBASE_TYPE result = xTimerChangePeriod(LedTimer, (LED_BLINK_RATE/portTICK_RATE_MS), 0);	//reconfiguring timer restarts it
					vtoggleLedBlinkRate = FALSE;
				}
				ledBlinkCount--;
				if (ledBlinkCount == 1)
				{
					if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
					{
						//timer is active, stop it
						xTimerStop(LedTimer, 0);
					}
					portBASE_TYPE res = xTimerChangePeriod(LedTimer, (BLINK_DELAY/portTICK_RATE_MS), 0);	//reconfiguring timer restarts it
					vtoggleLedBlinkRate = TRUE;
					ledBlinkCount = 2*vledState +1;	//reload the ledBlinkCount
				}
			}
		}
	}
	
	//Blink two colors alternately
	else if ((vledState != DRV_LED_SOLID) && (vEnableAlternateBlink == TRUE))
	{
		ledBlinkCount--;
		if (ledBlinkCount == 4)
		{
			set_led_color(vledColor);
		}
		else if (ledBlinkCount == 3)
		{
			//switch off all LEDs
			drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_HIGH);
		}
		else if (ledBlinkCount == 2)
		{
			set_led_color(vAlternateColor);
		}
		else
		{
			//switch off all LEDs
			drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_HIGH);
			ledBlinkCount = 5;
		}
	}
	else
	{
		//usually shouldn't end up here
		//if it does the timer was not stopped while initializing led to SOLID state
		xTimerStop(LedTimer, 0);
	}
}

/***********************************************************************************************
 * drv_led_init(drv_led_config_t* configLed)
 * @brief initialize LED driver and the timer
 * @param configLed, the configuration structure for the LED, contains gpio mappings.  
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_led_init(drv_led_config_t* configLed)
{
	status_t result = STATUS_PASS;
	//store the led configuration
	ledConfig.redLed = configLed->redLed;
	ledConfig.blueLed = configLed->blueLed;
	ledConfig.greenLed = configLed->greenLed;
	
	//create LedTimer task
	LedTimer = xTimerCreate("Led Timer", (LED_BLINK_RATE/portTICK_RATE_MS), pdTRUE, NULL, vLedTimerCallback);
	if (LedTimer == NULL)
	{
		result = STATUS_FAIL;
	}
	return result;
}

/***********************************************************************************************
 * drv_led_set(drv_led_color_type_t ledColor, drv_led_state_type_t ledState)
 * @brief Set the LED color and blink pattern
 * @param ledColor- the color to be set, ledState- one of the states enumerated in drv_led_state_type_t   
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_led_set(drv_led_color_type_t ledColor, drv_led_state_type_t ledState)
{
	status_t result = STATUS_PASS;
	if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
	{
		//timer is active, stop it
		xTimerStop(LedTimer, 0);		
	}
	//configure the multiple blink pattern here
	if ((ledState == DRV_LED_SOLID) || (ledState == DRV_LED_FLASH))
	{
		ledBlinkCount = 0;
	}
	else
	{
		ledBlinkCount = 2*ledState;
	}
	
	//switch off all LEDs
	drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_HIGH);
	
	vEnableAlternateBlink = FALSE;	//clear the alternate blink flag
	vAlternateColor = DRV_LED_OFF;	//clear the alternate blink color
	vledColor = ledColor;	//copy the color to a global variable used in timer callback
	vledState = ledState;	//copy the state to a global variable used in timer callback
	
	set_led_color(ledColor);
	
	if ((ledState != DRV_LED_SOLID) & (ledColor != DRV_LED_OFF))
	{
		xTimerChangePeriod(LedTimer, (LED_BLINK_RATE/portTICK_RATE_MS), 0);
		xTimerReset(LedTimer, 0);
	}
	else
	{
		//stop the timer
		if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
		{
			xTimerStop(LedTimer, 0);
		}
	}
	return result;
}

/***********************************************************************************************
 * drv_led_activate_timer()
 * @brief Activate or start the LED timer.
 * @param Void
 * @return Void
 ***********************************************************************************************/
void drv_led_activate_timer()
{
	xTimerStart(LedTimer, 0);
}

/***********************************************************************************************
 * drv_led_set_alternate_blink(drv_led_color_type_t ledColor1, drv_led_color_type_t ledColor2, drv_led_state_type_t ledState)
 * @brief Set the LED to alternately blink two colors. Only DRV_LED_FLASH is an accepted state
 * @param ledColor1, ledColor2- two colors to blink alternately, ledState- the state of LED
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_led_set_alternate_blink(drv_led_color_type_t ledColor1, drv_led_color_type_t ledColor2, drv_led_state_type_t ledState)
{
	if (ledState != DRV_LED_FLASH)
	{
		ledState = DRV_LED_FLASH;
	}
	if ((ledColor1 != DRV_LED_OFF) && (ledColor2 != DRV_LED_OFF) && (ledColor1 != ledColor2))
	{
		vledState = ledState;	//copy the state to a global variable used in timer callback
		vledColor = ledColor1;	//copy the color to a global variable used in timer callback
		vAlternateColor = ledColor2;	//set the alternate blink color
		vEnableAlternateBlink = TRUE;	//set the alternate blink flag
		ledBlinkCount = 4;	//used in the timer callback to sequence the blinks
		status_t result = STATUS_PASS;
		if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
		{
			//timer is active, stop it
			xTimerStop(LedTimer, 0);		
		}
		//switch off all LEDs
		drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_HIGH);
		drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_HIGH);
		drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_HIGH);
		set_led_color(ledColor1);
	
		if ((ledState != DRV_LED_SOLID) & (ledColor1 != DRV_LED_OFF))
		{
			xTimerChangePeriod(LedTimer, (LED_BLINK_RATE/portTICK_RATE_MS), 0);
			xTimerReset(LedTimer, 0);
		}
		else
		{
			//stop the timer
			if (xTimerIsTimerActive(LedTimer) != pdFALSE)	//check if timer is active or dormant
			{
				xTimerStop(LedTimer, 0);
			}
		}
	}
	else
	{
		vEnableAlternateBlink = FALSE;
		return STATUS_FAIL;
	}
	return STATUS_PASS;
}

/**
 * @brief: Turn on LEDs corresponding to specified color
 **/
static void set_led_color(drv_led_color_type_t ledColor)
{
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
			case DRV_LED_OFF:
				drv_gpio_setPinState(ledConfig.redLed, DRV_GPIO_PIN_STATE_HIGH);
				drv_gpio_setPinState(ledConfig.greenLed, DRV_GPIO_PIN_STATE_HIGH);
				drv_gpio_setPinState(ledConfig.blueLed, DRV_GPIO_PIN_STATE_HIGH);
				break;
			default:
				break;
		}
}

/**
 * @brief: Toggle LEDs corresponding to specified color
 **/
static void toggle_led_color(drv_led_color_type_t ledColor)
{
	switch(ledColor)
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
		default:
		break;
		
	}
}