/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
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
#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bootloader.h"
#include "Board_Init.h"
#include "settings.h"
#include "conf_board.h"
#include "task_main.h"
#include "GPIO.h"
#include "drv_uart.h"
#include "drv_gpio.h"
#include "DebugLog.h"
#include "task_quinticInterface.h"
#include "task_commandProc.h"


//these values are actually defined in Board_Init.c
extern void xPortSysTickHandler(void);
extern drv_uart_config_t uart0Config;
extern drv_uart_config_t uart1Config;
extern drv_uart_config_t usart0Config;
extern drv_uart_config_t usart1Config;
extern brainSettings_t brainSettings; 
volatile unsigned long sgSysTickCount = 0;

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

/**
 * \brief Handler for System Tick interrupt.
 */
void SysTick_Handler(void)
{
	sgSysTickCount++;
	xPortSysTickHandler();
}


int main (void)
{
	irq_initialize_vectors();
	cpu_irq_enable();
	//Initialize system clock and peripherals
	sysclk_init();
	#ifdef COMPILE_AS_BOOTLOADER
	runBootloader(); 
	#else
	
	//turn on pullups for SWDIO (PB5) and SWDIO (PB6)
	PIOB->PIO_PUER |= PIO_PB5 | PIO_PB6; 
	board_init();
	

	
	
	/*	Create task Main	*/
	if (xTaskCreate(TaskMain, "Main", TASK_MAIN_STACK_SIZE, NULL, TASK_MAIN_STACK_PRIORITY, NULL ) != pdPASS)
	{
		//debugPrintString("Failed to create Main task\r\n");
	}

	/* Start the scheduler. */
	vTaskStartScheduler();
	
	#endif
	//we should never get here. 
	/*	Debug code */
	while (1) 
	{		
		delay_ms(1000);
	}
	return 0;
}
