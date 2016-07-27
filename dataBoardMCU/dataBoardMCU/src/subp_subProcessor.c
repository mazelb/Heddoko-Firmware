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
#include "drv_uart.h"
#include "dbg_debugManager.h"
#include "heddokoPacket.pb-c.h"
#include "pkt_packetParser.h"

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void processRawPacket(pkt_rawPacket_t* packet);
status_t convertFullFrameToProtoBuff(subp_fullImuFrameSet_t* rawFullFrame, Heddoko__Packet* protoPacket);
/*	Local variables	*/
xQueueHandle queue_subp = NULL;
drv_uart_config_t uart0Config =
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
	.mode = DRV_UART_MODE_DMA
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
	if(drv_uart_init(&uart0Config) != STATUS_PASS)
	{
		dbg_printString("failed to open UART0 for subc\r\n"); 
	}

	
	//send the get time command to the power board
	
	//start the main thread where we listen for packets and messages	
	while (1)
	{
		if (xQueueReceive(queue_subp, &receivedMessage, 1) == true)
		{
			processMessage(receivedMessage);
		}
		if(drv_uart_getPacketTimed(&uart0Config,&rawPacket,10) == STATUS_PASS)
		{
			//we have a full packet	
			processRawPacket(&rawPacket);

		}					
		vTaskDelay(1);		// carefully assign the delay as one packet can be as fast as 1.85ms
	}
}

static void processRawPacket(pkt_rawPacket_t* packet)
{
	subp_fullImuFrameSet_t* rawFullFrame; //pointer to raw packet type
	size_t serializedProtoPacketLength = 0;
	uint16_t serializedLength = 0;
	//check which type of packet it is.
	//All the packets should be of this type... we're not on a 485 bus. 
		
	if(packet->payload[0] == PACKET_TYPE_SUB_PROCESSOR)
	{
		
		switch(packet->payload[1])
		{
			case PACKET_COMMAND_ID_SUBP_GET_STATUS_RESP:
			
			
			
			
			break;
			case PACKET_COMMAND_ID_SUBP_FULL_FRAME:
				//this is a frame, cast the payload to the rawFrame type. 
				rawFullFrame = (subp_fullImuFrameSet_t*) (&packet->payload[3]);
				convertFullFrameToProtoBuff(rawFullFrame,&dataFrameProtoPacket);	
				//Serialize protobuf packet
				serializedProtoPacketLength = heddoko__packet__pack(&packet,serializedProtoBuf);		
				//now encode the packet for saving/transmission	
				if(pkt_serializeRawPacket(serializedDataBuffer, MAX_SERIALIZED_DATA_LENGTH,&serializedLength,
					serializedProtoBuf,serializedProtoPacketLength) == STATUS_PASS)
				{
					
				}
				
			break;
			default:
			
			break;
		}
	}	
	dbg_printString("Received a packet!!!!"); 
}


//Initialize the protopacket, should only need to be done once
void protoPacketInit()
{
	int i =0;
	heddoko__packet__init(&dataFrameProtoPacket);
	heddoko__full_data_frame__init(&dataFrame);
	dataFrameProtoPacket.type = HEDDOKO__PACKET_TYPE__DataFrame;
	dataFrameProtoPacket.fulldataframe = &dataFrame;
	dataFrameProtoPacket.fulldataframe->imudataframe = frameArrayPtr;
	heddoko__full_data_frame__init(dataFrameProtoPacket.fulldataframe);
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
	for(i=0;i<MAX_NUMBER_OF_IMU_SENSORS;i++)
	{	
		//initialize all the frames
		heddoko__imu_data_frame__init(&protoPacket->fulldataframe->imudataframe[i]);
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
			
		break;
		case MSG_TYPE_READY:
		break;
		case MSG_TYPE_ERROR:
		break;
		default:
		break;
		 
	}
}

