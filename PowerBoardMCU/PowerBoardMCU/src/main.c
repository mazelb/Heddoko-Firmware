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
 /**
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <asf.h>
#include "brd_board.h"
#include "cmd_commandProc.h"
#include "mgr_managerTask.h"
#include "dat_dataRouter.h"
#include "chrg_chargeMonitor.h"

extern void xPortSysTickHandler(void);
extern void vApplicationMallocFailedHook( void );
void HardFault_Handler()
{
	while(1); 
}
void MemManage_Handler()
{
	while(1); 
}
void BusFault_Handler()
{
	while(1); 
}
void UsageFault_Handler()
{
	while(1); 
}

void vApplicationMallocFailedHook( void )
{
	while(1);
}

///**
 //* \brief Handler for System Tick interrupt.
 //*/
//void SysTick_Handler(void)
//{
	////sgSysTickCount++;
	//xPortSysTickHandler();
//}

int main (void)
{
	irq_initialize_vectors();
	cpu_irq_enable();
	////Initialize system clock and peripherals
	sysclk_init();	
	
	board_init();
	brd_enableWatchdog();
	//drv_gpio_initializeAll();
	//drv_gpio_setPinState(DRV_GPIO_PIN_LED_RED, DRV_GPIO_PIN_STATE_LOW);
	//drv_gpio_setPinState(DRV_GPIO_PIN_LED_BLUE, DRV_GPIO_PIN_STATE_LOW);
	//drv_gpio_setPinState(DRV_GPIO_PIN_LED_GREEN, DRV_GPIO_PIN_STATE_LOW);
	// Insert application code here, after the board has been initialized.
	//start all the tasks
	int retCode = xTaskCreate(mgr_managerTask, "MGR", TASK_MANAGER_STACK_SIZE, NULL, TASK_MANAGER_PRIORITY, NULL);
	if (retCode != pdPASS)
	{
		printf("Failed to create MGR task code %d\r\n", retCode);
	}	
	/* Start the scheduler. */
	vTaskStartScheduler();	
	
	// This skeleton code simply sets the LED to the state of the button.
	while (1) 
	{
		// Is button pressed?
		if (ioport_get_pin_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE) 
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
