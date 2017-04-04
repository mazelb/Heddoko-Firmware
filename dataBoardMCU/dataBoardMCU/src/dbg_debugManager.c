/**
* @file dbg_debugManager.c
* @brief Contains code relevant to debug prints, logging and string command interpreter.
* The string command interpreter listens on connection to a wifi network and through the 
* USB and debug com ports.
* @author Sean Cloghesy (sean@heddoko.com)
* @date July 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/
#include "dbg_debugManager.h"
#include "asf.h"
#include "sdc_sdCard.h"
#include "net_wirelessNetwork.h"
#include "subp_subProcessor.h"
#include "tftp_fileTransferClient.h"
#include <stdio.h>
#include <stdarg.h>
#include "drv_gpio.h"
#include "drv_uart.h"
#include "drv_piezo.h"
#include "msg_messenger.h"
#include "nvm_nvMemInterface.h"


/* Global Variables */
static xQueueHandle sgQueue_debugManager = NULL;
static xSemaphoreHandle sgSemaphore_dbgUartWrite = NULL; 
static dbg_debugLogLevel_t sgDebugLogLevel = DBG_LOG_LEVEL_DEBUG; //default the log level to debug for now. 

volatile uint8_t gaDebugLogBufferA[DEBUGLOG_MAX_BUFFER_SIZE] = {0} , gaDebugLogBufferB[DEBUGLOG_MAX_BUFFER_SIZE] = {0};
static sdc_file_t sgDebugLogFile =
{
	.bufferIndexA = 0,
	.bufferIndexB = 0,
	.bufferPointerA = gaDebugLogBufferA,
	.bufferPointerB = gaDebugLogBufferB,
	.bufferSize = DEBUGLOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileName = "sysHdk",
	.fileOpen = false,
	.activeBuffer = 0,
	.sem_bufferAccess = NULL
};

const char* gaModuleNameString[] = {
	"MODULE_SYSTEM_MANAGER",
	"MODULE_SDCARD",
	"MODULE_WIFI",
	"MODULE_CONFIG_MANAGER",
	"MODULE_DEBUG",
	"MODULE_SUB_PROCESSOR",
	"MODULE_BLE",
	"MODULE_GPIO_MANAGER",
    "MODULE_TFTP_CLIENT",
	"MODULE_NUMBER_OF_MODULES"
};

const char* gaSystemStateString[] = {
    	"SYSTEM_STATE_SLEEP",
    	"SYSTEM_STATE_INIT",
    	"SYSTEM_STATE_CONNECTING",
    	"SYSTEM_STATE_WAITING_FOR_CAL",
    	"SYSTEM_STATE_CALIBRATION",
    	"SYSTEM_STATE_IDLE",
    	"SYSTEM_STATE_ERROR",
    	"SYSTEM_STATE_RECORDING",
    	"SYSTEM_STATE_STREAMING",
    	"SYSTEM_STATE_SYNCING"
};

static bool sgDebugMsgOverUsb = false; 


drv_uart_config_t gDebugUartConfig =
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

static net_wirelessConfig_t sgWirelessConfig =
{
    .securityType = M2M_WIFI_SEC_WPA_PSK,
    .passphrase = "test$891",
    .ssid = "HeddokoTest2ghz",
    .channel = 255, //default to 255 so it searches all channels for the signal
};



/*	Local static functions	*/
static void processEvent(msg_message_t* message);
static void printString(char* str);
static void configure_console(void);
static char* getTimeString(); 
static void debugSocketEventCallback(SOCKET socketId, net_socketStatus_t status);
static void debugSocketReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength);
static status_t processRecordCfg(char* command);
static status_t processStreamCfg(char* command);
static status_t processWifiCfg(char* command);
static status_t processFileTransferTest(tftp_transferType_t transferType, char* command);

/*	Extern functions	*/
/*	Extern variables	*/
extern nvmSettings_t currentSystemSettings;
static net_socketConfig_t sgDebugServer =
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


/***********************************************************************************************
* @brief The debug manager task. Is executed by the free rtos
* main task and loops while the brainpack is functional.
* Processes text debug commands and messages from other modules
* @param pvParameters, pointer to the initialization variables for the task (none for this task)
* @return void
 ***********************************************************************************************/
void dbg_debugTask(void* pvParameters)
{
	char vaBuffer[200] = {0};
	msg_message_t vEventMessage;	
	//register for system messages
	sgQueue_debugManager = xQueueCreate(10, sizeof(msg_message_t));
	if (sgQueue_debugManager != 0)
	{
		msg_registerForMessages(MODULE_DEBUG, 0xff, sgQueue_debugManager);
	}
	//initialize the semaphore
	sgSemaphore_dbgUartWrite = xSemaphoreCreateMutex();
	configure_console(); //enable printf... consider removing
	//initialize the debug uart
	drv_uart_init(&gDebugUartConfig);
	dbg_printString(DBG_LOG_LEVEL_DEBUG, "Debug Task Started\r\n");
	while(1)
	{
		if(drv_uart_getlineTimed(&gDebugUartConfig,vaBuffer,sizeof(vaBuffer),5) == STATUS_PASS)
		{			
			dbg_processCommand(DBG_CMD_SOURCE_SERIAL,vaBuffer,strlen(vaBuffer));
		}
		if(xQueueReceive(sgQueue_debugManager, &(vEventMessage), 1) == true)
		{
			processEvent(&vEventMessage);		
		}
		vTaskDelay(200);		
	}
}
/***********************************************************************************************
 * dbg_printf(dbg_debugLogLevel_t vMsgLogLevel, char *fmt, ...)
 * @brief processes debug messages from all of the other modules, uses standard printf formatting. 
 *      prints the debug string on all enabled channels. (USB, SERIAL, NETWORK) 
 * @param vMsgLogLevel, The log level of the debug message, from debug to error
 * @param fmt, printf formatted string
 * @param ..., parameters for the printf command  
 * @return void
 ***********************************************************************************************/
void dbg_printf(dbg_debugLogLevel_t vMsgLogLevel, char *fmt, ...) 
{
	char buffer[200];
	if(vMsgLogLevel <= sgDebugLogLevel)
	{	
		va_list va;
		va_start (va, fmt);
		vsnprintf(buffer,sizeof(buffer), fmt, va);
		va_end (va);
		printString(buffer); 
	}
}
/***********************************************************************************************
 * dbg_printString(dbg_debugLogLevel_t msgLogLevel, char* string)
 * @brief processes debug messages from all of the other modules and prints the 
 *      debug string on all enabled channels. (USB, SERIAL, NETWORK) 
 * @param vMsgLogLevel, The log level of the debug message, from debug to error
 * @param pointer to string that is to be printed
 * @return void
 ***********************************************************************************************/
void dbg_printString(dbg_debugLogLevel_t vMsgLogLevel, char* vpString)
{
	char vaBuffer[200];
    //only process the message if the log level is below the 
	if(vMsgLogLevel <= sgDebugLogLevel)
	{
		strncpy(vaBuffer,vpString, strlen(vpString)); 
        printString(vaBuffer); 	
	}
	
}



#define MAX_RESPONSE_STRING_SIZE 255
char newSerialNumber[10] = {0}; 
char responseBuffer[MAX_RESPONSE_STRING_SIZE] = {0}; 
/***********************************************************************************************
 * dbg_processCommand(dbg_commandSource_t source, char* command, size_t cmdSize)
 * @brief A general Command processor which receives commands from Serial terminal and executes them
 * @param source, Where the command came from, can be USB, Serial, or network (used for routing return)
 * @param command, pointer to char array of the command 
 * @param cmdSize, length of the command. 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error 
 ***********************************************************************************************/
status_t dbg_processCommand(dbg_commandSource_t vSource, char* vpCommand, size_t vCmdSize)
{
	status_t vStatus = STATUS_PASS; 
	size_t vResponseLength = 0;
    int vCmdLength = 0;
    
	if(strncmp(vpCommand, "Record\r\n",vCmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_RECORDING);
        strncpy(responseBuffer,"Starting to record!\r\n", MAX_RESPONSE_STRING_SIZE);
	}
	else if(strncmp(vpCommand, "Idle\r\n",vCmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
		strncpy(responseBuffer,"Entering Idle!\r\n", MAX_RESPONSE_STRING_SIZE);
	}	
	else if(strncmp(vpCommand, "Stream\r\n",vCmdSize) == 0)
	{		
		msg_sendBroadcastMessageSimple(MODULE_DEBUG, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_STREAMING);
        strncpy(responseBuffer,"Starting to Stream!\r\n", MAX_RESPONSE_STRING_SIZE);		
	}
    else if(strncmp(vpCommand, "recordCfg",9) == 0)
	{		
		if(processRecordCfg(vpCommand+9) == STATUS_PASS)
        {
            strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }
        else
        {
            strncpy(responseBuffer,"NACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }             
        		
	}	
    else if(strncmp(vpCommand, "streamCfg",9) == 0)
	{		
		if(processStreamCfg(vpCommand+9) == STATUS_PASS)
        {
            strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }
        else
        {
            strncpy(responseBuffer,"NACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }        		
	}	    
    else if(strncmp(vpCommand, "wifiCfg",7) == 0)
	{		
		if(processWifiCfg(vpCommand+7) == STATUS_PASS)
        {
            strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }
        else
        {
            strncpy(responseBuffer,"NACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }             
        		
	}
    else if(strncmp(vpCommand,"setSerial",9) == 0)
    {
        vCmdLength = strlen(vpCommand+9); 
        strncpy(newSerialNumber, vpCommand+9, 7); 
        snprintf(responseBuffer, MAX_RESPONSE_STRING_SIZE,"Serial Set: %s %d\r\n",newSerialNumber, vCmdLength);
        msg_sendMessage(MODULE_SYSTEM_MANAGER,MODULE_DEBUG,MSG_TYPE_SET_SERIAL,newSerialNumber);         
    }
    else if(strncmp(vpCommand,"saveConfig",10) == 0)
    {
        msg_sendMessage(MODULE_SYSTEM_MANAGER,MODULE_DEBUG,MSG_TYPE_SAVE_SETTINGS,NULL); 
        strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
    }        		
	else if(strncmp(vpCommand, "?\r\n",vCmdSize) == 0)
	{
		strncpy(responseBuffer,"Brain pack alive!\r\n", MAX_RESPONSE_STRING_SIZE);        
	}
    else if(strncmp(vpCommand, "getSettings\r\n",vCmdSize) == 0)
    {
           snprintf(responseBuffer,MAX_RESPONSE_STRING_SIZE,"SSID:%s\r\nkey:%s\r\nAdvertisingPort:%d\r\nConfigPort:%d\r\nStreamPort:%d\r\n",
           currentSystemSettings.defaultWifiConfig.ssid,currentSystemSettings.defaultWifiConfig.passphrase,currentSystemSettings.advPortNumber,
           currentSystemSettings.serverPortNumber, currentSystemSettings.streamCfg.streamPort);
    }        
	else if(strncmp(vpCommand, "wifiConnect\r\n",vCmdSize) == 0)
	{
		net_connectToNetwork(&sgWirelessConfig);
	}
	else if(strncmp(vpCommand, "wifiDisconnect\r\n",vCmdSize) == 0)
	{
		net_disconnectFromNetwork();
	}
	else if (strncmp(vpCommand, "buzz1\r\n",vCmdSize) == 0)
	{
		drv_gpio_setPinState(DRV_GPIO_PIN_HAPTIC_OUT, DRV_GPIO_PIN_STATE_LOW);
	}
	else if (strncmp(vpCommand, "buzz0\r\n",vCmdSize) == 0)
	{
		drv_gpio_setPinState(DRV_GPIO_PIN_HAPTIC_OUT, DRV_GPIO_PIN_STATE_HIGH);
	}
	else if(strncmp(vpCommand, "powerDown\r\n",vCmdSize) == 0)
	{
		msg_sendMessageSimple(MODULE_SUB_PROCESSOR,MODULE_DEBUG, MSG_TYPE_SUBP_POWER_DOWN_READY, 0x00);
	}	
	else if(strncmp(vpCommand, "getTime\r\n",vCmdSize) == 0)
	{
		snprintf(responseBuffer, MAX_RESPONSE_STRING_SIZE,"Time: %s\r\n",getTimeString());        
	}
    else if(strncmp(vpCommand, "debugEn1\r\n",vCmdSize) == 0)
	{
		sgDebugMsgOverUsb = true; 
        strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
	}
    else if(strncmp(vpCommand, "debugEn0\r\n",vCmdSize) == 0)
	{
		sgDebugMsgOverUsb = false; 
        strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
	}         
    else if((vCmdSize > 10) && strncmp(vpCommand, "PwrBrdMsg:",10) == 0)
    {
        strncpy(responseBuffer, vpCommand, sizeof(responseBuffer));
    }
    else if((vCmdSize > 10) && strncmp(vpCommand, "sendFile",8) == 0)
    {
        
        if(processFileTransferTest(TFTP_TRANSFER_TYPE_SEND_FILE,vpCommand+8) == STATUS_PASS)
        {
            strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }
        else
        {
            strncpy(responseBuffer,"NACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }
    }    
    else if((vCmdSize > 10) && strncmp(vpCommand, "getFile",7) == 0)
    {
        
        if(processFileTransferTest(TFTP_TRANSFER_TYPE_GET_FILE,vpCommand+7) == STATUS_PASS)
        {
            strncpy(responseBuffer,"ACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }
        else
        {
            strncpy(responseBuffer,"NACK\r\n", MAX_RESPONSE_STRING_SIZE);
        }
    }  
    else if(strncmp(vpCommand, "disablePiezo\r\n",vCmdSize)==0)
    {
        drv_piezo_togglePiezo(false);
    }    
    else if(strncmp(vpCommand, "sendBleInit\r\n",vCmdSize)==0)
    {
        msg_sendMessageSimple(MODULE_BLE, MODULE_DEBUG, MSG_TYPE_DEBUG_BLE, 0);
    }
    vResponseLength = strlen(responseBuffer);
    if(vResponseLength > 0)
    {
        if(vSource == DBG_CMD_SOURCE_SERIAL)
        {
            drv_uart_putData(&gDebugUartConfig, responseBuffer,vResponseLength);
        }
        else if(vSource == DBG_CMD_SOURCE_NET)
        {
            net_sendPacketToClientSock(&sgDebugServer, responseBuffer, vResponseLength,false); 
        }
        else if(vSource == DBG_CMD_SOURCE_USB)
        {
            subp_sendStringToUSB(responseBuffer,vResponseLength); 
        }   
        responseBuffer[0] = 0; //clear the string           
    }        
	return vStatus;	
}
static void processEvent(msg_message_t* message)
{
	subp_status_t* subpReceivedStatus = NULL; 
	switch(message->msgType)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
			dbg_printf(DBG_LOG_LEVEL_DEBUG,"New State: %s\r\n",gaSystemStateString[message->data]);			
		break; 
		case MSG_TYPE_SDCARD_STATE:
			dbg_printf(DBG_LOG_LEVEL_DEBUG,"Received SD Card state Event:%d\r\n",message->data);
		break;
		case MSG_TYPE_WIFI_STATE:
			
            if(message->data == NET_WIFI_STATE_CONNECTED)
            {
                dbg_printf(DBG_LOG_LEVEL_DEBUG,"Wifi Connected\r\n");
                if(net_createServerSocket(&sgDebugServer, 255) == STATUS_PASS)
                {
                    dbg_printf(DBG_LOG_LEVEL_DEBUG,"Initializing server socket\r\n");
                }
                else
                {
                   dbg_printf(DBG_LOG_LEVEL_DEBUG,"Failed to initialize server socket\r\n"); 
                }   
            }
            else if(message->data == NET_WIFI_STATE_DISCONNECTED)
            {
               dbg_printf(DBG_LOG_LEVEL_DEBUG,"Wifi Disconnected\r\n");
               net_closeSocket(&sgDebugServer);  
            }                                
		break;
		case MSG_TYPE_ERROR:
			dbg_printf(DBG_LOG_LEVEL_DEBUG,"Received Error from Module: %s\r\n", gaModuleNameString[message->source]);
		break;		
		case MSG_TYPE_SUBP_STATUS:
			//dbg_printf(DBG_LOG_LEVEL_DEBUG,"Received subp status\r\n");
			//subpReceivedStatus = (subp_status_t*)message->parameters;
			//dbg_printf(DBG_LOG_LEVEL_DEBUG,"bat: %03d mask: %04x\r\n",subpReceivedStatus->chargeLevel,subpReceivedStatus->sensorMask);
		break;
	}
} 


static void printString(char* str)
{
	//if the semaphore is initialized, use it, otherwise just send the data.
	if(sgSemaphore_dbgUartWrite != NULL)
	{
		if(xSemaphoreTake(sgSemaphore_dbgUartWrite,5) == true)
		{
			drv_uart_putString(&gDebugUartConfig, str);
			xSemaphoreGive(sgSemaphore_dbgUartWrite);
		}
	}
	else
	{
		drv_uart_putString(&gDebugUartConfig, str);
	}
    if(sgDebugServer.clientSocketId > -1)
    {        
        net_sendPacketToClientSock(&sgDebugServer, str, strlen(str),true); 
    }
    
    if(sgDebugMsgOverUsb)
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
            net_closeSocket(&sgDebugServer);
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
subp_recordingConfig_t newRecordingCfg;
static status_t processRecordCfg(char* command)
{   
    //the format of the recordCfg command is, "recordCfg,<filename>,<sensor mask in hex>,<dataRate>\r\n"   
    int ret = sscanf(command, ", %s ,%x,%u\r\n",newRecordingCfg.filename, &(newRecordingCfg.sensorMask), &(newRecordingCfg.rate)); 
    //the ret value should be 3 if the information was correctly parsed. 
    if(ret == 3)
    {
        //send to both modules, TODO in the future use the broadcast method when all the module have their masks configured. 
        msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_DEBUG, MSG_TYPE_RECORDING_CONFIG,&newRecordingCfg);
        msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_RECORDING_CONFIG,&newRecordingCfg);    
        return STATUS_PASS;
    }
    else
    {
        return STATUS_FAIL;
    }
}

subp_streamConfig_t newStreamCfg;
static status_t processStreamCfg(char* command)
{   
    int ip[4] = {0,0,0,0}; 
    //the format of the streamCfg command is, "streamCfg,<Port>,<IP Address xxx.xxx.xxx.xxx>\r\n"   
    int ret = sscanf(command, ",%d,%d.%d.%d.%d\r\n",&(newStreamCfg.streamPort), ip,ip+1,ip+2,ip+3); 
    //the ret value should be 5 if the information was correctly parsed. 
    if(ret == 5)
    {        
        newStreamCfg.ipaddress.s_addr = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0] ; 
        //send to both modules, TODO in the future use the broadcast method when all the module have their masks configured. 
        msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_DEBUG, MSG_TYPE_STREAM_CONFIG,&newStreamCfg);
        msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_STREAM_CONFIG,&newStreamCfg);    
        return STATUS_PASS;
    }
    else
    {
        return STATUS_FAIL;
    }
}



static status_t processWifiCfg(char* command)
{
    int securitySetting = 0; 
    //the format of the streamCfg command is, "wifiCfg,<SSID>,<Passphrase><securityMode 1 = none, 2 = WPA, 3 = WEP>\r\n"   
    int ret = sscanf(command, ", %s , %s ,%d\r\n",&(sgWirelessConfig.ssid),&(sgWirelessConfig.passphrase),&securitySetting); 
    //the ret value should be 3 if the information was correctly parsed. 
    if(ret == 3)
    {
        
        if(securitySetting < 1 || securitySetting > 3)
        {
            //invalid security setting
            return STATUS_FAIL;
        }
        sgWirelessConfig.securityType = securitySetting; 
        msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_WIFI_CONFIG,&sgWirelessConfig);    
        return STATUS_PASS;
    }
    else
    {
        return STATUS_FAIL;
    }    
}
tftp_transferParameters_t transferParameters = {.filename = "TestFile.txt", .transferType = TFTP_TRANSFER_TYPE_SEND_FILE};
static status_t processFileTransferTest(tftp_transferType_t transferType, char* command)
{
    int ip[4] = {0,0,0,0};
    uint16_t port;
    //the format of the streamCfg command is, "streamCfg,<Port>,<IP Address xxx.xxx.xxx.xxx>\r\n"
    int ret = sscanf(command, ",%d,%d.%d.%d.%d\r\n",&port, ip,ip+1,ip+2,ip+3);
    transferParameters.transferEndpoint.sin_port = _htons(port);
    transferParameters.transferType = transferType;
    if(transferType == TFTP_TRANSFER_TYPE_GET_FILE)
    {
        strcpy(transferParameters.filename,"firmware.bin");
    }
    else
    {
        strcpy(transferParameters.filename,"testFile.txt");
    }
    //the ret value should be 5 if the information was correctly parsed.
    if(ret == 5)
    {
        transferParameters.transferEndpoint.sin_addr.s_addr = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0] ;
        //send to both modules, TODO in the future use the broadcast method when all the module have their masks configured.
        msg_sendMessage(MODULE_TFTP_CLIENT, MODULE_DEBUG, MSG_TYPE_TFTP_INITIATE_TRANSFER,&transferParameters);
        return STATUS_PASS;
    }
    else
    {
        return STATUS_FAIL;
    }
}
