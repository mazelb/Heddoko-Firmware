/*
 * gpm_gpioManager.c
 *
 * Created: 9/8/2016 10:07:45 AM
 *  Author: Hriday Mehta
 */ 

#include "gpm_gpioManager.h"
#include "dbg_debugManager.h"

/*	Local Defines	*/
#define GPM_LONG_PRESS_DURATION	(1*SECONDS)		// the duration for which a button should be pressed to be considered as a long press
#define GPM_HARD_RESET_TIMEOUT	(10*SECONDS)	// the duration for which two buttons should be pressed to perform hard system reset

/*	Static function declarations	*/
void acSw1_intHandler();
void acSw2_intHandler();
static void vGpioDurationTimerCallBack();

/*	Local variables	*/
typedef enum
{
	GPM_INTERRUPT_LOW = 0,
	GPM_INTERRUPT_HIGH
}pinInterruptState;

pinInterruptState acSw1_intState, acSw2_intState;
bool acSw1_isPressed = false, acSw2_isPressed = false;
bool acSw1_intGenerated = false, acSw2_intGenerated = false, gpioDurationTimerIntGenerated = false;
uint32_t currentGpioTimerDuration = GPM_LONG_PRESS_DURATION;
xTimerHandle gpioDurationTimer = NULL;

void gpm_gpioManagerTask(void* pvParameters)
{
	UNUSED(pvParameters);
	
	// register for button interrupts
	drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_AC_SW1, DRV_GPIO_INTERRUPT_LOW_EDGE, acSw1_intHandler);
	acSw1_intState = GPM_INTERRUPT_LOW;
	drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_AC_SW2, DRV_GPIO_INTERRUPT_LOW_EDGE, acSw2_intHandler);
	acSw2_intState = GPM_INTERRUPT_LOW;
	
	// initialize the timer for determining button press durations
	gpioDurationTimer = xTimerCreate("GDT", (GPM_LONG_PRESS_DURATION/portTICK_RATE_MS), pdFALSE, NULL, vGpioDurationTimerCallBack);	// set the default period to long press duration
	if (gpioDurationTimer == NULL)
	{
		dbg_printString("Failed to created GPIO duration timer\r\n");
	}
	currentGpioTimerDuration = GPM_LONG_PRESS_DURATION;
	
	while (1)
	{
		// check for button press interrupts and scrutinize the events
		if (acSw1_intGenerated)
		{
			acSw1_intGenerated = false;	// clear the flag to indicate that the interrupt was serviced
			
			if (acSw1_intState == GPM_INTERRUPT_LOW)	// the button was pressed
			{
				// reconfigure the interrupts
				drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW1, DRV_GPIO_INTERRUPT_HIGH_EDGE);
				acSw1_intState = GPM_INTERRUPT_HIGH;
				acSw1_isPressed = true;
				// start the timer to determine the duration of press
				if (acSw2_isPressed)
				{
					// both the switches are pressed, configure timer for hard reset timeout
					xTimerChangePeriod(gpioDurationTimer, (GPM_HARD_RESET_TIMEOUT/portTICK_RATE_MS), NULL);
					xTimerStart(gpioDurationTimer, NULL);
					currentGpioTimerDuration = GPM_HARD_RESET_TIMEOUT;
				}
				else
				{
					xTimerReset(gpioDurationTimer, NULL);
				}
			}
			else	// the button was released
			{
				// reconfigure the interrupts
				drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW1, DRV_GPIO_INTERRUPT_LOW_EDGE);
				acSw1_intState = GPM_INTERRUPT_LOW;
				acSw1_isPressed = false;
				// stop the timer
				xTimerChangePeriod(gpioDurationTimer, GPM_LONG_PRESS_DURATION, NULL);
				currentGpioTimerDuration = GPM_LONG_PRESS_DURATION;
				xTimerStop(gpioDurationTimer, NULL);
			}
		}
		
		if (acSw2_intGenerated)
		{
			acSw2_intGenerated = false;	// clear the flag to indicate that the interrupt was serviced
			
			if (acSw2_intState == GPM_INTERRUPT_LOW)	// the button was pressed
			{
				// reconfigure the interrupts
				drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW2, DRV_GPIO_INTERRUPT_HIGH_EDGE);
				acSw2_intState = GPM_INTERRUPT_HIGH;
				acSw2_isPressed = true;
				// start the timer to determine the duration of press
				if (acSw1_isPressed)
				{
					// both the switches are pressed, configure timer for hard reset timeout
					xTimerChangePeriod(gpioDurationTimer, (GPM_HARD_RESET_TIMEOUT/portTICK_RATE_MS), NULL);
					xTimerStart(gpioDurationTimer, NULL);
					currentGpioTimerDuration = GPM_HARD_RESET_TIMEOUT;
				}
				else
				{
					xTimerReset(gpioDurationTimer, NULL);
				}
			}
			else	// the button was released
			{
				// reconfigure the interrupts
				drv_gpio_config_interrupt(DRV_GPIO_PIN_AC_SW2, DRV_GPIO_INTERRUPT_LOW_EDGE);
				acSw2_intState = GPM_INTERRUPT_LOW;
				acSw2_isPressed = false;
				// stop the timer
				xTimerChangePeriod(gpioDurationTimer, GPM_LONG_PRESS_DURATION, NULL);
				currentGpioTimerDuration = GPM_LONG_PRESS_DURATION;
				xTimerStop(gpioDurationTimer, NULL);
			}
		}
		
		if (gpioDurationTimerIntGenerated)
		{
			// stop the timer
			xTimerStop(gpioDurationTimer, NULL);
			
			// pass the respective event
			if ((acSw1_isPressed && acSw2_isPressed) && (currentGpioTimerDuration = GPM_HARD_RESET_TIMEOUT))
			{
				// both the switches are pressed, send message for hard reset
				
				// change the timer period back to long press duration
				xTimerChangePeriod(gpioDurationTimer, GPM_LONG_PRESS_DURATION, NULL);
				currentGpioTimerDuration = GPM_LONG_PRESS_DURATION;
			}
			else if (acSw1_isPressed)
			{
				// pass ac sw1 event - long press
			}
			else if (acSw2_isPressed)
			{
				// pass ac sw2 event - long press
			}
		}
		
		vTaskDelay(10);
	}
}

/*	Static function definitions	*/
void acSw1_intHandler()
{
	// TODO: insert the call to the a new function in drv_gpio that imitates the interrupt handlers in the it.
	acSw1_intGenerated = true;	// set the flag to indicate that the interrupt was generated
}

void acSw2_intHandler()
{
	acSw2_intGenerated = true;	// set the flag to indicate that the interrupt was generated
}

static void vGpioDurationTimerCallBack()
{
	gpioDurationTimerIntGenerated = true;
}