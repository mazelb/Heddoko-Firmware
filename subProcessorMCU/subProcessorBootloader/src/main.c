/**
 * \file
 *
 * \brief Empty user application template
 *
 */
#include "bootloader.h"
/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
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
#include <asf.h>


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



int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
    irq_initialize_vectors();
    cpu_irq_enable();
    ////Initialize system clock and peripherals
    sysclk_init();
    
    board_init();
    
    runBootloader();
    
	/* Insert application code here, after the board has been initialized. */
}
