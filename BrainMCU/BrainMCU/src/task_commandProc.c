/*
 * task_commandProc.c
 *
 * Created: 10/28/2015 9:46:58 AM
 * Copyright Heddoko(TM) 2015, all rights reserved
 *  Author: sean
 */ 
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "task_commandProc.h"
#include "task_stateMachine.h"
#include "task_quinticInterface.h"
#include "task_sdCardWrite.h"
#include "task_fabricSense.h"
#include "rtc.h"

//external values
extern xQueueHandle queue_dataHandler;
extern uint32_t totalBytesWritten;
extern uint32_t totalFramesWritten;
extern unsigned long sgSysTickCount;
extern quinticConfiguration_t quinticConfig[];
extern imuConfiguration_t imuConfig[];
extern fabricSenseConfig_t fsConfig;
extern drv_uart_config_t uart0Config;
extern drv_uart_config_t uart1Config;
extern drv_uart_config_t usart0Config;
extern drv_uart_config_t usart1Config;
extern brainSettings_t brainSettings;
extern nvmSettings_t nvmSettings;
extern char dataLogFileName[];

volatile bool enableRecording = false;
volatile bool bluetoothConnected = false;
volatile bool debugPrintsEnabled = false; 
char stringBuf[50*10] = {0}; 
commandProcConfig_t* config = NULL;	

//static function forward declarations	
static void printStats();
static status_t processCommand(char* command, size_t cmdSize); 
static void setTimeFromString(char* dateTime); 
static char* getTimeString();

drv_uart_config_t uartInitialization =
{
	.p_usart = UART0,
	.mem_index = 0,
	.uart_options =
	{
		.baudrate   = 9600,
		.charlength = US_MR_CHRL_8_BIT,
		.paritytype = UART_MR_PAR_NO,
		.stopbits   = US_MR_NBSTOP_1_BIT
	}
};	
/**
 * \brief This task, when started will loop back \r\n terminated strings
 */
void task_commandHandler(void *pvParameters)
{
	UNUSED(pvParameters);
	int result = 0;
	char buffer[100] = {0};
	int pointer = 0;
	config = (commandProcConfig_t*)pvParameters; 
	//configure the bluetooth dongle, if brand new
	//TODO add a check to make sure it needs to be reconfigured first. 
	//drv_uart_deInit(config->uart);
	//drv_uart_init(&uartInitialization);
	//drv_uart_putString(&uartInitialization,"AT+NAMEHEDDOKO01\r\n");
	//vTaskDelay(100);
	//drv_uart_putString(&uartInitialization,"AT+BAUD8\r\n");
	//vTaskDelay(100);
	//drv_uart_init(config->uart);
	
	//char val = 0xA5; 
	while(1)
	{
		if(drv_uart_getline(config->uart,buffer,sizeof(buffer)) == STATUS_PASS)
		{
			//drv_uart_putString(&uart1Config,buffer); 
			processCommand(buffer,sizeof(buffer)); 
		}		
		vTaskDelay(10); 
	}
}



/***********************************************************************************************
 * printStats()
 * @brief Prints the Stats of the system to serial terminal
 * @param 
 * @return 
 ***********************************************************************************************/
static void printStats()
{
	int i = 0; 
	size_t numberOfImus = 9;//sizeof(imuConfig) / sizeof(imuConfiguration_t);
	size_t numberOfQuintics = 3;//sizeof(quinticConfig) / sizeof(quinticConfiguration_t); 	
	printString("QUINTIC STATS \r\n");
	for(i = 0; i < numberOfQuintics; i++)
	{
		printf("Q%d:\r\n", i);
		printf("	Corrupt Packets: %d\r\n", quinticConfig[i].corruptPacketCnt);
		printf("	Dropped Bytes:   %d\r\n", drv_uart_getDroppedBytes(quinticConfig[i].uartDevice));
		vTaskDelay(1);
	}
	printString("IMU STATS \r\n");
	for(i = 0; i < numberOfImus; i++)
	{		
		printf("IMU%d:\r\n", imuConfig[i].imuId);
		printf("	IMU Present: %d\r\n", imuConfig[i].imuPresent);
		printf("	IMU Connected: %d\r\n", imuConfig[i].imuConnected);
		vTaskDelay(1);
		printf("	Dropped Packets: %d\r\n", imuConfig[i].stats.droppedPackets);
		printf("	Average Rx interval(ticks): %d\r\n",imuConfig[i].stats.avgPacketTime);
		printf("	Packet Rx Count:   %d\r\n", imuConfig[i].stats.packetCnt);
		vTaskDelay(1);
	}
	printf("Total Bytes Written: %d\r\n", totalBytesWritten); 
	vTaskDelay(1);
	printf("Total Frames Written: %d \r\n", totalFramesWritten); 
	int queuedMessages = uxQueueMessagesWaiting(queue_dataHandler);
	printf("Queued Messages: %d\r\n", queuedMessages); 
	printf("--- task ## %u", (unsigned int)uxTaskGetNumberOfTasks());	
	vTaskList((signed portCHAR *)stringBuf);
	printf(stringBuf);
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
		task_stateMachine_EnqueueEvent(SYS_EVENT_RECORD_SWITCH,0);
		printString( "ACK\r\n");
	}
	else if(strncmp(command, "Reset\r\n",cmdSize) == 0)
	{
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_SWITCH,0);
		printString( "ACK\r\n");
	}
	else if(strncmp(command, "Power\r\n",cmdSize) == 0)
	{
		task_stateMachine_EnqueueEvent(SYS_EVENT_POWER_SWITCH,0);
		printString( "ACK\r\n");
	}
	else if(strncmp(command, "GetState\r\n",cmdSize) == 0)
	{
		switch(getCurrentState())
		{
			case SYS_STATE_IDLE: 
			printString("Idle\r\n"); 
			break; 
			case SYS_STATE_RESET:
			printString("Reset\r\n");
			break;
			//get accel data is part of the recording state.
			case SYS_STATE_GET_ACCEL_DATA:
			case SYS_STATE_RECORDING:
			printString("Recording\r\n");
			break;
			case SYS_STATE_ERROR:
			printString("Error\r\n");
			break;
			default: 
			printString("UnDef\r\n");
			break;			
		}
	}
	else if(strncmp(command, "AutoOff",7) == 0)
	{
		if(*(command+7) == '1')
		{
			brainSettings.autoTurnOff = true; 
		}
		else
		{
			brainSettings.autoTurnOff = false; 
		}
		printString( "ACK\r\n");
	}	
	else if(strncmp(command, "SetSerial",9) == 0)
	{
		//check that the size makes sense
		cmdSize = strlen(command);
		if((cmdSize -9) > 3)
		{
			//get rid of the \r\n
			command[cmdSize-2] = NULL;
			strncpy(nvmSettings.suitNumber, command+9, 7);	//restrict the suit number size to 6
			if(saveNvmSettings() == STATUS_PASS)
			{
				printString("ACK\r\n");
				return;
			}
		}
		printString( "NACK\r\n");
	}
	else if (strncmp(command, "GetSerial",9) == 0)
	{
		printString(nvmSettings.suitNumber);
		printString("\r\nACK\r\n");		
	}
	else if (strncmp(command, "EnableCSV", 9) == 0)
	{
		cmdSize = strlen(command);
		if ((cmdSize - 9) >= 1)
		{
			command[cmdSize - 2] = NULL;
			if (command[9] == '0')
			{
				nvmSettings.enableCsvFormat = 0;
				if(saveNvmSettings() == STATUS_PASS)
				{
					printString("ACK\r\n");
					return;
				}
			}
			else if(command[9] == '1')
			{
				nvmSettings.enableCsvFormat = 1;
				if(saveNvmSettings() == STATUS_PASS)
				{
					printString("ACK\r\n");
					return;
				}
			}
		}
		printString( "NACK\r\n");
	}
	else if (strncmp(command, "SetRecordName",13) == 0)
	{
		//check if size makes sense
		cmdSize = strlen(command);
		if (((cmdSize - 13) < MAX_FILE_NAME_LENGTH) && ((cmdSize - 13) > 0))
		{
			//get rid of the \r\n
			command[cmdSize - 2] = NULL;
			memset(brainSettings.fileName, NULL, sizeof(brainSettings.fileName));	//clear the existing name first
			strncpy(brainSettings.fileName, (command + 13), (cmdSize - 13));	//write the new name to settings file
			printString("ACK\r\n");
			return;
		}
		printString("NACK\r\n");
	}
	else if (strncmp(command, "GetRecordName",13) == 0)
	{
		snprintf(stringBuf, sizeof(stringBuf), "%s\r\n",dataLogFileName);
		printString(stringBuf);
	}
	else if(strncmp(command, "DebugEn",7) == 0)
	{
		if(*(command+7) == '1')
		{
			brainSettings.debugPrintsEnabled = true; 
		}
		else
		{
			brainSettings.debugPrintsEnabled = false; 
		}
		printString( "ACK\r\n");
	}				
	else if (strncmp(command, "CheckRssi\r\n",cmdSize) == 0)
	{
		printString("RSSI level:\r\n");
		task_quintic_checkRssiLevel(&quinticConfig[0]);
		task_quintic_checkRssiLevel(&quinticConfig[1]);
		task_quintic_checkRssiLevel(&quinticConfig[2]);
	}	
	else if(strncmp(command, "rstBLE\r\n",cmdSize) == 0)
	{
		drv_gpio_setPinState(quinticConfig[0].resetPin, DRV_GPIO_PIN_STATE_LOW);
		drv_gpio_setPinState(quinticConfig[2].resetPin, DRV_GPIO_PIN_STATE_LOW);
		vTaskDelay(50);
		drv_gpio_setPinState(quinticConfig[0].resetPin, DRV_GPIO_PIN_STATE_HIGH);
		drv_gpio_setPinState(quinticConfig[2].resetPin, DRV_GPIO_PIN_STATE_HIGH);
		printString("Pin reset\r\n");
		enableRecording = false;
	}
		
	else if(strncmp(command, "disableUARTs\r\n",cmdSize) == 0)
	{
		drv_uart_deInit(&uart1Config);
		drv_uart_deInit(&usart0Config);
		drv_uart_deInit(&usart1Config);
		drv_gpio_ConfigureBLEForProgramming(); 
		printString("UARTs set as High impedance\r\n");
		enableRecording = false;
	}	
	else if(strncmp(command, "setTime",7) == 0)
	{
		if(strlen(command) >= 24)
		{
			setTimeFromString(command+7); 
		}
	}
	else if(strncmp(command, "?\r\n",3) == 0)
	{
		printf("HEDDOKO BRAIN PACK %s\r\n", VERSION); 
		printf("BUILD DATE: %s %s\r\n", __DATE__,__TIME__);
		printf("SUIT NUMBER: %s\r\n",brainSettings.suitNumber); 
		printf("SUIT TIME: %s \r\n",getTimeString());  		
	}
	else if(strncmp(command, "flushUarts\r\n",cmdSize) == 0)
	{
		drv_uart_flushRx(&usart1Config);
		drv_uart_flushRx(&uart0Config);
		drv_uart_flushRx(&usart0Config);
	}
	else if(strncmp(command,"getStats\r\n", cmdSize) == 0)
	{
		printStats(); 
	}	
	else if(strncmp(command,"HardReset\r\n", cmdSize) == 0)	
	{
		rstc_start_software_reset(RSTC);
	}
	else if (strncmp(command,"debugPackets\r\n", cmdSize) == 0)
	{
		//turns on the zeroing of missing packets in the output data. 
		brainSettings.debugPackets = true; 
	}
	else if (strncmp(command,"chMap ", 6) == 0)
	{
		// Save the received channel map to settings.
		strncpy(brainSettings.channelmap, command+6, 10);
		strncpy((brainSettings.channelmap)+10, "\r\n", 2);	//Add CR+LF at the end of the srting
		printString("Received new channel map request:\r\n");
		printString(brainSettings.channelmap);
	}
	else if (strncmp(command, "setLedConfig", 12) == 0)
	{
		if (*(command + 12) == '0')
		{
			nvmSettings.ledConfiguration.redLed = DRV_GPIO_PIN_RED_LED;
			nvmSettings.ledConfiguration.greenLed = DRV_GPIO_PIN_GREEN_LED;
			nvmSettings.ledConfiguration.blueLed = DRV_GPIO_PIN_BLUE_LED;
		}
		else
		{
			nvmSettings.ledConfiguration.redLed = DRV_GPIO_PIN_BLUE_LED;
			nvmSettings.ledConfiguration.greenLed = DRV_GPIO_PIN_GREEN_LED;
			nvmSettings.ledConfiguration.blueLed = DRV_GPIO_PIN_RED_LED;
		}
		if(saveNvmSettings() == STATUS_PASS)
		{
			printString("ACK\r\n");
			return;
		}
		printString("NACK\r\n");
	}
	else if (strncmp(command, "getSdCdValue", 12) == 0)
	{
		drv_gpio_pin_state_t sdCdPinState;
		drv_gpio_getPinState(DRV_GPIO_PIN_SD_CD, &sdCdPinState);
		if (sdCdPinState == DRV_GPIO_PIN_STATE_HIGH)
		{
			printString("High\r\n");
		}
		else if (sdCdPinState == DRV_GPIO_PIN_STATE_LOW)
		{
			printString("Low\r\n");
		}
	}
	else if (strncmp(command, "LowBattery", 10) == 0)
	{
		//this message is sent from the power board to indicate that the battery is low
		debugPrintString("Low Battery\r\n");		
		task_stateMachine_EnqueueEvent(SYS_EVENT_LOW_BATTERY,0);		
	}
	else if(strncmp(command, "PwrBrdMsg:", 10) == 0)
	{
		//do nothing, the message will be logged to file below. 
	}	
	else
	{		
		debugPrintString("Received invalid command\r\n");
		status = STATUS_PASS; 
	}
	snprintf(stringBuf,50*10,"Received Command: %s \r\n",command);
	debugPrintString(stringBuf);
	return status;	
}

static void setTimeFromString(char* dateTime)
{
	uint32_t year, month, day, dow; //dow is day of week (1-7)
	uint32_t hour, minute, second; 
	if(sscanf(dateTime,"%d-%d-%d-%d-%d:%d:%d\r\n", &year, &month, &day, &dow, &hour, &minute, &second ) == 7)
	{
		// we successfully parsed the data, set the time and date
		rtc_set_time(RTC,hour,minute,second); 
		rtc_set_date(RTC,year,month,day,dow); 
		printString("ACK\r\n");
	}
	else
	{
		printString("NACK\r\n");
	}
}
char timeString[100] = {0}; 
static char* getTimeString()
{
	uint32_t hour, minute, second; 
	rtc_get_time(RTC,&hour,&minute,&second); 
	sprintf(timeString,"%02d:%02d:%02d",hour,minute,second); 
	return timeString; 
} 

void __attribute__((optimize("O0"))) debugPrintStringInt(char* str, int number)
{
	size_t length = 0;
	char timeStampedStr[MAX_DEBUG_STRING_LENGTH];
	int len = itoa(sgSysTickCount, timeStampedStr, 10);
	timeStampedStr[len++] = ',';
	length = len;
	len = itoa(number, timeStampedStr+length, 10);
	timeStampedStr[length+len] = ',';
	length += len + 1;
	strncpy(timeStampedStr+length, str, MAX_DEBUG_STRING_LENGTH-length);
	length = strlen(timeStampedStr);	
	//length = snprintf(timeStampedStr,200,"%08d,%s %d\r\n",sgSysTickCount,str,number);
	if(length > 0)
	{
		if(brainSettings.debugPrintsEnabled)
		{
			if(config != NULL)
			{		
				drv_uart_putData((config->uart), timeStampedStr, length);
			}
		}
		task_debugLogWriteEntry(timeStampedStr, length);
	}
}

void __attribute__((optimize("O0"))) debugPrintString(char* str)
{
	size_t length = 0;
	char timeStampedStr[MAX_DEBUG_STRING_LENGTH];
	int len = itoa(sgSysTickCount, timeStampedStr, 10);
	timeStampedStr[len++] = ',';
	strncpy(timeStampedStr+len, str, MAX_DEBUG_STRING_LENGTH-len);	
	length = strlen(timeStampedStr); 
	if(length > 0)
	{
		if(brainSettings.debugPrintsEnabled)
		{
			if(config != NULL)
			{
				drv_uart_putData((config->uart), timeStampedStr, length);
			}
		}
		task_debugLogWriteEntry(timeStampedStr, length);
	}
}

void printString(char* str)
{
	drv_uart_putString((config->uart), str);
}

void sendPacket(char* buf, size_t length)
{
	drv_uart_putData((config->uart), buf, length);
}