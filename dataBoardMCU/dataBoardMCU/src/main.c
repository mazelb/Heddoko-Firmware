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
#include "sys_systemManager.h"

void  prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}

void HardFault_Handler()
{
	__asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
	//while (1);
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
	if (xTaskCreate(sys_systemManagerTask, "SM", TASK_SYSTEM_MANAGER_STACK_SIZE, NULL, TASK_SYSTEM_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create main task\r");
	}
	
	// NOTE: All other tasks are initialized from the System Manager.
	
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
