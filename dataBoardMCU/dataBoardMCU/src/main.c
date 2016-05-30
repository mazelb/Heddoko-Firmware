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

#include <asf.h>
#include "common.h"
#include "string.h"
#include "sdc_sdCard.h"

unsigned long sgSysTickCount = 0;

/*	Extern Variables	*/
extern sdc_file_t dataLogFile, debugLogFile;
extern char debugLogNewFileName[], debugLogOldFileName[];

/*	Extern functions	*/
extern bool sdCardPresent();

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

void init_sd_card()
{
	//static FATFS fs;
	//static FATFS* fs1;	//pointer to FATFS structure used to check free space
	//static FRESULT res;
	//static DWORD freeClusters, freeSectors, totalSectors;
	//status_t result = STATUS_FAIL;
	//Ctrl_status status = STATUS_FAIL;
	//
	//gpio_configure_pin(SD_CD_PIN, (PIO_INPUT | PIO_PULLUP));
	///* Configure HSMCI pins */
	//gpio_configure_pin(PIN_HSMCI_MCCDA_GPIO, PIN_HSMCI_MCCDA_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCCK_GPIO, PIN_HSMCI_MCCK_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA0_GPIO, PIN_HSMCI_MCDA0_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA1_GPIO, PIN_HSMCI_MCDA1_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA2_GPIO, PIN_HSMCI_MCDA2_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA3_GPIO, PIN_HSMCI_MCDA3_FLAGS);
	//
	//sd_mmc_init();
	//do
	//{
		//status = sd_mmc_test_unit_ready(0);
		//if (CTRL_FAIL == status)
		//{
			//puts("Card install FAIL\n\r");
			//puts("Please unplug and re-plug the card.\n\r");
			//while (CTRL_NO_PRESENT != sd_mmc_check(0))
			//{
				//;
			//}
		//}
	//} while (CTRL_GOOD != status);
	//
	///*	Mount the SD card	*/
	//if(status == CTRL_GOOD)
	//{
		//memset(&fs, 0, sizeof(FATFS));
		//res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		//if (res == FR_INVALID_DRIVE)
		//{
			//puts("Error: Invalid Drive\r");
			//return result;
		//}
//
		////Check the free space on card
		//status = f_getfree("0:", &freeClusters, &fs1);
		//if (status != FR_OK)
		//{
			//result = STATUS_FAIL;
			//puts("Error: Cannot calculate free space\r");
			//return result;
		//}
		//totalSectors = (fs1->n_fatent -2) * fs1->csize;	//only needed to calculate used space
		//freeSectors = freeClusters * fs1->csize;	//assuming 512 bytes/sector
		//if ((freeSectors/2) < 307200)
		//{
			//result = STATUS_FAIL;
			//puts("Error: Low disk space on SD-card\r");
			//return result;
		//}
	//}
}

void TaskMain()
{
	bool sd_val, sd_oldVal, enable = false;
	char data[20] = {0};
	uint16_t count = 0;
	
	vTaskDelay(5000);
	//open new files
	sdc_openFile(&dataLogFile, "dataLog", SDC_FILE_OPEN_READ_WRITE_APPEND);
	sdc_openFile(&debugLogFile, "sysHdk", SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG);
	
	while (1)
	{
		sd_val = ioport_get_pin_level(SD_CD_PIN);
		if (sd_val != sd_oldVal)
		{
			if (sd_val == true)
			{
				puts("Card removed\r");
				enable = false;
			}
			else
			{
				puts("Card inserted\r");
			}
		}
		sd_oldVal = sd_val;
		
		if (!ioport_get_pin_level(SW0_PIN))
		{
			while(!ioport_get_pin_level(SW0_PIN));
			enable = !enable;
			ioport_set_pin_level(LED_0_PIN, !enable);
			sdc_writeToFile(&debugLogFile, "Button Press\r\n", 14);	
		}
		if (enable)
		{
			snprintf(data, 21, "Data count: %06d\r\n", count);
			puts(data);
			sdc_writeToFile(&dataLogFile, data, sizeof(data));		
			count++;
		}
		vTaskDelay(10);
	}
}

void configure_console()
{
	const usart_serial_options_t conf_uart = 
	{
		.baudrate = CONF_UART_BAUDRATE,
		.charlength = 0,
		.paritytype = CONF_UART_PARITY,
		.stopbits = 1
	};

	stdio_serial_init(UART1, &conf_uart);
	setbuf(stdout, NULL);
}

int main (void)
{
	status_t status = STATUS_FAIL;

	irq_initialize_vectors();
	cpu_irq_enable();
	board_init();
	sysclk_init(); 
	NVIC_EnableIRQ(SysTick_IRQn);
	configure_console();
	puts("System Initialized.\r");
	
	/* Insert application code here, after the board has been initialized. */
	if (xTaskCreate(TaskMain, "Main", TASK_MAIN_STACK_SIZE, NULL, TASK_MAIN_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create main task\r");
	}
	if (xTaskCreate(sdc_sdCardTask, "SD", TASK_SD_CARD_STACK_SIZE, NULL, TASK_SD_CARD_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create SD-card task\r");
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
