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
#include "subp_subProcessor.h"
#include <stdio.h>
#include <stdarg.h>
#include "drv_gpio.h"
#include "drv_uart.h"
#include "msg_messenger.h"

/* Global Variables */
xQueueHandle queue_debugManager = NULL;
xSemaphoreHandle semaphore_dbgUartWrite = NULL; 
dbg_debugLogLevel_t debugLogLevel = DBG_LOG_LEVEL_DEBUG; //default the log level to debug for now. 

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

const char* moduleNameString[] = {
"MODULE_SYSTEM_MANAGER",
"MODULE_SDCARD",
"MODULE_WIFI",
"MODULE_COMMAND",
"MODULE_DEBUG",
"MODULE_SUB_PROCESSOR",
"MODULE_DATA_MANAGER",
"MODULE_BLE",
"MODULE_NUMBER_OF_MODULES"
};

bool debugMsgOverUsb = false; 

/*	Local static functions	*/
static void processEvent(msg_message_t* message);
static void printString(char* str);
static void configure_console(void);
static char* getTimeString(); 
static void debugSocketEventCallback(SOCKET socketId, net_socketStatus_t status);
static void debugSocketReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength);
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
	.passphrase = "test$891",
	.ssid = "HeddokoTest2ghz",	
	.channel = 255, //default to 255 so it searches all channels for the signal	
};

net_socketConfig_t debugServer =
{
    .endpoint.sin_addr = 0, //irrelevant for the server
    .endpoint.sin_family = AF_INET,
    .endpoint.sin_port = _htons(6663),
    .sourceModule = MODULE_DEBUG,
    .socketStatusCallback = debugSocketEventCallback,
    .socketDataReceivedCallback = debugSocketReceivedDataCallback,
    .socketId = -1,
    .clientSocketId = -1
    
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
			dbg_processCommand(DBG_CMD_SOURCE_SERIAL,buffer,strlen(buffer));
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

void dbg_printf(dbg_debugLogLevel_t msgLogLevel, char *fmt, ...) 
{
	char buffer[200];
	if(msgLogLevel <= debugLogLevel)
	{	
		va_list va;
		va_start (va, fmt);
		vsnprintf(buffer,sizeof(buffer), fmt, va);
		va_end (va);
		printString(buffer); 
	}
}


void dbg_printString(dbg_debugLogLevel_t msgLogLevel, char* string)
{
	//only process the message if the log level is below the 
	if(msgLogLevel <= debugLogLevel)
	{
		printString(string); 	
	}
	
}


/***********************************************************************************************
 * processCommand(char* command, size_t cmdSize)
 * @brief A general Command processor which receives commands from Serial terminal and executes them
 * @param char* command, size_t cmdSize
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error 
 ***********************************************************************************************/
#define MAX_RESPONSE_STRING_SIZE 255
char responseBuffer[MAX_RESPONSE_STRING_SIZE] = {0}; 
status_t dbg_processCommand(dbg_commandSource_t source, char* command, size_t cmdSize)
{
	status_t status = STATUS_PASS; 
	size_t responseLength = 0;
	if(strncmp(command, "Record\r\n",cmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_RECORDING);
        strncpy(responseBuffer,"Starting to record!\r\n", MAX_RESPONSE_STRING_SIZE);
	}
	else if(strncmp(command, "Idle\r\n",cmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
		strncpy(responseBuffer,"Entering Idle!\r\n", MAX_RESPONSE_STRING_SIZE);
	}	
	else if(strncmp(command, "Stream\r\n",cmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_STREAMING);
        strncpy(responseBuffer,"Starting to Stream!\r\n", MAX_RESPONSE_STRING_SIZE);		
	}		
	else if(strncmp(command, "?\r\n",cmdSize) == 0)
	{
		strncpy(responseBuffer,"Brain pack alive!\r\n", MAX_RESPONSE_STRING_SIZE);
        
	}
	else if(strncmp(command, "wifiConnect\r\n",cmdSize) == 0)
	{
		net_connectToNetwork(&wirelessConfig);
	}
	else if(strncmp(command, "wifiDisconnect\r\n",cmdSize) == 0)
	{
		net_disconnectFromNetwork();
	}
	else if (strncmp(command, "buzz1\r\n",cmdSize) == 0)
	{
		drv_gpio_setPinState(DRV_GPIO_PIN_HAPTIC_OUT, DRV_GPIO_PIN_STATE_LOW);
	}
	else if (strncmp(command, "buzz0\r\n",cmdSize) == 0)
	{
		drv_gpio_setPinState(DRV_GPIO_PIN_HAPTIC_OUT, DRV_GPIO_PIN_STATE_HIGH);
	}
	else if(strncmp(command, "powerDown\r\n",cmdSize) == 0)
	{
		msg_sendMessageSimple(MODULE_SUB_PROCESSOR,MODULE_DEBUG, MSG_TYPE_SUBP_POWER_DOWN_READY, 0x00);
	}	
	else if(strncmp(command, "getTime\r\n",cmdSize) == 0)
	{
		snprintf(responseBuffer, MAX_RESPONSE_STRING_SIZE,"Time: %s\r\n",getTimeString());
	}
    else if(strncmp(command, "debugEn1\r\n",cmdSize) == 0)
	{
		debugMsgOverUsb = true; 
	}
    else if(strncmp(command, "debugEn0\r\n",cmdSize) == 0)
	{
		debugMsgOverUsb = false; 
	}         
    else if((cmdSize > 10) && strncmp(command, "PwrBrdMsg:",10) == 0)
    {
        strncpy(responseBuffer, command, sizeof(responseBuffer));
    }    
    responseLength = strlen(responseBuffer);
    if(responseLength > 0)
    {
        if(source == DBG_CMD_SOURCE_SERIAL)
        {
            drv_uart_putData(&debugUartConfig, responseBuffer,responseLength);
        }
        else if(source == DBG_CMD_SOURCE_NET)
        {
            net_sendPacketToClientSock(&debugServer, responseBuffer, responseLength,false); 
        }
        else if(source == DBG_CMD_SOURCE_USB)
        {
            subp_sendStringToUSB(responseBuffer,responseLength); 
        }   
        responseBuffer[0] = 0; //clear the string           
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
            if(message->data == NET_WIFI_STATE_CONNECTED)
            {
                if(net_createServerSocket(&debugServer, 255) == STATUS_PASS)
                {
                    printString("Initializing server socket\r\n");
                }
                else
                {
                   printString("Failed to initialize server socket\r\n"); 
                }   
            }
            else if(message->data == NET_WIFI_STATE_DISCONNECTED)
            {
               net_closeSocket(&debugServer);  
            }                                
		break;
		case MSG_TYPE_ERROR:
			dbg_printf(DBG_LOG_LEVEL_DEBUG,"Received Error from Module: %s\r\n", moduleNameString[message->source]);
		break;		
		case MSG_TYPE_SUBP_STATUS:
			printString("Received subp status\r\n");
			subpReceivedStatus = (subp_status_t*)message->parameters;
			dbg_printf(DBG_LOG_LEVEL_DEBUG,"bat: %03d mask: %04x\r\n",subpReceivedStatus->chargeLevel,subpReceivedStatus->sensorMask);
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
    if(debugServer.clientSocketId > -1)
    {
        
        net_sendPacketToClientSock(&debugServer, str, strlen(str),true); 
    }
    
    if(debugMsgOverUsb)
    {
        subp_sendStringToUSB(str,strlen(str));     
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

static void debugSocketEventCallback(SOCKET socketId, net_socketStatus_t status)
{
    switch (status)
    {
        case NET_SOCKET_STATUS_SERVER_OPEN:
            printString("Debug Server Open!\r\n");
        break;
        case NET_SOCKET_STATUS_SERVER_OPEN_FAILED:
            printString("Debug Server Open Failed!\r\n");
            //close the debug server socket
            net_closeSocket(&debugServer);
        break;
        case NET_SOCKET_STATUS_CLIENT_CONNECTED:
            printString("Client Connected!\r\n");
        break;
        case NET_SOCKET_STATUS_CLIENT_DISCONNECTED:
            printString("Client Disconnected!\r\n");
        break;        
    }
}
static void debugSocketReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength)
{
    dbg_processCommand(DBG_CMD_SOURCE_NET,buf,strnlen(buf,bufLength));
}