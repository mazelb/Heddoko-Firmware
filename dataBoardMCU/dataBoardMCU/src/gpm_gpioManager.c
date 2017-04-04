/**
* @file gpm_gpioManager.c
* @brief Driver for handling the button GPIO, creates events based on different
*       button combinations. 
* @author Hriday Mehta (Hriday@heddoko.com)
* @date September 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/
#include "gpm_gpioManager.h"
#include "dbg_debugManager.h"
#include "msg_messenger.h"

/*	Local Defines	*/
#define GPM_LONG_PRESS_DURATION	(1*SECONDS)		// the duration for which a button should be pressed to be considered as a long press
#define GPM_HARD_RESET_TIMEOUT	(10*SECONDS)	// the duration for which two buttons should be pressed to perform hard system reset

/*	Static function declarations	*/
static void vGpioDurationTimerCallBack();
static void changeTimerPeriod(uint32_t newPeriod);
void acSw1_intHandler(uint32_t ul_id, uint32_t ul_mask);
void acSw2_intHandler(uint32_t ul_id, uint32_t ul_mask);

/*	Local variables	*/
typedef enum
{
	GPM_INTERRUPT_LOW = 0,	// the next time interrupt is generated, the button GPIO will be low
	GPM_INTERRUPT_HIGH		// the next time interrupt is generated, the button GPIO will be high
}pinInterruptState;

pinInterruptState acSw1_intState, acSw2_intState;
bool acSw1_isPressed = false, acSw2_isPressed = false;
bool acSw1_intGenerated = false, acSw2_intGenerated = false, gpioDurationTimerIntGenerated = false;
uint32_t currentGpioTimerDuration = GPM_LONG_PRESS_DURATION;	// variable to track the GPIO timer duration
xTimerHandle gpioDurationTimer = NULL;                                                                                                                

/*	Function definitions	*/

/***********************************************************************************************
 * gpm_gpioManagerTask(void* pvParameters)
 * @brief Main task to monitor the GPIOs.
 * @param void* pvParameters: void
 * @return void
 ***********************************************************************************************/
void gpm_gpioManagerTask(void* pvParameters)
{
	UNUSED(pvParameters);
	
	// register for button interrupts, save the current interrupt state
	drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_AC_SW1, DRV_GPIO_INTERRUPT_LOW_EDGE, (void*)acSw1_intHandler);
	acSw1_intState = GPM_INTERRUPT_LOW;
	drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_AC_SW2, DRV_GPIO_INTERRUPT_LOW_EDGE, (void*)acSw2_intHandler);
	acSw2_intState = GPM_INTERRUPT_LOW;
	
	// initialize the timer for determining button press durations
	gpioDurationTimer = xTimerCreate("GDT", (GPM_LONG_PRESS_DURATION/portTICK_RATE_MS), pdFALSE, NULL, vGpioDurationTimerCallBack);	// set the default period to long press duration
	if (gpioDurationTimer == NULL)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR, "Failed to created GPIO duration timer\r\n");
	}
	currentGpioTimerDuration = GPM_LONG_PRESS_DURATION;
	
	while (1)
	{
		// first check if any timer events are generated
		if (gpioDurationTimerIntGenerated)
		{
			// stop the timer
			xTimerStop(gpioDurationTimer, NULL);
			gpioDurationTimerIntGenerated = false;
			
			// pass the respective event
			if ((acSw1_isPressed && acSw2_isPressed) && (currentGpioTimerDuration == GPM_HARD_RESET_TIMEOUT))
			{
				// both the switches are pressed, send message for hard reset
				msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_GPIO_MANAGER, MSG_TYPE_GPM_BUTTON_EVENT, GPM_BOTH_BUTTON_LONG_PRESS);
				
				// change the timer period back to long press duration
				changeTimerPeriod(GPM_LONG_PRESS_DURATION);
				
				// clear the flag to indicate that the message was passed
				acSw1_isPressed = false;
				acSw2_isPressed = false;
			}
			else if (acSw1_isPressed)
			{
				// pass ac sw1 event - long press
				msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_GPIO_MANAGER, MSG_TYPE_GPM_BUTTON_EVENT, GPM_BUTTON_ONE_LONG_PRESS);
				
				// clear the flag to indicate that the message was passed
				acSw1_isPressed = false;
			}
			else if (acSw2_isPressed)
			{
				// pass ac sw2 event - long press
				msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_GPIO_MANAGER, MSG_TYPE_GPM_BUTTON_EVENT, GPM_BUTTON_TWO_LONG_PRESS);
				
				// clear the flag to indicate that the message was passed
				acSw2_isPressed = false;
			}
		}
		
		// check for button press interrupts and scrutinize the events
		if (acSw1_intGenerated)
		{
			acSw1_intGenerated = false;	// clear the flag to indicate that the interrupt was serviced
			
			if (acSw1_intState == GPM_INTERRUPT_HIGH)	// the button was pressed
			{
				acSw1_isPressed = true;
				// start the timer to determine the duration of press
				if (acSw2_isPressed)
				{
					// both the switches are pressed, configure timer for hard reset timeout
					changeTimerPeriod(GPM_HARD_RESET_TIMEOUT);
					xTimerReset(gpioDurationTimer, NULL);
				}
				else
				{
					changeTimerPeriod(GPM_LONG_PRESS_DURATION);
					xTimerReset(gpioDurationTimer, NULL);
				}
			}
			else	// the button was released
			{
				if (acSw1_isPressed)
				{
					// pass the message to indicate short press.
					msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_GPIO_MANAGER, MSG_TYPE_GPM_BUTTON_EVENT, GPM_BUTTON_ONE_SHORT_PRESS);
				}
				acSw1_isPressed = false;
				// stop the timer
				xTimerStop(gpioDurationTimer, NULL);
				changeTimerPeriod(GPM_LONG_PRESS_DURATION);
			}
		}
		
		if (acSw2_intGenerated)
		{
			acSw2_intGenerated = false;	// clear the flag to indicate that the interrupt was serviced
			
			if (acSw2_intState == GPM_INTERRUPT_HIGH)	// the button was pressed
			{
				acSw2_isPressed = true;
				// start the timer to determine the duration of press
				if (acSw1_isPressed)
				{
					// both the switches are pressed, configure timer for hard reset timeout
					changeTimerPeriod(GPM_HARD_RESET_TIMEOUT);
					xTimerReset(gpioDurationTimer, NULL);
				}
				else
				{
					changeTimerPeriod(GPM_LONG_PRESS_DURATION);
					xTimerReset(gpioDurationTimer, NULL);
				}
			}
			else	// the button was released
			{
				if (acSw2_isPressed)
				{
					// pass the message to indicate short press.
					msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_GPIO_MANAGER, MSG_TYPE_GPM_BUTTON_EVENT, GPM_BUTTON_TWO_SHORT_PRESS);
				}
				acSw2_isPressed = false;
				// stop the timer
				changeTimerPeriod(GPM_LONG_PRESS_DURATION);
				xTimerStop(gpioDurationTimer, NULL);
			}
		}
		
		vTaskDelay(10);
	}
}

/*	Static function definitions	*/
/***********************************************************************************************
 * changeTimerPeriod(uint32_t newPeriod)
 * @brief Checks and updates the period of the gpio timer
 * @param uint32_t newPeriod: new period for the timer
 * @return void
 ***********************************************************************************************/
static void changeTimerPeriod(uint32_t newPeriod)
{
	if (currentGpioTimerDuration != newPeriod)	// only change period if the current period is different one
	{
		xTimerChangePeriod(gpioDurationTimer, (newPeriod/portTICK_RATE_MS), NULL);
		currentGpioTimerDuration = newPeriod;
	}
}

/***********************************************************************************************
 * acSw1_intHandler(uint32_t ul_id, uint32_t ul_mask)
 * @brief Interrupt handler for action switch 1.
 * @param uint32_t ul_id, uint32_t ul_mask
 * @return void
 ***********************************************************************************************/
void acSw1_intHandler(uint32_t ul_id, uint32_t ul_mask)
{
	drv_gpio_service_Int(DRV_GPIO_PIN_AC_SW1, ul_mask, &acSw1_intGenerated);
	// reconfigure the interrupt
	if (acSw1_intState == GPM_INTERRUPT_LOW)
	{
		// it was a low edge interrupt, reconfigure to be a high edge
		drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW1, DRV_GPIO_INTERRUPT_HIGH_EDGE);
		acSw1_intState = GPM_INTERRUPT_HIGH;
	}
	else
	{
		// it was a high edge interrupt, reconfigure to be a low edge
		drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW1, DRV_GPIO_INTERRUPT_LOW_EDGE);
		acSw1_intState = GPM_INTERRUPT_LOW;
	}
}

/***********************************************************************************************
 * acSw2_intHandler(uint32_t ul_id, uint32_t ul_mask)
 * @brief Interrupt handler for action switch 2.
 * @param uint32_t ul_id, uint32_t ul_mask
 * @return void
 ***********************************************************************************************/
void acSw2_intHandler(uint32_t ul_id, uint32_t ul_mask)
{
	drv_gpio_service_Int(DRV_GPIO_PIN_AC_SW2, ul_mask, &acSw2_intGenerated);
	// reconfigure the interrupt
	if (acSw2_intState == GPM_INTERRUPT_LOW)
	{
		// it was a low edge interrupt, reconfigure to be a high edge
		drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW2, DRV_GPIO_INTERRUPT_HIGH_EDGE);
		acSw2_intState = GPM_INTERRUPT_HIGH;
	}
	else
	{
		// it was a high edge interrupt, reconfigure to be a low edge
		drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW2, DRV_GPIO_INTERRUPT_LOW_EDGE);
		acSw2_intState = GPM_INTERRUPT_LOW;
	}
}

/***********************************************************************************************
 * vGpioDurationTimerCallBack()
 * @brief Interrupt handler for the gpio duration timer
 * @param void
 * @return void
 ***********************************************************************************************/
static void vGpioDurationTimerCallBack()
{
	gpioDurationTimerIntGenerated = true;
}