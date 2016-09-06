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
static void processMessage(msg_message_t message);
static void processRawPacket(pkt_rawPacket_t* packet);
status_t convertFullFrameToProtoBuff(subp_fullImuFrameSet_t* rawFullFrame, Heddoko__Packet* protoPacket);
//message senders
static void sendGetStatusMessage(drv_uart_config_t* uartConfig);
static void sendStreamMessage(drv_uart_config_t* uartConfig, uint8_t enable);
static void sendGetTimeRequestMessage(drv_uart_config_t* uartConfig);
static void sendConfigtMessage(drv_uart_config_t* uartConfig, uint8_t rate, uint32_t sensorMask);
static void sendPwrDownResponseMessage(drv_uart_config_t* uartConfig);

void protoPacketInit();
//state change event functions
static status_t recordingStateEntry();
static status_t recordingStateExit();

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
				
			
			
			
			break;
			case PACKET_COMMAND_ID_SUBP_FULL_FRAME:
				//this is a frame, cast the payload to the rawFrame type. 
				rawFullFrame = (subp_fullImuFrameSet_t*) (&packet->payload[2]);
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
					//net_sendPacket(serializedDataBuffer, serializedLength);
				}
				if(rawFullFrame->timeStamp > lastTimeStamp)
				{					
					if(rawFullFrame->frames[8].Rotation_z == 13)
					{
						result = 1;	//everything is good
					}
					else
					{
						result = 2; 	//corrupt frame
					}					 
				}
				else
				{
					result = 0; //out of sequence frame
				}
				lastTimeStamp = rawFullFrame->timeStamp;
				dgb_printf(DBG_LOG_LEVEL_DEBUG,"%d,%d,%d,%d,%d\r\n",packetReceivedCount++,rawFullFrame->timeStamp,result,errorCount,drv_uart_getDroppedBytes(&subpUart));
				
			break;
			
			case PACKET_COMMAND_ID_SUBP_POWER_DOWN_REQ:
				sendPwrDownResponseMessage(&subpUart);
			break;
			
			default:
			
			break;
		}
	}	
	//dbg_printString("Received a packet!!!!"); 
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
					msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_READY, NULL); 
				}
				else
				{
					msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_ERROR, NULL); 
				}				
			}
		}
		break;
		case MSG_TYPE_LEAVING_STATE:
		{
			if(message.data == SYSTEM_STATE_RECORDING)
			{
				recordingStateExit();
				msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_READY, NULL); 
			}
		}
		break;	
		case MSG_TYPE_STATE:
		break;
		default:
		break;
		 
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
	
	//send config to power board
	sendConfigtMessage(&subpUart,subp_config.rate,subp_config.sensorMask);
	//send stream start command to power board. 
	sendStreamMessage(&subpUart, 0x01);
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
	uint8_t bytes[] = {0x01,PACKET_COMMAND_ID_SUBP_POWER_DOWN_RESP};
	pkt_sendRawPacket(uartConfig,bytes, sizeof(bytes));
}