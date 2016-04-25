/*
 * Tasks.c
 *
 * Created: 9/9/2015 12:28:29 PM
 *  Author: Hriday Mehta
 */ 

#include "Tasks.h"
#include "Board_Init.h"

/**
 * \brief Called if stack overflow during execution
 */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
		signed char *pcTaskName)
{
	//printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	/* If the parameters have been corrupted then inspect pxCurrentTCB to
	 * identify which task has overflowed its stack.
	 */
	for (;;) {
	}
}

/**
 * \brief This function is called by FreeRTOS idle task
 */
extern void vApplicationIdleHook(void)
{
}

/**
 * \brief This function is called by FreeRTOS each tick
 */
extern void vApplicationTickHook(void)
{
}

/**
 * \brief This task is initialized first to initiate the board peripherals and run the initial tests
 */
void TaskMain(void *pvParameters)
{
	// This skeleton code simply sets the LED to the state of the button.
	
	UNUSED(pvParameters);
	/*	Create a Semaphore to pass between tasks	*/
	vSemaphoreCreateBinary(DebugLogSemaphore);
	
	powerOnInit();
	initializeNodsAndQuintics();
	
	for (;;) 
	{
		/*	Debug code */
	
		// Is button pressed?
		if (ioport_get_pin_level(PIN_SW0_GPIO) == BUTTON_0_ACTIVE)
		{
			// Yes, so turn LED on.
			ioport_set_pin_level(LED_0_PIN, LED_0_ACTIVE);
		} 
		else
		{
			// No, so turn LED off.
			ioport_set_pin_level(LED_0_PIN, !LED_0_ACTIVE);
		}
		vTaskDelay(10);
	}
}


/**
 * \brief This task, when activated, make LED blink at a fixed rate
 */
void TaskLed(void *pvParameters)
{
	// This skeleton code simply sets the LED to the state of the button.
	
	UNUSED(pvParameters);
	printf("Task Led Initialized\r\n");
	
	for (;;) 
	{
		/*	Debug code */
	
		// Is button pressed?
		if (ioport_get_pin_level(PIN_SW0_GPIO) == BUTTON_0_ACTIVE)
		{
			// Yes, so turn LED on.
			ioport_set_pin_level(LED_0_PIN, LED_0_ACTIVE);
		} 
		else
		{
			// No, so turn LED off.
			ioport_set_pin_level(LED_0_PIN, !LED_0_ACTIVE);
		}

	}
}

