/*
 * task_fabricSense.c
 *
 * Created: 10/5/2015 9:19:27 AM
 *  Author: Sean Cloghesy
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 

#include "task_fabricSense.h"
#include "task_dataProcessor.h"
#include "task_stateMachine.h"
#include "task_quinticInterface.h"
#include "task_commandProc.h"
#include "drv_uart.h"
#include <string.h>

extern xQueueHandle queue_dataHandler;
extern uint32_t sgSysTickCount;
extern bool enableRecording; 
extern uint16_t packetReceivedMask; 
extern drv_uart_config_t uart0Config;
//static function foward declarations

void createDummyFabSensePacket(char* dataPacket, size_t maxPacketSize, uint32_t seqNum);

/***********************************************************************************************
 * task_fabSenseHandler(void *pvParameters)
 * @brief The main task for a fabric sense module, associated UART has to be initialized before calling this
 *	function. 
 * @param pvParameters, void pointer to structure containing fabric sense configuration. 
 * @return void
 ***********************************************************************************************/
void task_fabSenseHandler(void *pvParameters)
{
	//cast void pointer to fabric sense config type.
	fabricSenseConfig_t *fsConfig = (fabricSenseConfig_t*)pvParameters;
	//check that uart device is initialized
	if(drv_uart_isInit(fsConfig->uartDevice) != STATUS_PASS)
	{
		//the UART driver has not been initialized, this should be done before starting this task!
		debugPrintString("FATAL: uart not initialized for FS\r\n");
		//spin forever, this is a fatal error.  
		while(1)
		{
			vTaskDelay(1);
		}
		return;
	}
	//initialize the module, send configuration values. 
	//if(task_fabSense_init(fsConfig) != STATUS_PASS)
	//{
		////this is an error, we should do something here!
		//return; 
	//} 
	dataPacket_t packet;
	packet.type = DATA_PACKET_TYPE_SS;
	//main loop of task, this is where we request information and store it.
	char buf[FS_RESPONSE_BUF_SIZE] = {0};
	uint8_t numPacketsReceived = 0; 
	uint8_t dataSize = 0;
	uint32_t sequenceNumber = 0;
	uint32_t packetsReceived = 0; 
	packetReceivedMask |= 1<<9; //add mask for fabric sense
	while(1)
	{
		if(enableRecording)
		{		
			#ifdef CREATE_DUMMY_PACKETS
			vTaskDelay(20); 
			createDummyFabSensePacket(buf, FS_RESPONSE_BUF_SIZE, sequenceNumber++); 
			memcpy(packet.data, buf, 23);
			//enqueue the packet for the data processor.
			if(queue_dataHandler != NULL)
			{
				if(xQueueSendToBack( queue_dataHandler,( void * ) &packet,10 ) != TRUE)
				{					
					vTaskDelay(1);
				}
			}
			#else
			if (getCurrentState() != SYS_STATE_RESET)		//Change to SYS_STATE_RESET after complete merge
			{
				if (drv_uart_getlineTimedSized(fsConfig->uartDevice, buf, CMD_RESPONSE_BUF_SIZE, 400, &dataSize) == STATUS_PASS)
				{
					if(strncmp(buf, "!", 1) == 0)
					{
						if (dataSize != 31)
						{
							//the size does not match the expected size, package is corrupt
						}
						else
						{
							//copy the data to the packet. 
							memcpy(packet.data, buf+1, 20);
						}
					}
					else if (strncmp(buf, "@", 1) == 0)
					{
						debugPrintString("Received command feedback from FabSense\r\n");
						//do nothing the message should just be consumed
					}
					
					//enqueue the packet for the data processor.
					if(queue_dataHandler != NULL)
					{
						if(xQueueSendToBack( queue_dataHandler,( void * ) &packet,10 ) != TRUE)
						{
							vTaskDelay(1);
						}
					}					
				}				
			}
			#endif
	
		}
		else
		{
			vTaskDelay(10); 
		}
		
	}
}
/***********************************************************************************************
 * task_fabSense_init(fabricSenseConfig_t* fabSenseConfig)
 * @brief Function called to initialize the fabric sense module parameters
 * @param pvParameters, void pointer to structure containing fabric sense configuration. 
 * @return void
 ***********************************************************************************************/
status_t task_fabSense_init(fabricSenseConfig_t* fabSenseConfig)
{
	status_t result = STATUS_FAIL;
	char buf[150] = {0};
	char* bufPtr = buf; 
	
	#ifdef CREATE_DUMMY_PACKETS
	return STATUS_PASS;
	#else
	uint8_t initTryCount = 3;
	uint32_t vSamplePeriod = 0, vNumAvg = 0;
	char vCommandStr[20] = {0}, vCompareStr[20] = {0};
		
	drv_uart_getlineTimed(fabSenseConfig->uartDevice, buf, sizeof(buf), 1500);	//consume any garbage data present on the bus
	drv_uart_flushRx(fabSenseConfig->uartDevice);	//flush the uart first
	
	snprintf(vCommandStr, sizeof(vCommandStr), "#p %d\r\n", fabSenseConfig->samplePeriod_ms);
	snprintf(vCompareStr, sizeof(vCompareStr), "%d msec", fabSenseConfig->samplePeriod_ms);
	// Send the command to change the time interval to 20ms. Do it max 3 times.
	while (initTryCount != 0)
	{
		drv_uart_putString(fabSenseConfig->uartDevice, vCommandStr);
		vTaskDelay(1);
		if (drv_uart_getlineTimed(fabSenseConfig->uartDevice, buf, sizeof(buf), 1500) == STATUS_PASS)
		{
			debugPrintString(buf);
			if (strncmp(buf, "@Sample", 7) == 0)
			{
				bufPtr = buf + 18;
				if (strncmp(bufPtr, vCompareStr, 7) == 0)
				{
					//result = STATUS_PASS;
					break;
				}
				else
				{
					result = STATUS_FAIL;
					return result;
				}
			}
		}
		initTryCount--;
	}
	
	initTryCount = 3;
	snprintf(vCommandStr, sizeof(vCommandStr), "#f %d\r\n", fabSenseConfig->numAverages);
	snprintf(vCompareStr, sizeof(vCompareStr), "%dsamples", fabSenseConfig->numAverages);
	// Send the command to change the time interval to 20ms. Do it max 3 times.
	while (initTryCount != 0)
	{
		drv_uart_putString(fabSenseConfig->uartDevice, vCommandStr);
		vTaskDelay(1);
		if (drv_uart_getlineTimed(fabSenseConfig->uartDevice, buf, sizeof(buf), 1500) == STATUS_PASS)
		{
			debugPrintString(buf);
			if (strncmp(buf, "@N-Filter", 9) == 0)
			{
				bufPtr = buf + 14;
				if (strncmp(bufPtr, vCompareStr, 8) == 0)
				{
					result = STATUS_PASS;
					break;
				}
			}
		}
		initTryCount--;
	}
	#endif
	if(result != STATUS_PASS)
	{
		debugPrintString("Init SS failed!\r\n");	
	}
	
	return result;
}
/***********************************************************************************************
 * task_fabSense_start(fabricSenseConfig_t* fabSenseConfig)
 * @brief Function called to start the fabrix sense transmission of data. 
 * @param pvParameters, void pointer to structure containing fabric sense configuration. 
 * @return void
 ***********************************************************************************************/
status_t task_fabSense_start(fabricSenseConfig_t* fabSenseConfig)
{
	
	status_t status = STATUS_PASS; 
	packetReceivedMask |= 1<<9;
	#ifndef CREATE_DUMMY_PACKETS
	drv_uart_putString(fabSenseConfig->uartDevice, "#s\r\n");
	#endif
	enableRecording = true; 
	return status; 
}
/***********************************************************************************************
 * task_fabSense_stop(fabricSenseConfig_t* fabSenseConfig)
 * @brief Function called to stop the fabric sense module  
 * @param pvParameters, void pointer to structure containing fabric sense configuration. 
 * @return void
 ***********************************************************************************************/
status_t task_fabSense_stop(fabricSenseConfig_t* fabSenseConfig)
{	
	status_t status = STATUS_PASS;
	#ifndef CREATE_DUMMY_PACKETS
	drv_uart_putString(fabSenseConfig->uartDevice, "#t\r\n");
	#endif	
	enableRecording = false; 
	return status;	
}


//static functions


void createDummyFabSensePacket(char* dataPacket, size_t maxPacketSize, uint32_t seqNum)
{
	//int i = 0; 
	//snprintf(dataPacket, maxPacketSize, "%04dBBBBCCCCDDDDEEEE%08d\r\n",seqNum%10000, sgSysTickCount); 
	strcpy(dataPacket, "1234BBBBCCCCDDDDEEEE\r\n");
}