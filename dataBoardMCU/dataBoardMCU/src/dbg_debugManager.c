/*
 * dbg_debugManager.c
 *
 * Created: 7/12/2016 10:25:21 AM
 *  Author: sean
 */ 
#include "dbg_debugManager.h"
#include "asf.h"
#include "sdc_sdCard.h"
#include "net_wirelessNetwork.h"
#include <stdio.h>
#include <stdarg.h>
#include "drv_uart.h"
#include "msg_messenger.h"
#include "subp_subProcessor.h"

/* Global Variables */
xQueueHandle queue_debugManager = NULL;
xSemaphoreHandle semaphore_dbgUartWrite = NULL; 
dgb_debugLogLevel_t debugLogLevel = DBG_LOG_LEVEL_DEBUG; //default the log level to debug for now. 

volatile uint8_t debugLogBufferA[DEBUGLOG_MAX_BUFFER_SIZE] = {0} , debugLogBufferB[DEBUGLOG_MAX_BUFFER_SIZE] = {0};
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
	.activeBuffer = 0,
	.sem_bufferAccess = NULL
};

/*	Local static functions	*/
static status_t processCommand(char* command, size_t cmdSize);
static void processEvent(msg_message_t* message);
static void printString(char* str);
static void configure_console(void);
static char* getTimeString(); 
/*	Extern functions	*/
/*	Extern variables	*/



drv_uart_config_t debugUartConfig =
{
	.p_usart = UART1,
	.mem_index = 1,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
	.mode = DRV_UART_MODE_INTERRUPT
};

net_wirelessConfig_t wirelessConfig = 
{
	.securityType = M2M_WIFI_SEC_WPA_PSK,
	.passphrase = "heddoko123",
	.ssid = "heddokoTestNet",	
	.channel = 255, //default to 255 so it searches all channels for the signal	
};

void dbg_debugTask(void* pvParameters)
{
	status_t status = STATUS_PASS;
	char buffer[200] = {0};
	msg_message_t eventMessage;	
	//register for system messages
	queue_debugManager = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_debugManager != 0)
	{
		msg_registerForMessages(MODULE_DEBUG, 0xff, queue_debugManager);
	}
	//initialize the semaphore
	semaphore_dbgUartWrite = xSemaphoreCreateMutex();
	configure_console(); //enable printf... consider removing
	//initialize the debug uart
	status = drv_uart_init(&debugUartConfig);
	dbg_printString(DBG_LOG_LEVEL_DEBUG, "Debug Task Started\r\n");
	while(1)
	{
		if(drv_uart_getlineTimed(&debugUartConfig,buffer,sizeof(buffer),5) == STATUS_PASS)
		{			
			processCommand(buffer,strlen(buffer));
		}
		if(xQueueReceive(queue_debugManager, &(eventMessage), 1) == true)
		{
			processEvent(&eventMessage);		
		}
		vTaskDelay(200);		
	}
}

status_t dbg_init()
{
	status_t status = STATUS_PASS;
	status = drv_uart_init(&debugUartConfig); 
	return status; 
}

void dgb_printf(dgb_debugLogLevel_t msgLogLevel, char *fmt, ...) 
{
	char buffer[200];
	if(msgLogLevel <= debugLogLevel)
	{	
		va_list va;
		va_start (va, fmt);
		vsnprintf(buffer,sizeof(buffer), fmt, va);
		va_end (va);
		drv_uart_putString(&debugUartConfig,buffer); 
	}
}


void dbg_printString(dgb_debugLogLevel_t msgLogLevel, char* string)
{
	//only process the message if the log level is below the 
	if(msgLogLevel <= debugLogLevel)
	{
		drv_uart_putString(&debugUartConfig,string); 	
	}
	
}

/***********************************************************************************************
 * processCommand(char* command, size_t cmdSize)
 * @brief A general Command processor which receives commands from Serial terminal and executes them
 * @param char* command, size_t cmdSize
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error 
 ***********************************************************************************************/
static status_t processCommand(char* command, size_t cmdSize)
{
	status_t status = STATUS_PASS; 
	
	if(strncmp(command, "Record\r\n",cmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_LEAVING_STATE, SYSTEM_STATE_IDLE);
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_RECORDING);
		printString("Starting to record!\r\n");
	}
	else if(strncmp(command, "Idle\r\n",cmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_LEAVING_STATE, SYSTEM_STATE_RECORDING);
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
		printString("Entering Idle!\r\n");
	}	
	else if(strncmp(command, "Stream\r\n",cmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_STREAMING);
		printString("Starting to record!\r\n");
	}		
	else if(strncmp(command, "?\r\n",cmdSize) == 0)
	{
		printString("Brain pack alive!\r\n");
	}
	else if(strncmp(command, "wifiConnect\r\n",cmdSize) == 0)
	{
		net_connectToNetwork(&wirelessConfig);
	}
	else if(strncmp(command, "wifiDisconnect\r\n",cmdSize) == 0)
	{
		net_disconnectFromNetwork();
	}
	else if(strncmp(command, "powerDown\r\n",cmdSize) == 0)
	{
		msg_sendMessageSimple(MODULE_SUB_PROCESSOR,MODULE_DEBUG, MSG_TYPE_SUBP_POWER_DOWN_READY, 0x00);
	}	
	else if(strncmp(command, "getTime\r\n",cmdSize) == 0)
	{
		dgb_printf(DBG_LOG_LEVEL_DEBUG,"Time: %s\r\n",getTimeString());
	}	
	return status;	
}
static void processEvent(msg_message_t* message)
{
	subp_status_t* subpReceivedStatus = NULL; 
	switch(message->type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
			printString("Received Entering New State event\r\n");			
		break; 
		case MSG_TYPE_SDCARD_STATE:
			printString("Received SD Card state Event\r\n");
		break;
		case MSG_TYPE_WIFI_STATE:
			printString("Received Wifi state event\r\n");
		break;
		case MSG_TYPE_SUBP_STATUS:
			printString("Received subp status\r\n");
			subpReceivedStatus = (subp_status_t*)message->parameters;
			dgb_printf(DBG_LOG_LEVEL_DEBUG,"battery Level: %03d\r\n",subpReceivedStatus->chargeLevel);
		break;
	}
} 
static void printString(char* str)
{
	//if the semaphore is initialized, use it, otherwise just send the data.
	if(semaphore_dbgUartWrite != NULL)
	{
		if(xSemaphoreTake(semaphore_dbgUartWrite,5) == true)
		{
			drv_uart_putString(&debugUartConfig, str);
			xSemaphoreGive(semaphore_dbgUartWrite);
		}
	}
	else
	{
		drv_uart_putString(&debugUartConfig, str);
	}
}

static void configure_console(void)
{
	const usart_serial_options_t usart_serial_options = 
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS,
	};

	/* Configure console UART. */
	stdio_serial_init(UART1, &usart_serial_options);

	/* Specify that stdout should not be buffered. */
	#if defined(__GNUC__)
		setbuf(stdout, NULL);
	#else
	/* Already the case in IAR's Normal DLIB default configuration: printf()
	 * emits one character at a time.
	 */
	#endif
}

char timeString[100] = {0}; 
static char* getTimeString()
{
	uint32_t hour, minute, second; 
	rtc_get_time(RTC,&hour,&minute,&second); 
	sprintf(timeString,"%02d:%02d:%02d",hour,minute,second); 
	return timeString; 
} 