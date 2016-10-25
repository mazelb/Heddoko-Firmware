/*
 * subp_subProcessor.c
 *
 * Created: 6/20/2016 10:33:32 AM
 *  Author: sean
 * 
 */ 

#include "subp_subProcessor.h"
#include "common.h"
#include "msg_messenger.h"
#include "sys_systemManager.h"
#include "drv_uart.h"
#include "dbg_debugManager.h"
#include "net_wirelessNetwork.h"
#include "heddokoPacket.pb-c.h"
#include "pkt_packetParser.h"

/*	Static function forward declarations	*/
static void processRawPacket(pkt_rawPacket_t* packet);
static void processMessage(msg_message_t message);
status_t convertFullFrameToProtoBuff(subp_fullImuFrameSet_t* rawFullFrame, Heddoko__Packet* protoPacket);
//message senders
static void sendGetStatusMessage(drv_uart_config_t* uartConfig);
static void sendStreamMessage(drv_uart_config_t* uartConfig, uint8_t enable);
static void sendGetTimeRequestMessage(drv_uart_config_t* uartConfig);
static void sendConfigtMessage(drv_uart_config_t* uartConfig, uint8_t rate, uint32_t sensorMask);
static void sendPwrDownResponseMessage(drv_uart_config_t* uartConfig);
//possibly move this somewhere else, such as the 
static status_t setDateTimeFromPacket(pkt_rawPacket_t *packet);

void protoPacketInit();
//state change event functions
static status_t recordingStateEntry();
static status_t recordingStateExit();
static status_t streamingStateEntry();
static status_t streamingStateExit();

/*	Local variables	*/
xQueueHandle queue_subp = NULL;
subp_config_t subp_config = 
{
	.rate = 20,
	.sensorMask = 0x000001FF
};
subp_status_t subp_CurrentStatus = 
{
	.chargeLevel = 0,
	.chargerState = 0,
	.jackDetectState = 0x00,
	.sensorMask = 0x00000000,
	.streamState = 0,
	.usbCommState = 0
};
drv_uart_config_t subpUart =
{
	.p_usart = UART0,
	.mem_index = 0,
	.uart_options =
	{
		.baudrate   = 460800,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
	.mode = DRV_UART_MODE_INTERRUPT
};
Heddoko__Packet dataFrameProtoPacket;
Heddoko__FullDataFrame dataFrame;
Heddoko__ImuDataFrame* frameArrayPtr[MAX_NUMBER_OF_IMU_SENSORS];
Heddoko__ImuDataFrame frameArray[MAX_NUMBER_OF_IMU_SENSORS];
uint8_t serializedProtoBuf[2000]; //storage buffer for the largest serialized proto packet. 
#define MAX_SERIALIZED_DATA_LENGTH 2000
uint8_t serializedDataBuffer[MAX_SERIALIZED_DATA_LENGTH]; //storage buffer for the encoded data. 

volatile uint8_t dataLogBufferA[DATALOG_MAX_BUFFER_SIZE] = {0} , dataLogBufferB[DATALOG_MAX_BUFFER_SIZE] = {0};

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
	.activeBuffer = 0,
	.sem_bufferAccess = NULL
};

bool wifiConnected = false; 
net_socketConfig_t streamingSocket =
{
	.endpoint.sin_addr = 0xFFFFFFFF, //broadcast Address
	.endpoint.sin_family = AF_INET,
	.endpoint.sin_port = _htons(6669),
	.sourceModule = MODULE_WIFI,
    .socketId = -1,
    .clientSocketId = -1
};

sys_manager_systemState_t subpState = SYSTEM_STATE_INIT; 

/*	Extern functions	*/

/*	Extern variables	*/
extern uint32_t errorCount;
extern volatile uint32_t fullBufferError;
/*	Function Definitions	*/
void subp_subProcessorTask(void *pvParameters)
{
	msg_message_t receivedMessage;	
	pkt_rawPacket_t rawPacket = 
	{
		.bytesReceived = 0,
		.escapeFlag = 0,
		.payloadSize = 0
	};
	
	queue_subp = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_subp != 0)
	{
		msg_registerForMessages(MODULE_SUB_PROCESSOR, 0xff, queue_subp);
	}
	//initialize the uart packet receiver
	if(drv_uart_init(&subpUart) != STATUS_PASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"failed to open UART0 for subc\r\n"); 
	}
	//send stop streaming message
    sendStreamMessage(&subpUart, 0x00); 
    //send get status command
	sendGetStatusMessage(&subpUart);
	
	//send the get time command to the power board
	sendGetTimeRequestMessage(&subpUart);
	//start the main thread where we listen for packets and messages	
	while (1)
	{
		if (xQueueReceive(queue_subp, &receivedMessage, 1) == true)
		{
			processMessage(receivedMessage);
		}
		if(pkt_getPacketTimed(&subpUart,&rawPacket,10) == STATUS_PASS)
		{
			//we have a full packet	
			processRawPacket(&rawPacket);
		}					
		vTaskDelay(3);		// carefully assign the delay as one packet can be as fast as 10ms
	}
}

void subp_sendStringToUSB(char* string, size_t length)
{
    uint8_t packetBytes[255] = {0};
    packetBytes[0] = PACKET_TYPE_MASTER_CONTROL;
    packetBytes[1] = PACKET_COMMAND_ID_SUBP_OUTPUT_DATA;     
    memcpy(&(packetBytes[2]), string, length); 
    pkt_sendRawPacket(&subpUart, packetBytes, length+3); 
    
}

char tempString[200] = {0};
uint32_t packetReceivedCount = 0;

uint32_t lastTimeStamp = 0;	
static void processRawPacket(pkt_rawPacket_t* packet)
{
	subp_fullImuFrameSet_t* rawFullFrame; //pointer to raw packet type
	size_t serializedProtoPacketLength = 0;
	uint16_t serializedLength = 0;
	//check which type of packet it is.
	//All the packets should be of this type... we're not on a 485 bus. 
	int i =0;	
	int result = 0;
	if(packet->payload[0] == PACKET_TYPE_SUB_PROCESSOR)
	{
		
		switch(packet->payload[1])
		{
			case PACKET_COMMAND_ID_SUBP_GET_STATUS_RESP:
				//copy the structure to the received packet
				memcpy(&subp_CurrentStatus,&(packet->payload[2]),sizeof(subp_status_t)); 			
				//send it to the debug module for now, just so we can see it printed up. Eventually send it to the config manager
				//the pointer is sent only, so it doesn't take up to much space on the queue. 
				msg_sendMessage(MODULE_DEBUG, MODULE_SUB_PROCESSOR, MSG_TYPE_SUBP_STATUS, &subp_CurrentStatus); 
				msg_sendMessage(MODULE_BLE, MODULE_SUB_PROCESSOR, MSG_TYPE_SUBP_STATUS, &subp_CurrentStatus);
                msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_SUBP_STATUS, &subp_CurrentStatus);
			break;
			case PACKET_COMMAND_ID_SUBP_POWER_DOWN_REQ:
				//we received a power down request, let the system manager know. 
				msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_SUBP_POWER_DOWN_REQ, NULL); 					
			break;
			case PACKET_COMMAND_ID_SUBP_FULL_FRAME:
				//this is a frame, cast the payload to the rawFrame type. 
				rawFullFrame = (subp_fullImuFrameSet_t*) (&packet->payload[2]);
				if(rawFullFrame->sensorCount == 0)
                {
                    //there are no frames in this packet. don't save it. 
                    
                    return; 
                }
                protoPacketInit();
				convertFullFrameToProtoBuff(rawFullFrame,&dataFrameProtoPacket);	
				//Serialize protobuf packet		
				serializedProtoBuf[0] = PACKET_TYPE_PROTO_BUF;		
				serializedProtoPacketLength = heddoko__packet__pack(&dataFrameProtoPacket,serializedProtoBuf+1);//offset by 1 to account for packet type. 	
				serializedProtoPacketLength += 1; //length plus 1 to account for packet type
				//now encode the packet for saving/transmission	 
				if(pkt_serializeRawPacket(serializedDataBuffer, MAX_SERIALIZED_DATA_LENGTH,&serializedLength,
					serializedProtoBuf,serializedProtoPacketLength) == STATUS_PASS)					
				{
					//the packet has been serialized...					
					if(dataLogFile.fileOpen)
					{					
						sdc_writeToFile(&dataLogFile, serializedDataBuffer, serializedLength); 
					}
					//check if the streaming socket is open. (should only be open in receive mode)
					if(streamingSocket.socketId > -1)
					{
						net_sendUdpPacket(&streamingSocket, serializedDataBuffer, serializedLength);
					}
				}
				lastTimeStamp = rawFullFrame->timeStamp;
                if(rawFullFrame->sensorCount != 9)
                {
				    //dbg_printf(DBG_LOG_LEVEL_DEBUG,"%d,%d,%d,%d,%d \r\n",packetReceivedCount++,rawFullFrame->timeStamp,result,errorCount,drv_uart_getDroppedBytes(&subpUart));
                }                
								
			break;
			case PACKET_COMMAND_ID_SUBP_GET_DATE_TIME_RESP:
				//set the time from the packet contents. 
				setDateTimeFromPacket(packet);
			break;
			case PACKET_COMMAND_ID_SUBP_OUTPUT_DATA:
			    dbg_processCommand(DBG_CMD_SOURCE_USB, (packet->payload)+2, packet->payloadSize - 2);
			
			break;			
			default:
			
			break;
		}
	}	
	//dbg_printString("Received a packet!!!!"); 
}

static void processMessage(msg_message_t message)
{
	switch(message.type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		{
			if(message.data == SYSTEM_STATE_RECORDING)
			{
				if(recordingStateEntry() == STATUS_PASS)
				{
					subpState = SYSTEM_STATE_RECORDING;
					msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_READY, NULL);
				}
				else
				{
					msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_ERROR, NULL);
				}
			}
			else if(message.data == SYSTEM_STATE_STREAMING)
			{
				if(streamingStateEntry() == STATUS_PASS)
				{
					subpState = SYSTEM_STATE_STREAMING;
					msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_READY, NULL);
				}
				else
				{
					msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_ERROR, NULL);
				}
			}
			else if(message.data == SYSTEM_STATE_IDLE || message.data == SYSTEM_STATE_ERROR)
			{
				if(subpState == SYSTEM_STATE_RECORDING)
				{
					recordingStateExit();
				}
				else if(subpState == SYSTEM_STATE_STREAMING)
				{
					streamingStateExit();
				}
				subpState = message.data; 
			}
		}
		break;
		case MSG_TYPE_LEAVING_STATE:
		{
			if(message.data == SYSTEM_STATE_RECORDING)
			{
				recordingStateExit();
				msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_READY, NULL);
				subpState = SYSTEM_STATE_IDLE;
			}
			else if(message.data == SYSTEM_STATE_STREAMING)
			{
				streamingStateExit();
				msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_READY, NULL);
				subpState = SYSTEM_STATE_IDLE;
			}			
		}
		break;
		case MSG_TYPE_SUBP_POWER_DOWN_READY:
		{
			//power down is ready, so send the response message back to the power board.
			sendPwrDownResponseMessage(&subpUart);
		}
		break;
		case MSG_TYPE_WIFI_STATE:
		{
			if(message.data == 0)
			{
				wifiConnected = false;
                
			}
			else
			{
				wifiConnected = true; 
			}
		}		
		default:
		break;
		
	}
}


//Initialize the protopacket, should only need to be done once
void protoPacketInit()
{
	int i =0;
	heddoko__packet__init(&dataFrameProtoPacket);
	heddoko__full_data_frame__init(&dataFrame);
	dataFrameProtoPacket.type = HEDDOKO__PACKET_TYPE__DataFrame;
	heddoko__full_data_frame__init(dataFrameProtoPacket.fulldataframe);
	dataFrameProtoPacket.fulldataframe = &dataFrame;
	dataFrameProtoPacket.fulldataframe->imudataframe = frameArrayPtr;
	
	for(i=0;i<MAX_NUMBER_OF_IMU_SENSORS;i++)
	{
		heddoko__imu_data_frame__init(&frameArray[i]);
		frameArrayPtr[i] = &frameArray[i];	
	}
	
}
status_t convertFullFrameToProtoBuff(subp_fullImuFrameSet_t* rawFullFrame, Heddoko__Packet* protoPacket)
{
	int i = 0;
	protoPacket->fulldataframe->timestamp = rawFullFrame->timeStamp;	
	protoPacket->fulldataframe->n_imudataframe = rawFullFrame->sensorCount;
	for(i=0;i<rawFullFrame->sensorCount;i++)
	{	
		//initialize all the frames
		//heddoko__imu_data_frame__init(&protoPacket->fulldataframe->imudataframe[i]);
		//frameArrayPtr[i] = &frameArray[i];
		//only copy over the data if it's there		
		if(i<rawFullFrame->sensorCount)
		{		
			protoPacket->fulldataframe->imudataframe[i]->imuid = rawFullFrame->frames[i].sensorId;
			protoPacket->fulldataframe->imudataframe[i]->sensormask = 0x1fff;	//as it has all the values from the sensor
			protoPacket->fulldataframe->imudataframe[i]->has_sensormask = true;
			
			protoPacket->fulldataframe->imudataframe[i]->accel_x = rawFullFrame->frames[i].Acceleration_x;
			protoPacket->fulldataframe->imudataframe[i]->accel_y = rawFullFrame->frames[i].Acceleration_y;
			protoPacket->fulldataframe->imudataframe[i]->accel_z = rawFullFrame->frames[i].Acceleration_z;
			protoPacket->fulldataframe->imudataframe[i]->has_accel_x = true;
			protoPacket->fulldataframe->imudataframe[i]->has_accel_y = true;
			protoPacket->fulldataframe->imudataframe[i]->has_accel_z = true;

			protoPacket->fulldataframe->imudataframe[i]->quat_x_yaw = rawFullFrame->frames[i].Quaternion_x;
			protoPacket->fulldataframe->imudataframe[i]->quat_y_pitch = rawFullFrame->frames[i].Quaternion_y;
			protoPacket->fulldataframe->imudataframe[i]->quat_z_roll = rawFullFrame->frames[i].Quaternion_z;
			protoPacket->fulldataframe->imudataframe[i]->quat_w = rawFullFrame->frames[i].Quaternion_w;
			protoPacket->fulldataframe->imudataframe[i]->has_quat_x_yaw = true;
			protoPacket->fulldataframe->imudataframe[i]->has_quat_y_pitch = true;
			protoPacket->fulldataframe->imudataframe[i]->has_quat_z_roll = true;
			protoPacket->fulldataframe->imudataframe[i]->has_quat_w = true;

			protoPacket->fulldataframe->imudataframe[i]->mag_x = rawFullFrame->frames[i].Magnetic_x;
			protoPacket->fulldataframe->imudataframe[i]->mag_y = rawFullFrame->frames[i].Magnetic_y;
			protoPacket->fulldataframe->imudataframe[i]->mag_z = rawFullFrame->frames[i].Magnetic_z;
			protoPacket->fulldataframe->imudataframe[i]->has_mag_x = true;
			protoPacket->fulldataframe->imudataframe[i]->has_mag_y = true;
			protoPacket->fulldataframe->imudataframe[i]->has_mag_z = true;

			protoPacket->fulldataframe->imudataframe[i]->rot_x = rawFullFrame->frames[i].Rotation_x;
			protoPacket->fulldataframe->imudataframe[i]->rot_y = rawFullFrame->frames[i].Rotation_y;
			protoPacket->fulldataframe->imudataframe[i]->rot_z = rawFullFrame->frames[i].Rotation_z;
			protoPacket->fulldataframe->imudataframe[i]->has_rot_x = true;
			protoPacket->fulldataframe->imudataframe[i]->has_rot_y = true;
			protoPacket->fulldataframe->imudataframe[i]->has_rot_z = true;
		}
	}	
}

void processStatusMessage(uint8_t* packet)
{
	//copy the packet to the local structure
	memcpy(&subp_CurrentStatus,packet, sizeof(subp_status_t));
	//do something with this status... should we send it around everywhere, or just leave it for other modules to grab?
	
}




static status_t recordingStateEntry()
{
	status_t status = STATUS_PASS;
	//check for valid calibration file (the name of the last calibration file should be populated)
	
	//open/create recording data file
	status =  sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);		
	//write file header to log	
	if(status == STATUS_PASS)
    {
	    //send config to power board
	    sendConfigtMessage(&subpUart,subp_config.rate,subp_config.sensorMask);
	    //send stream start command to power board. 
	    sendStreamMessage(&subpUart, 0x01);
    }    
	return status;
}

static status_t recordingStateExit()
{
	status_t status = STATUS_PASS;
	//send stop streaming command to power board	
	sendStreamMessage(&subpUart, 0x00);
	//close file
	status =  sdc_closeFile(&dataLogFile);
	return status; 
}

static status_t streamingStateEntry()
{
	status_t status = STATUS_PASS;
	
	//check for valid calibration file (the name of the last calibration file should be populated)
	
	//check if we are connected to wifi
	if(!wifiConnected)
	{
		//since we're not connected we cannot stream. don't know how we'd get here. 
		return STATUS_FAIL;	
	}
	//create the udp socket
	dbg_printString(DBG_LOG_LEVEL_DEBUG, "Creating streaming socket\r\n");
	status = net_createUdpSocket(&streamingSocket, 255);	
	//open/create recording data file
	status |=  sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);
	//write file header to log
	if(status == STATUS_PASS)
    {
	    //send config to power board
	    sendConfigtMessage(&subpUart,subp_config.rate,subp_config.sensorMask);
	    //send stream start command to power board.
	    sendStreamMessage(&subpUart, 0x01);
    }    
	return status;
}

static status_t streamingStateExit()
{
	status_t status = STATUS_PASS;
	//send stop streaming command to power board
	sendStreamMessage(&subpUart, 0x00);
	//close UDP socket
	status = net_closeSocket(&streamingSocket);
	//close file
	status =  sdc_closeFile(&dataLogFile);	
	
	return status;
}

//message sending functions
static void sendGetStatusMessage(drv_uart_config_t* uartConfig)
{
	uint8_t getStatusBytes[2] = {0x01,PACKET_COMMAND_ID_SUBP_GET_STATUS};
	pkt_sendRawPacket(uartConfig,getStatusBytes, sizeof(getStatusBytes));
}

static void sendStreamMessage(drv_uart_config_t* uartConfig, uint8_t enable)
{
	uint8_t streamMessageBytes[] = {0x01,PACKET_COMMAND_ID_SUBP_STREAMING,enable};
	pkt_sendRawPacket(uartConfig,streamMessageBytes, sizeof(streamMessageBytes));
}

static void sendGetTimeRequestMessage(drv_uart_config_t* uartConfig)
{
	uint8_t getTimeBytes[] = {0x01,PACKET_COMMAND_ID_SUBP_GET_DATE_TIME};
	pkt_sendRawPacket(uartConfig,getTimeBytes, sizeof(getTimeBytes));
}

static void sendConfigtMessage(drv_uart_config_t* uartConfig, uint8_t rate, uint32_t sensorMask)
{
	uint8_t configBytes[] = {0x01,PACKET_COMMAND_ID_SUBP_CONFIG,0x00,0x00,0x00,0x00,0x00};
	configBytes[2] = rate;
	memcpy(&configBytes[3],&sensorMask,4); //copy the sensor mask to the config bytes.  
	pkt_sendRawPacket(uartConfig,configBytes, sizeof(configBytes));
}

static void sendPwrDownResponseMessage(drv_uart_config_t* uartConfig)
{
	uint8_t rawBytes[] = {0x01,PACKET_COMMAND_ID_SUBP_POWER_DOWN_RESP};
	pkt_sendRawPacket(uartConfig,rawBytes, sizeof(rawBytes));
}

/************************************************************************
 * setDateTimeFromPacket(pkt_rawPacket_t *packet)
 * @brief Set the date and time received from the data board
 * @param pkt_rawPacket_t *packet: pointer to the packet
 * @return status_t: STATUS_PASS or STATUS_FAILED                   
 ************************************************************************/
static status_t setDateTimeFromPacket(pkt_rawPacket_t *packet)
{
	status_t status = STATUS_PASS;
	subp_dateTime_t dateTime;
	unsigned long startTime = xTaskGetTickCount(), curTime = 0;
	
	// set to registers directly
	dateTime.time = packet->payload[2] | (packet->payload[3] << 8) | (packet->payload[4] << 16) | (0x00 <<22);	// 0x00 as we are using 24-hour mode
	dateTime.date = packet->payload[6] | (packet->payload[7] << 8) | (((packet->payload[8] & 0x1F) | (packet->payload[8] & 0xE0)) << 16) 
					| (packet->payload[9] << 24);
	
	taskENTER_CRITICAL();
	// Date
	RTC->RTC_CR |= RTC_CR_UPDCAL;
	//delay_ms(1500);	// NOTE: remove this delay when the while loop below starts working
	while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD)
	{
		curTime = xTaskGetTickCount();
		if (curTime - startTime > 2000)		// it can take up to one second to get the ACKUPD bit
		{
			status |= STATUS_FAIL;
			return status;
		}
	}
	RTC->RTC_SCCR = RTC_SCCR_ACKCLR;
	RTC->RTC_CALR = dateTime.date;
	RTC->RTC_CR &= (~RTC_CR_UPDCAL);
	RTC->RTC_SCCR |= RTC_SCCR_SECCLR;

	status |=  (RTC->RTC_VER & RTC_VER_NVCAL);	// check if the RTC contains a valid value
	
	// Time
	RTC->RTC_CR |= RTC_CR_UPDTIM;
	//delay_ms(1500);	// NOTE: remove this delay when the while loop below starts working
	while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD)
	{
		curTime = xTaskGetTickCount();
		if (curTime - startTime > 2000)		// it can take up to one second to get the ACKUPD bit
		{
			status |= STATUS_FAIL;
			return status;
		}
	}
	RTC->RTC_SCCR = RTC_SCCR_ACKCLR;
	RTC->RTC_TIMR = dateTime.time;
	RTC->RTC_CR &= (~RTC_CR_UPDTIM);
	RTC->RTC_SCCR |= RTC_SCCR_SECCLR;

	status |= (RTC->RTC_VER & RTC_VER_NVTIM);	// check if the RTC contains a valid value
	
	/*	return the status	*/
	taskEXIT_CRITICAL();
	return status;
}