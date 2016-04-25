/*
 * task_dataProcessor.c
 *
 * Created: 9/28/2015 11:18:25 AM
 * Copyright Heddoko(TM) 2015, all rights reserved
 *  Author: Sean Cloghesy
 */ 
#include <asf.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "task_dataProcessor.h"
#include "task_quinticInterface.h"
#include "task_stateMachine.h"
#include "task_commandProc.h"
#include "task_sdCardWrite.h"
#include "task_emInterface.h"
#include "settings.h"
#include "drv_uart.h"
#include "drv_led.h"

#define NUMBER_OF_SENSORS (NO_OF_IMUS_FOR_PACKET + NO_OF_FAB_SENSE)

//extern definitions
extern imuConfiguration_t imuConfig[];
extern drv_uart_config_t uart1Config;
extern uint32_t sgSysTickCount;
extern quinticConfiguration_t quinticConfig[]; 
extern brainSettings_t brainSettings;
xQueueHandle queue_dataHandler = NULL;
xTimerHandle frameTimeOutTimer;
uint8_t vframeTimeOutFlag = 0;
bool sentFirstFrame = FALSE, buttonEvent = FALSE;
bool sentReconnectToQuintics = FALSE;

dataPacket_t packetBuffer[NUMBER_OF_SENSORS]; //store 10 packets, one for each sensor
uint16_t missingSensorPacketCounts[NUMBER_OF_SENSORS]; //store the number of missing counts for each IMU, so we can send a re-connect
uint16_t packetReceivedFlags = 0x0000;  //a flag for each slot, when all are 1 then process it.
volatile uint16_t packetReceivedMask = 0x000;	//0x01FF; //this mask of which flags have to be set to save the files to disk.
volatile uint16_t accelPacketReceivedMask = 0x000;
uint16_t accelPacketReceivedFlags = 0x0000;
volatile uint32_t totalFramesWritten = 0; //the total bytes written to the file
volatile uint32_t accelFramesToWrite = 0; //the total frames of acceleration data captured. 

#define MAX_BUFFERED_DATA_FRAMES 3

extern xSemaphoreHandle semaphore_sdCardWrite;

//static function declarations
static status_t processPackets(); 

void vframeTimeOutTimerCallback()
{
	vframeTimeOutFlag = 1;
}

/***********************************************************************************************
 * task_dataHandler(void *pvParameters)
 * @brief Handles the incoming data from IMUs
 * @param void *pvParameters
 * @return 
 ***********************************************************************************************/
void task_dataHandler(void *pvParameters)
{

	//setup the queue
	queue_dataHandler = xQueueCreate( 50, sizeof(dataPacket_t));
	if(queue_dataHandler == 0)
	{
		// Queue was not created this is an error!
		debugPrintString("an error has occurred, data handler queue failure\r\n"); 
		return; 
	}
	int timerId = 0;

	frameTimeOutTimer = xTimerCreate("Frame Time Out Timer", (PACKET_WAIT_TIMEOUT/portTICK_RATE_MS), pdFALSE, NULL, vframeTimeOutTimerCallback);
	if (frameTimeOutTimer == NULL)
	{
		debugPrintString("Failed to create timer task\r\n");
	}
	
	
	//open file to read. 
	dataPacket_t packet; 
	int i =0; 
	//int index = dataFrameTail; 
	while(1)
	{		
		if(xQueueReceive( queue_dataHandler, &( packet ), 1000) == TRUE)
		{			
			//handle packet
			if(packet.type == DATA_PACKET_TYPE_IMU && accelFramesToWrite == 0)
			{
				if(packetReceivedFlags & (uint16_t)(1 << packet.imuId))
				{
					//we've already received data for this sensor, copy over it...
						memcpy(&packetBuffer[packet.imuId],&packet, sizeof(dataPacket_t));
						imuConfig[packet.imuIndex].stats.droppedPackets++; //we're dropping a packet. 	TODO must fix this case	
				}
				else
				{
					//make sure the index is alright. 
					if(packet.imuId < NUMBER_OF_SENSORS)
					{
						memcpy(&packetBuffer[packet.imuId],&packet, sizeof(dataPacket_t));
						packetReceivedFlags |= (1 << packet.imuId); //set flag
					}
				}	
			}
			else if(packet.type == DATA_PACKET_TYPE_ACCEL && accelFramesToWrite > 0)
			{
				memcpy(&packetBuffer[packet.imuId],&packet, sizeof(dataPacket_t));
				accelPacketReceivedFlags |= (1 << packet.imuId); //set flag								
			}
			
			else if(packet.type == DATA_PACKET_TYPE_SS && accelFramesToWrite == 0)
			{
				if(packetReceivedFlags & (uint16_t)(1 << NUMBER_OF_SENSORS -1))
				{
					//we've already received data for this sensor, process all the bytes as is.	
					memcpy(&packetBuffer[NUMBER_OF_SENSORS -1],&packet, sizeof(dataPacket_t));				
				}
				else
				{
					//FabSense data always stored at end of buffer
					memcpy(&packetBuffer[NUMBER_OF_SENSORS -1],&packet, sizeof(dataPacket_t));
					packetReceivedFlags |= (1 << NUMBER_OF_SENSORS -1); //set flag
				}				
			}
			
			else if (packet.type == DATA_PACKET_TYPE_BUTTON && getCurrentState() == SYS_STATE_RECORDING)	//process the button event only while recording
			{
				if (sentFirstFrame == TRUE)
				{
					buttonEvent = TRUE;
				}
			}
			
			if((packetReceivedFlags == packetReceivedMask) || (vframeTimeOutFlag == 1))
			{
				
				if(vframeTimeOutFlag == 1)
				{
					//since this is an incomplete frame, tally the total lost frames count
					for(i=0;i<NUMBER_OF_SENSORS;i++)
					{
						if(((packetReceivedFlags >> i) & 0x0001) == 0)
						{
							missingSensorPacketCounts[i]++;
							if(missingSensorPacketCounts[i] > PACKET_LOSS_COUNT_FOR_RECONNECT)
							{
								//send the connect command. 								
								if(sentReconnectToQuintics == FALSE)
								{							
									sentReconnectToQuintics = TRUE;
								}
								if (missingSensorPacketCounts[i] >= PACKET_LOSS_COUNT_FOR_ERROR)	//if sensor stays disconnected for more than 100 frames
								{
									missingSensorPacketCounts[i] = 0;
									debugPrintStringInt("Connection try out\r\n", i);
									task_stateMachine_EnqueueEvent(SYS_EVENT_IMU_DISCONNECT, 0x00);	//Send IMU_DISCONNECT event
								}
							}	
						}
						else
						{
							missingSensorPacketCounts[i] = 0;
						}						
					}					
				}
 				vframeTimeOutFlag = 0; 	
 				xTimerReset(frameTimeOutTimer, 0);
 				if (packetReceivedFlags == packetReceivedMask)
 				{
					//set all the missing packet counts to zero. 
					memset(missingSensorPacketCounts, 0,sizeof(missingSensorPacketCounts)); 					
					if(sentFirstFrame == FALSE)
					{
						drv_led_set(DRV_LED_RED, DRV_LED_SOLID);	
						sentFirstFrame = TRUE;
					}	 				
					sentReconnectToQuintics = FALSE; 
 				}
 				if (sentFirstFrame == TRUE)
 				{
					processPackets();
					//clear flag at the end
					packetReceivedFlags = 0x0000;
				}				
			}				
			//if we're currently writing 
			if(accelFramesToWrite > 0)
			{
				if(accelPacketReceivedFlags == accelPacketReceivedMask)
				{
					//set the last packet to be stretchsense. 
					packetBuffer[NUMBER_OF_SENSORS -1].type = DATA_PACKET_TYPE_SS; 
					processPackets(); 
					//clear flag at the end
					accelPacketReceivedFlags = 0x0000;
					accelFramesToWrite--;
					if(accelFramesToWrite == 0)
					{
						task_stateMachine_EnqueueEvent(SYS_EVENT_GET_ACCEL_DATA_COMPLETE,0x00); 
					}
				}
			}						
		}		
		vTaskDelay(1);
	}
	
}

void task_dataProcessor_startGetAccelData(uint32_t numberOfFrames)
{
	//this function resets the flags and clears the memory buffers before a recording.  
	sentFirstFrame = FALSE; //the system will only start streaming once all the sensor have checked in.
	memset(packetBuffer, 0x00, sizeof(packetBuffer));	
	packetBuffer[NUMBER_OF_SENSORS -1].type = DATA_PACKET_TYPE_SS; 
	accelPacketReceivedFlags = 0x0000;
	packetReceivedFlags = 0x0000; // Set it to 0x0000 to indicate its an acceleration frame. 
	memset(missingSensorPacketCounts, 0,sizeof(missingSensorPacketCounts));
	accelFramesToWrite = numberOfFrames;
	xTimerStop(frameTimeOutTimer, 0);
	vframeTimeOutFlag = 0;
}

void task_dataProcessor_startRecording()
{
	//this function resets the flags and clears the memory buffers before a recording. 
	sentFirstFrame = FALSE; //the system will only start streaming once all the sensor have checked in. 
	memset(packetBuffer, 0x00, sizeof(packetBuffer));
	packetReceivedFlags = 0x0000;
	memset(missingSensorPacketCounts, 0,sizeof(missingSensorPacketCounts)); 
	xTimerReset(frameTimeOutTimer, 0);
	vframeTimeOutFlag = 0;
	accelFramesToWrite = 0; //make sure the accelFramesToWrite is zero. 
}

#define NUMBER_OF_PACKETS_PER_MESSAGE 10
#define IMU_PACKET_LENGTH 3  //in number of data sizes. 
#define IMU_PACKET_DATA_SIZE 4 

#define FS_PACKET_LENGTH 5  //There are 5 channels on the fabric sense
#define FS_PACKET_DATA_SIZE 4 //each data point is 4 characters
char entryBuffer[200] = {0};

/***********************************************************************************************
 * processPackets()
 * @brief process the incoming packets and create a full frame. 
 * @param 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error 
 ***********************************************************************************************/	
static status_t processPackets()
{
	status_t status = STATUS_PASS; 
	int i, j, k;	  
	int entryBufferIdx = 0; 
	int res = 0; 
	int numberBytes = 0; 
	//char* entryBuffer = NULL; 	
	
	entryBufferIdx = snprintf(entryBuffer, 17 ,"%010d,%04x,", sgSysTickCount,packetReceivedFlags);
	for(i = 0; i < 10; i++) //sensor reading
	{
		//if imu or acceleration process this way, 
		if(i < NUMBER_OF_SENSORS -1)
		{			
			for(j=0; j < IMU_PACKET_LENGTH; j++) //reading value
			{
				for(k = 0; k < IMU_PACKET_DATA_SIZE; k++) //reading bytes
				{				
					//copy the asci data to the entry buffer
					if(packetBuffer[i].data != NULL)
					{
						entryBuffer[entryBufferIdx++] = packetBuffer[i].data[(j*4)+k];	
					}
					else
					{
						entryBuffer[entryBufferIdx++] = 0;
					}
						
					if(entryBuffer[entryBufferIdx-1] == 0)
					{
						//if that entry is empty/NULL, fill it in with '0's
						entryBuffer[entryBufferIdx-1] = '0'; 
					}
				}
				if(j != IMU_PACKET_LENGTH-1)
				{
					entryBuffer[entryBufferIdx++] = ';';	
				}			
			}
		}
		else //if stretch sense data, add it differently.
		{
			for(j=0; j < FS_PACKET_LENGTH; j++) //reading value
			{
				for(k = 0; k < FS_PACKET_DATA_SIZE; k++) //reading bytes
				{
					if(packetBuffer[i].data != NULL)
					{
						//copy the ascii data to the entry buffer
						entryBuffer[entryBufferIdx++] = packetBuffer[i].data[(j*4)+k];							
					}
					else
					{
						//there is no data, set to zero. 
						entryBuffer[entryBufferIdx++] = 0;
					}
					if(entryBuffer[entryBufferIdx-1] == 0)
					{
						//if that entry is empty/NULL, fill it in with '0's
						entryBuffer[entryBufferIdx-1] = '0';
					}
				}
				if(j != FS_PACKET_LENGTH-1)
				{
					entryBuffer[entryBufferIdx++] = ';';
				}
			}				
		}			
		 			
		entryBuffer[entryBufferIdx++] = ',';		
		
	}
	if(brainSettings.debugPackets)
	{
		memset(packetBuffer, 0x00, sizeof(packetBuffer));
	}	
	if (buttonEvent)
	{
		memcpy(entryBuffer+entryBufferIdx, "BTN,", 4);
		entryBufferIdx += 4;
		buttonEvent = FALSE;
	}
	entryBuffer[entryBufferIdx++] = '\r';
	entryBuffer[entryBufferIdx++] = '\n';
	entryBuffer[entryBufferIdx] = 0; //terminate the string
		
	sendPacket(entryBuffer,entryBufferIdx); //don't want to print the null
	totalFramesWritten++;	
	//write the entry to file
	task_sdCardWriteEntry(entryBuffer,entryBufferIdx);
	entryBufferIdx = 0; //reset pointer.		

	return status; 
}