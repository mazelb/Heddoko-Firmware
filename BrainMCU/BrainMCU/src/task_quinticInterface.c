/*
 * task_quinticInterface.c
 *
 * Created: 9/18/2015 10:49:35 AM
 *  Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved 
 */
/**
 * @file  task_quinticInterface.c
 * @brief handler task and associated functions for management of the Quintic modules
 * @author Heddoko
 * @note 
 *
 * @copy Heddoko (2015)
 */ 
#include "task_quinticInterface.h"
#include "task_dataProcessor.h"
#include "task_stateMachine.h"
#include "task_commandProc.h"
#include "task_main.h"
#include "settings.h"
#include <string.h>

//#define DEBUG_DUMMY_DATA 
extern xQueueHandle queue_dataHandler, queue_stateMachineEvents;
extern bool enableRecording; 
extern brainSettings_t brainSettings;
//static declarations
static status_t sendString(drv_uart_config_t* uartConfig, char* cmd);
static status_t getAck(drv_uart_config_t* uartConfig);
static status_t scanForImus(quinticConfiguration_t* qConfig); 
static status_t connectToImus(quinticConfiguration_t* qConfig);
static status_t getResponse(drv_uart_config_t* uartConfig, char* expectedResponse);
static void createDummyData(int imuId, int seqNumber, int numVals, char* buf, size_t bufSize);
static int getIndex(char c);
extern uint32_t sgSysTickCount;
extern drv_uart_config_t uart0Config;

/***********************************************************************************************
 * task_quinticHandler(void *pvParameters)
 * @brief The main task for a quintic module, associated UART has to be initialized before calling this
 *	function. 
 * @param pvParameters, void pointer to structure containing quintic and imu configuration. 
 * @return void
 ***********************************************************************************************/
void task_quinticHandler(void *pvParameters)
{
	//cast void pointer to quintic config type. 
	quinticConfiguration_t *qConfig = (quinticConfiguration_t*)pvParameters; 
	//initialize all structures ,check everything is alright
	int i = 0;
	//initialize all buffers and pointers to zero
	for(i = 0 ; i < QUINTIC_MAX_NUMBER_OF_IMUS ; i++)
	{
		//qConfig->imuArray[i]->bufferEnd = 0;
		//qConfig->imuArray[i]->bufferHead = 0;
		//memset(qConfig->imuArray[i]->packetBuffer, 0 , IMU_BUFFER_SIZE*IMU_PACKET_LENGTH); 	
		qConfig->imuArray[i]->stats.avgPacketTime = 0;
		qConfig->imuArray[i]->stats.droppedPackets = 0;
		qConfig->imuArray[i]->stats.lastPacketTime = sgSysTickCount;
		qConfig->imuArray[i]->stats.maxPacketTime = 0;
		qConfig->imuArray[i]->stats.packetCnt = 0;
	}
	
	if(drv_uart_isInit(qConfig->uartDevice) != STATUS_PASS)
	{
		//the UART driver has not been initialized, this should be done before starting this task!
		return; 
	}
	//cycle power on quintic module
	#ifdef DEBUG_DUMMY_DATA

	#else
	#endif 
	dataPacket_t packet; 
	packet.type = DATA_PACKET_TYPE_IMU; 
	//main loop of task, this is where we request information and store it. 
	char buf[CMD_RESPONSE_BUF_SIZE] = {0}; 
	int packetNumber = 0;
	uint32_t timeNow = 0;
	int index = -1; 
	qConfig->isinit = true;
	char debugString[50]; 
	while(1)
	{
		#ifndef DEBUG_DUMMY_DATA
		if(getCurrentState() != SYS_STATE_RESET)
		{		
			if(drv_uart_getlineTimed(qConfig->uartDevice, buf, CMD_RESPONSE_BUF_SIZE, 400) == STATUS_PASS)
			{
				index = -1; 
				if(buf[0] == '&') //euler data
				{
					index = getIndex(buf[1]); 
					packet.type = DATA_PACKET_TYPE_IMU; 
				}
				else if(buf[0] == '@') //acceleration data
				{
					index = getIndex(buf[1]);
					packet.type = DATA_PACKET_TYPE_ACCEL;  					
				}
				else if (strncmp(buf, "DiscResp", 8) == 0)
				{
					debugPrintStringInt("Disconnection event from Quintic\r\n", qConfig->qId);
					int i = 0;
					char* bufPtr = buf;
					if(strncmp(buf,"DiscResp", 8) == 0)
					{
						bufPtr = buf + 8;
						for (i=0; i<5; i++)
						{
							if (bufPtr[i] == '1')
							{
								qConfig->imuArray[i]->imuConnected = 1;
							}
							else
							{
								qConfig->imuArray[i]->imuConnected = 0;
							}
						}
					}
					//task_stateMachine_EnqueueEvent(SYS_EVENT_IMU_DISCONNECT, qConfig->qId);
					//send the connect string twice, try to get it to reconnect to the missing NOD. 
					sendString(qConfig->uartDevice, "connect\r\n");
				}
				else if (strncmp(buf, "ConnResp", 8) == 0)
				{
					debugPrintStringInt(buf,qConfig->qId);
				}
				else if(strncmp(buf, "AppStart\r\n",10) == 0)
				{
					//this means that the quintic has restarted, throw an error
					if(getCurrentState() == SYS_STATE_RECORDING || getCurrentState() == SYS_STATE_IDLE)
					{				
						debugPrintStringInt("Quintic Crashed!",qConfig->qId); 
						task_stateMachine_EnqueueEvent(SYS_EVENT_IMU_DISCONNECT, qConfig->qId);	
					}
				}
				else if ((strncmp(buf, "RSSI", 4) == 0))
				{
					debugPrintStringInt(buf,qConfig->qId);
				}
				else
				{				
					//this is a corrupt packet, increment the count. 
					qConfig->corruptPacketCnt++;
					//vTaskDelay(10);
				}
				//validate the index
				if((index >= 0 && index <= 4) && (index <= qConfig->expectedNumberOfNods))
				{
					packet.imuId = qConfig->imuArray[index]->imuId; 
					packet.imuIndex = packet.imuId; 
					qConfig->imuArray[index]->stats.packetCnt++; //increment the packets received count
					//disable the average packet time by default. 
					if(brainSettings.debugPackets == true)
					{						
						timeNow = sgSysTickCount; 
						//calculate the new running average packet time --> (average + (last packet received time - current Time)/2)
						qConfig->imuArray[index]->stats.avgPacketTime = (qConfig->imuArray[index]->stats.avgPacketTime + ( timeNow - qConfig->imuArray[index]->stats.lastPacketTime ))>>1;
						qConfig->imuArray[index]->stats.lastPacketTime = timeNow; 				
					}
					memcpy(packet.data,buf+2, 12+1);				
					if(queue_dataHandler != NULL)
					{
						if(xQueueSendToBack( queue_dataHandler,( void * ) &packet,5) != TRUE)
						{
							//error failed to queue the packet.
							qConfig->imuArray[index]->stats.droppedPackets++;
							debugPrintString("Queue Full Dropped packet\r\n");
						}													
					}				
				}
			}
		}
		//taskYIELD();
		vTaskDelay(1);
		#else

		#endif
	}	
}

/***********************************************************************************************
 * task_quintic_initializeImus(void *pvParameters)
 * @brief Start the initializing process for IMUs.
 * @param pvParameters, void pointer to structure containing quintic and imu configuration. 
 * @return void
 ***********************************************************************************************/
void task_quintic_initializeImus(void *pvParameters)
{
	quinticConfiguration_t *qConfig = (quinticConfiguration_t*)pvParameters;
	status_t result = STATUS_PASS;
	status_t scanSuccess=STATUS_FAIL, connSuccess=STATUS_FAIL;
	int vScanLoopCount=0;
	//reset the quintic here...
	drv_uart_flushRx(qConfig->uartDevice);	//flush the uart first
	drv_gpio_setPinState(qConfig->resetPin,DRV_GPIO_PIN_STATE_LOW);
	vTaskDelay(100);
	drv_gpio_setPinState(qConfig->resetPin,DRV_GPIO_PIN_STATE_HIGH);
	#ifndef USE_Q1_Q2
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_HIGH);
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_HIGH);
	toggleJackEnables(DRV_GPIO_PIN_STATE_HIGH);
	#endif
	vTaskDelay(100);
	#ifndef USE_Q1_Q2
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW);
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW);
	toggleJackEnables(DRV_GPIO_PIN_STATE_LOW);
	#endif
	vTaskDelay(100);
	//wait for first ACK
	result = getResponse(qConfig->uartDevice, "AppStart\r\n"); 
	drv_uart_flushRx(qConfig->uartDevice);	//flush the uart first
	vTaskDelay(10);
	//get quintic ready to receive the
	if (result != STATUS_PASS)
	{
		debugPrintString("Did not receive first ACK from Q\r\n");
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0xff);
		vTaskDelete(NULL);
		return;
	}
	
	sendString(qConfig->uartDevice,QCMD_BEGIN);
	vTaskDelay(10);
	result |= getAck(qConfig->uartDevice);
	if (result != STATUS_PASS)
	{
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0xff);
		vTaskDelete(NULL);
		return;
	}
	
	//send MAC addresses for each NOD	
	int i = 0;
	for(i=0;i<qConfig->expectedNumberOfNods; i++)
	{
		sendString(qConfig->uartDevice,qConfig->imuArray[i]->macAddress);
		vTaskDelay(10);
		result |= getAck(qConfig->uartDevice);
		if (result != STATUS_PASS)
		{
			task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0xff);
			vTaskDelete(NULL);
			return;
		}
	}
	sendString(qConfig->uartDevice,"end\r\n");
	vTaskDelay(10);
	result |= getAck(qConfig->uartDevice);
	if (result != STATUS_PASS)
	{
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0xff);
		vTaskDelete(NULL);
		return;
	}
	
	//send the latest channel mapping to quintics
	char buf[20] = {0};
	strncat(buf, "chmap ", 6);		//append the channel map command to the mask
	sendString(qConfig->uartDevice, strncat(buf, brainSettings.channelmap, 20-6));
	vTaskDelay(10);
	result |= getAck(qConfig->uartDevice);
	if (result != STATUS_PASS)
	{
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0xff);
		vTaskDelete(NULL);
		return;
	}
	//
	scanSuccess = scanForImus(qConfig);
	if(scanSuccess == STATUS_PASS)
	{
		connSuccess = connectToImus(qConfig);
	}
		
	
	//pass command to implement the new channel map
	//can only be passed after the connection has been established with the IMUs.
	sendString(qConfig->uartDevice, "setMap\r\n");
	
	if(scanSuccess == STATUS_PASS && connSuccess == STATUS_PASS)
	{		
		//printf("connected to IMUs %d, %d, %d\r\n",qConfig->imuArray[0]->imuId,qConfig->imuArray[1]->imuId,qConfig->imuArray[2]->imuId);
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_COMPLETE, qConfig->qId);			
	}
	else
	{
		//printf("Failed connection to IMUs %d, %d, %d\r\n",qConfig->imuArray[0]->imuId,qConfig->imuArray[1]->imuId,qConfig->imuArray[2]->imuId);
		//result = STATUS_FAIL; 
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0x00);
	}
	vTaskDelete(NULL);
	//return the result;
	//return result;

}
/***********************************************************************************************
 * task_quintic_sendConnectMsg(quinticConfiguration_t* qConfig)
 * @brief Send the connect command to the quintic modules
 * @param quinticConfiguration_t* qConfig 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t task_quintic_sendConnectMsg(quinticConfiguration_t* qConfig)
{
	//send the start command. 	
	if(qConfig->isinit == 1)
	{
		sendString(qConfig->uartDevice, "connect\r\n");	
	}	
	return STATUS_PASS; 
}

/***********************************************************************************************
 * task_quintic_startRecording(quinticConfiguration_t* qConfig)
 * @brief Send the start command to the IMUs to request Notif data from them
 * @param quinticConfiguration_t* qConfig 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t task_quintic_startRecording(quinticConfiguration_t* qConfig)
{
	//send the start command. 
	if ((qConfig->isinit) && (qConfig->expectedNumberOfNods > 0))
	{	
		sendString(qConfig->uartDevice, "start\r\n");
	}
	return STATUS_PASS; 
}

/***********************************************************************************************
 * task_quintic_stopRecording(quinticConfiguration_t* qConfig)
 * @brief Send the stop command to the IMUs to stop Notif data from them
 * @param quinticConfiguration_t* qConfig 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t task_quintic_stopRecording(quinticConfiguration_t* qConfig)
{
	//send the stop
	if ((qConfig->isinit) && (qConfig->expectedNumberOfNods > 0))
	{	
		sendString(qConfig->uartDevice, "stop\r\n");
		//wait for a bit
		vTaskDelay(2);
		//flush the Rx buffer, it'll still have crap in it
		drv_uart_flushRx(qConfig->uartDevice);
	}
	return STATUS_PASS;
}

/***********************************************************************************************
 * task_quintic_checkRssiLevel(quinticConfiguration_t* qConfig)
 * @brief Send the "rssi" command to check RSSI level of each of the connected IMUs
 * @param quinticConfiguration_t* qConfig 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t task_quintic_checkRssiLevel(quinticConfiguration_t* qConfig)
{
	if ((qConfig->isinit) && (qConfig->expectedNumberOfNods > 0))
	{	
		//send the rssi check command. 	
		sendString(qConfig->uartDevice, "rssi\r\n");
	}
	return STATUS_PASS; 
}

/***********************************************************************************************
 * task_quintic_startGetAccelData(quinticConfiguration_t* qConfig)
 * @brief Send the start command to the IMUs to request acceleration data
 * @param quinticConfiguration_t* qConfig 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t task_quintic_startGetAccelData(quinticConfiguration_t* qConfig)
{
	//send the start command. 
	if ((qConfig->isinit) && (qConfig->expectedNumberOfNods > 0))
	{	
		sendString(qConfig->uartDevice, "getAccel1\r\n");
		vTaskDelay(100); 
		sendString(qConfig->uartDevice, "start\r\n");
	}
	return STATUS_PASS; 
}
/***********************************************************************************************
 * task_quintic_stopGetAccelData(quinticConfiguration_t* qConfig)
 * @brief Send the start command to the IMUs to request acceleration data
 * @param quinticConfiguration_t* qConfig 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t task_quintic_stopGetAccelData(quinticConfiguration_t* qConfig)
{
	//send the start command. 
	if ((qConfig->isinit) && (qConfig->expectedNumberOfNods > 0))
	{	
		sendString(qConfig->uartDevice, "getAccel0\r\n");
		vTaskDelay(100); 
		sendString(qConfig->uartDevice, "stop\r\n");
	}
	return STATUS_PASS; 
}
/***********************************************************************************************
 * task_quintic_putQuinticToSleep(quinticConfiguration_t* qConfig)
 * @brief Send the sleep command to the quintic module. 
 * @param quinticConfiguration_t* qConfig 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t task_quintic_putQuinticToSleep(quinticConfiguration_t* qConfig)
{
	//send the start command. 
	if ((qConfig->isinit) && (qConfig->expectedNumberOfNods > 0))
	{	
		sendString(qConfig->uartDevice, "sleep\r\n");
	}
	return STATUS_PASS; 
}

//static functions

/***********************************************************************************************
 * sendString(drv_uart_config_t* uartConfig, char* cmd)
 * @brief Send a string to requested serial port
 * @param drv_uart_config_t* uartConfig, char* cmd 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
static status_t sendString(drv_uart_config_t* uartConfig, char* cmd)
{
	int len = strlen(cmd); 
	int i = 0; 
	for(;i<len;)
	{
		if(drv_uart_putChar(uartConfig,cmd[i]) == STATUS_PASS) //returns pass only when char sent
		{
			i++;
		}
	}
	return STATUS_PASS; 
}

/***********************************************************************************************
 * getAck(drv_uart_config_t* uartConfig)
 * @brief Look for an acknowledge from a Quintic
 * @param drv_uart_config_t* uartConfig
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
static status_t getAck(drv_uart_config_t* uartConfig)
{
	status_t result = STATUS_FAIL; 
	char buf[CMD_RESPONSE_BUF_SIZE] = {0}; //should move to static buffer for each quintic?
	/*result = drv_uart_getline(uartConfig, buf,CMD_RESPONSE_BUF_SIZE);*/
	result = drv_uart_getlineTimed(uartConfig, buf, CMD_RESPONSE_BUF_SIZE, 1000);
	if(result == STATUS_PASS)
	{
		if(strcmp(buf,QCMD_QN_ACK) != 0)
		{
			result = STATUS_FAIL;
		}
	}
	return result; 
}

/***********************************************************************************************
 * getResponse(drv_uart_config_t* uartConfig, char* expectedResponse)
 * @brief Look for a response from a Quintic
 * @param drv_uart_config_t* uartConfig, char* expectedResponse
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
static status_t getResponse(drv_uart_config_t* uartConfig, char* expectedResponse)
{
	status_t result = STATUS_FAIL;
	char buf[CMD_RESPONSE_BUF_SIZE] = {0}; //should move to static buffer for each quintic?
	if(drv_uart_getlineTimed(uartConfig, buf,CMD_RESPONSE_BUF_SIZE, 2000) == STATUS_PASS)
	{
		if(strcmp(buf,expectedResponse) == 0)
		{
			result = STATUS_PASS;
		}
	}
	return result;     
}

/***********************************************************************************************
 * createDummyData(int imuId, int seqNumber, int numVals, char* buf, size_t bufSize)
 * @brief Create dummy data of IMUs
 * @param int imuId, int seqNumber, int numVals, char* buf, size_t bufSize
 * @return void
 ***********************************************************************************************/
static void createDummyData(int imuId, int seqNumber, int numVals, char* buf, size_t bufSize)
{
	int i = 0; 
	int numChars = 0; 
	for(i =0 ; i < numVals; i++)
	{
		numChars += sprintf(buf+numChars,"%02dYY%04dRRR%d",i,seqNumber,imuId);
		if(numChars >= bufSize)
		{
			break;
		}
	}
}

/***********************************************************************************************
 * checkConnectedImus(quinticConfiguration_t* qConfig)
 * @brief Send a check command and see if all the IMUs are still connected
 * @param quinticConfiguration_t* qConfig
 * @return void
 ***********************************************************************************************/
status_t checkConnectedImus(quinticConfiguration_t* qConfig)
{
	status_t status = STATUS_FAIL;
	char buf[CMD_RESPONSE_BUF_SIZE] = {0};
	int i = 0, presentImuCount;
	char* bufPtr = buf;
	
	sendString(qConfig->uartDevice, "check\r\n");	//send the check command
	vTaskDelay(10);
	if(drv_uart_getlineTimed(qConfig->uartDevice, buf, CMD_RESPONSE_BUF_SIZE, 1500) == STATUS_PASS)
	{
		debugPrintString(buf);
		if(strncmp(buf,"ConnResp",8) == 0)
		{
			bufPtr = buf + 8;
			presentImuCount = 0;
			for (i=0; i<5; i++)
			{
				if (bufPtr[i] == '1')
				{
					qConfig->imuArray[i]->imuConnected = 1;
					presentImuCount++;
				}
				else
				{
					qConfig->imuArray[i]->imuConnected = 0;
				}
			}
			if(presentImuCount >= qConfig->expectedNumberOfNods)
			{
				status = STATUS_PASS;
				return status;
			}
		}
		
	}
	return status;
}

/***********************************************************************************************
 * scanForImus(quinticConfiguration_t* qConfig)
 * @brief Issue Scan command to Quintics and check the response. If it fails it tries two more times
 * @param quinticConfiguration_t* qConfig
 * @return void
 ***********************************************************************************************/
static status_t scanForImus(quinticConfiguration_t* qConfig)
{
	status_t status = STATUS_FAIL; 
	char buf[150] = {0}; 
	int i = 0;	
	char* bufPtr = buf; 
	int vScanLoopCount = 0;
	int presentImuCount = 0;
	do
	{
		sendString(qConfig->uartDevice,QCMD_SCAN); //send the scan command
		vTaskDelay(1);
		if(drv_uart_getlineTimed(qConfig->uartDevice, buf, sizeof(buf),16000) == STATUS_PASS)
		{
			debugPrintStringInt(buf, qConfig->qId);
			if(strncmp(buf,"ScanResp",8) == 0)
			{
				bufPtr = buf + 8; 
				presentImuCount = 0;
				for(i=0;i<qConfig->expectedNumberOfNods;i++)
				{
					if(bufPtr[i] == '1')
					{
						qConfig->imuArray[i]->imuPresent = 1; 	
						presentImuCount++;					
					}
					else
					{
						qConfig->imuArray[i]->imuPresent = 0; 
						if (i < qConfig->expectedNumberOfNods)
						{
							debugPrintStringInt("Failed on IMU Id\r\n", qConfig->imuArray[i]->imuId);
						}
					}			
				}
				if(presentImuCount >= qConfig->expectedNumberOfNods)
				{
					status = STATUS_PASS; 
					break; 
				}
			}
		}
		else
		{
			debugPrintStringInt("No response on scan\r\n", qConfig->qId);
		}		
		vTaskDelay(10);
		vScanLoopCount++;
	}while(vScanLoopCount<=QUINTIC_MAX_SCAN_ATTEMPTS);	
	return status; 
}

/***********************************************************************************************
 * connectToImus(quinticConfiguration_t* qConfig)
 * @brief Issue Connect command to Quintics and check the response. If it fails it tries two more times
 * @param quinticConfiguration_t* qConfig
 * @return void
 ***********************************************************************************************/
static status_t connectToImus(quinticConfiguration_t* qConfig)
{
	status_t status = STATUS_FAIL;
	char buf[150] = {0};
	int i = 0;
	char* bufPtr = buf;
	int vConnectionLoopCount = 0;
	int connectedImuCount = 0;
	uint32_t startTime = 0; 

	while(vConnectionLoopCount < 4)
	{
		//flush the buffer before sending the command. 
		//drv_uart_flushRx(qConfig->uartDevice);
		debugPrintStringInt("sent connect\r\n", qConfig->qId);
		sendString(qConfig->uartDevice,QCMD_CONNECT); //send the connect command
		vTaskDelay(1);
		//wait for 16 seconds trying to get the connect response from the quintic. 
		startTime = sgSysTickCount; 
		while(sgSysTickCount < (startTime + 16000))
		{
			if(drv_uart_getlineTimed(qConfig->uartDevice, buf, sizeof(buf), 4000) == STATUS_PASS)
			{
				debugPrintStringInt(buf, qConfig->qId);
				if(strncmp(buf,"ConnResp",8) == 0)
				{
					bufPtr = buf + 8;
					connectedImuCount = 0;
					for(i=0;i<qConfig->expectedNumberOfNods;i++)
					{
						if(bufPtr[i] == '1')
						{
							qConfig->imuArray[i]->imuConnected = 1;
							connectedImuCount++;
						}
						else
						{
							qConfig->imuArray[i]->imuConnected = 0;
							if (i < qConfig->expectedNumberOfNods)
							{
								debugPrintStringInt("Failed on IMU Id\r\n", qConfig->imuArray[i]->imuId);
							}
						}
					}
					if(connectedImuCount >= qConfig->expectedNumberOfNods)
					{
						status = STATUS_PASS;
						break;
					}
				}			
			}
			else
			{
				debugPrintStringInt("No response on connect\r\n", qConfig->qId);
			}			
		}
		if(status == STATUS_PASS)
		{
			break;
		}
		vConnectionLoopCount++;
	}
	return status;
}

/***********************************************************************************************
 * DisconnectImus(void)
 * @brief Issue begin command to Quintics to disconnect and power cycle them
 * @param quinticConfiguration_t* qConfig
 * @return void
 ***********************************************************************************************/
void DisconnectImus(quinticConfiguration_t* qConfig)
{
	if ((qConfig->isinit) && (qConfig->expectedNumberOfNods > 0))
	{
		sendString(qConfig->uartDevice,QCMD_BEGIN);
		vTaskDelay(100);
		getAck(qConfig->uartDevice);
		drv_gpio_setPinState(qConfig->resetPin, DRV_GPIO_PIN_STATE_LOW);
		vTaskDelay(100);
		drv_gpio_setPinState(qConfig->resetPin, DRV_GPIO_PIN_STATE_HIGH);
	}
}

static int getIndex(char c)
{
	int index = -1; 
	switch(c)
	{
		case '0':
		index = 0;
		break;
		case '1':
		index = 1;
		break;
		case '2':
		index = 2;
		break;
		case '3':
		index = 3;
		break;
		case '4':
		index = 4;
		break;
		default:
		index = -1;
		break;
	}
	return index; 
}