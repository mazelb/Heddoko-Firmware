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
#include "msg_messenger.h"

unsigned long sgSysTickCount = 0;
volatile uint8_t dataLogBufferA[DATALOG_MAX_BUFFER_SIZE] = {0} , dataLogBufferB[DATALOG_MAX_BUFFER_SIZE] = {0};
volatile uint8_t debugLogBufferA[DEBUGLOG_MAX_BUFFER_SIZE] = {0} , debugLogBufferB[DEBUGLOG_MAX_BUFFER_SIZE] = {0};
xQueueHandle queue_systemManager = NULL;

/*	Extern Variables	*/
extern char debugLogNewFileName[], debugLogOldFileName[];

sdc_file_t dataLogFile =
{
	.bufferIndexA = 0,
	.bufferIndexB = 0,
	.bufferPointerA = dataLogBufferA,
	.bufferPointerB = dataLogBufferB,
	.bufferSize = DATALOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileName = "datalog",
	.fileOpen = false,
	.activeBuffer = 0
};

sdc_file_t debugLogFile =
{
	.bufferIndexA = 0,
	.bufferIndexB = 0,
	.bufferPointerA = debugLogBufferA,
	.bufferPointerB = debugLogBufferB,
	.bufferSize = DEBUGLOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileName = "sysHdk",
	.fileOpen = false,
	.activeBuffer = 0
};

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

static void processEvent(msg_message_t message)
{
	msg_sd_card_state_t* sd_eventData;
	sd_eventData = (msg_sd_card_state_t *) message.parameters;
	
	switch (message.type)
	{
		case MSG_TYPE_SDCARD_STATE:
			if (message.source == MODULE_SDCARD)
			{
				if (sd_eventData->mounted == false)
				{
					puts("SD-Card removed\r");
					
				}
				else if (sd_eventData->mounted == true)
				{
					puts("SD-card inserted\r");
					//open new files
					sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);
					sdc_openFile(&debugLogFile, debugLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG);
				}
			}
		break;
		case MSG_TYPE_ENTERING_NEW_STATE:
		case MSG_TYPE_READY:
		case MSG_TYPE_ERROR:
		case MSG_TYPE_SDCARD_SETTINGS:
		case MSG_TYPE_WIFI_STATE:
		case MSG_TYPE_WIFI_SETTINGS:
		case MSG_TYPE_USB_CONNECTED:
		case MSG_TYPE_CHARGER_EVENT:
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		default:
		break;
	}
	
	free(message.parameters);
}

void TaskMain()
{
	bool sd_val, sd_oldVal, enable = false;
	uint8_t data[20] = {0};
	uint16_t count = 0;
	status_t status = STATUS_PASS;
	msg_message_t eventMessage;
	
	queue_systemManager = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_systemManager != 0)
	{
		msg_registerForMessages(MODULE_SYSTEM_MANAGER, 0xff, queue_systemManager);
	}
	
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
			sdc_writeToFile(&dataLogFile, (void*)data, sizeof(data));
			count++;
		}
		
		if(xQueueReceive(queue_systemManager, &(eventMessage), 1) == true)
		{
			processEvent(eventMessage);
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
	uint16_t block, page;
	uint32_t error = 0;
	
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
