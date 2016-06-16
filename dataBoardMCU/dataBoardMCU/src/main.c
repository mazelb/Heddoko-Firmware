/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * This is a bare minimum user application template.
 *
 * For documentation of the board, go \ref group_common_boards "here" for a link
 * to the board-specific documentation.
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to board_init()
 * -# Basic usage of on-board LED and button
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

//should contain a call to system manager

#include <asf.h>
#include "common.h"
#include "string.h"
#include "sdc_sdCard.h"
#include "msg_messenger.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "task_SensorHandler.h"
#include "sys_systemManager.h"

void HardFault_Handler()
{
	while (1);
}
void BusFault_Handler()
{
	while (1);
}
void MemManage_Handler()
{
	while (1);
}
void UsageFault_Handler()
{
	while (1);
}

int main (void)
{	
	/*	Board initialization	*/
	irq_initialize_vectors();
	cpu_irq_enable();
	sysclk_init();
	
	/* Insert application code here, after the board has been initialized. */
	if (xTaskCreate(systemManager, "SM", TASK_SYSTEM_MANAGER_STACK_SIZE, NULL, TASK_SYSTEM_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create main task\r");
	}
	if (xTaskCreate(sdc_sdCardTask, "SD", TASK_SD_CARD_STACK_SIZE, NULL, TASK_SD_CARD_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create SD-card task\r");
	}
	if (xTaskCreate(task_SensorHandler, "SH", TASK_SENSOR_HANDLER_STACK_SIZE, NULL, TASK_SENSOR_HANDLER_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create sensor handler task\r");
	}
	
	vTaskStartScheduler();
	
	/* This skeleton code simply sets the LED to the state of the button. */
	while (1) 
	{
		/* Is button pressed? */
		if (ioport_get_pin_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE) 
		{
			/* Yes, so turn LED on. */
			ioport_set_pin_level(LED_0_PIN, LED_0_ACTIVE);
		} 
		else 
		{
			/* No, so turn LED off. */
			ioport_set_pin_level(LED_0_PIN, !LED_0_ACTIVE);
		}
	}
}
