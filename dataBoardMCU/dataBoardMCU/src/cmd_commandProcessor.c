/*
 * cmd_commandProcessor.c
 *
 * Created: 2016-04-18 12:27:36 PM
 *  Author: Hriday Mehta
 */ 

#include <asf.h>
#include <string.h>
#include "cmd_commandProcessor.h"
#include "common.h"
#include "arm_math.h"
#include "pkt_packetParser.h"
#include "imu.h"
#include "pkt_packetParser.h"
#include "heddokoPacket.pb-c.h"
#include "task_SensorHandler.h"
#include "drv_uart.h"

extern bool packetState;
extern uint8_t loopCount;
extern drv_uart_config_t uart0Config;

volatile cmd_debugStructure_t debugStructure =
{
	.accelReadErrorCount = 0,
	.gyroReadErrorCount = 0,
	.magReadErrorCount = 0,
	.quatReadErrorCount = 0,
	.statusMask = 0,
	.receiveErrorCount = 0
};

Heddoko__Packet packet;
Heddoko__FullDataFrame dataFrame;
Heddoko__ImuDataFrame* frameArrayPtr[9];
Heddoko__ImuDataFrame frameArray[9];
imuFrame_t* imuFrameDataProto;
uint8_t sensorId;
uint8_t packetBytes[1000] = {0};

void protoPacketInit()
{
	heddoko__packet__init(&packet);
	heddoko__full_data_frame__init(&dataFrame);
	packet.type = HEDDOKO__PACKET_TYPE__DataFrame;
	packet.fulldataframe = &dataFrame;
	heddoko__full_data_frame__init(packet.fulldataframe);
	packet.fulldataframe->timestamp = xTaskGetTickCount();
	packet.fulldataframe->imudataframe = frameArrayPtr;
	packet.fulldataframe->n_imudataframe = 9;
}

void clearProtoPacket()
{
	//memcpy(frameArray, NULL, sizeof(frameArray));
	for (int i = 0; i<9; i++)
	{
		frameArray[i].sensormask = 0;
	}
}

void makeProtoPacket(rawPacket_t* rawPacket)
{
	sensorId = rawPacket->payload[2];
	//sensorId = loopCount; 
	
	imuFrameDataProto = (imuFrame_t*) (&rawPacket->payload[3]);

	heddoko__imu_data_frame__init(&frameArray[sensorId]);
	frameArrayPtr[sensorId] = &frameArray[sensorId];
	frameArray[sensorId].imuid = sensorId;
	frameArray[sensorId].sensormask = 0x1fff;	//as it has all the values from the sensor
	frameArray[sensorId].has_sensormask = true;
	
	frameArray[sensorId].accel_x = imuFrameDataProto->Acceleration_x;
	frameArray[sensorId].accel_y = imuFrameDataProto->Acceleration_y;
	frameArray[sensorId].accel_z = imuFrameDataProto->Acceleration_z;
	frameArray[sensorId].has_accel_x = true;
	frameArray[sensorId].has_accel_y = true;
	frameArray[sensorId].has_accel_z = true;

	frameArray[sensorId].quat_x_yaw = imuFrameDataProto->Quaternion_x;
	frameArray[sensorId].quat_y_pitch = imuFrameDataProto->Quaternion_y;
	frameArray[sensorId].quat_z_roll = imuFrameDataProto->Quaternion_z;
	frameArray[sensorId].quat_w = imuFrameDataProto->Quaternion_w;
	frameArray[sensorId].has_quat_x_yaw = true;
	frameArray[sensorId].has_quat_y_pitch = true;
	frameArray[sensorId].has_quat_z_roll = true;
	frameArray[sensorId].has_quat_w = true;

	frameArray[sensorId].mag_x = imuFrameDataProto->Magnetic_x;
	frameArray[sensorId].mag_y = imuFrameDataProto->Magnetic_y;
	frameArray[sensorId].mag_z = imuFrameDataProto->Magnetic_z;
	frameArray[sensorId].has_mag_x = true;
	frameArray[sensorId].has_mag_y = true;
	frameArray[sensorId].has_mag_z = true;

	frameArray[sensorId].rot_x = imuFrameDataProto->Rotation_x;
	frameArray[sensorId].rot_y = imuFrameDataProto->Rotation_y;
	frameArray[sensorId].rot_z = imuFrameDataProto->Rotation_z;
	frameArray[sensorId].has_rot_x = true;
	frameArray[sensorId].has_rot_y = true;
	frameArray[sensorId].has_rot_z = true;
	//sensorId++;
	//if (sensorId>=9)
	//{
		//sensorId = 0;
	//}
}

void sendProtoPacket()
{
	packet.fulldataframe->timestamp = xTaskGetTickCount();
	size_t packetLength = heddoko__packet__pack(&packet, packetBytes);
	pkt_SendRawPacketProto(&uart0Config, packetBytes, packetLength);
	sensorId = 0;
}	

/* all packets have the format
 <type(1B)><command id(1B)><payload(size dependant on command type)> 
*/
void cmd_processPacket(rawPacket_t* packet)
{
	imuFrame_t* imuPacket;
	//cmd_debugStructure_t* debugStructure_sensor;
	
	//make sure the packet has enough bytes in it
	if(packet->payloadSize < 2)
	{
		//if there's less than two bytes... its not a valid packet
		return; 
	}
	//check which type of packet it is.
	//We don't require the Master_Control part
	if(packet->payload[0] == PACKET_TYPE_IMU_SENSOR)
	{
		//sensorId = packet->payload[2];
		switch(packet->payload[1])
		{
			case PACKET_COMMAND_ID_GET_FRAME_RESP:
				imuPacket = (imuFrame_t*) (&packet->payload[3]);
				#ifdef ENABLE_SENSOR_PACKET_TEST
				if (packetState == TRUE)
				{
					puts("Good, Cmp\r");	//sensor packet is complete and data is good
				}
				else
				{
					puts("Bad, Cmp \r");	//sensor packet is complete but data is bad
				}
				#endif
				//printf("Err Cnt: %03d,", debugStructure.receiveErrorCount);
				//printf("%f;%f;%f;%f,", imuPacket->Quaternion_x, imuPacket->Quaternion_y, imuPacket->Quaternion_z, imuPacket->Quaternion_w);
				//printf("%5d;%5d;%5d,", imuPacket->Magnetic_x, imuPacket->Magnetic_y, imuPacket->Magnetic_z);
				//printf("%5d;%5d;%5d,", imuPacket->Acceleration_x, imuPacket->Acceleration_y, imuPacket->Acceleration_z);
				//printf("%5d;%5d;%5d\r\n", imuPacket->Rotation_x, imuPacket->Rotation_y, imuPacket->Rotation_z);
				
				makeProtoPacket(packet);
				
			break;
			case PACKET_COMMAND_ID_BUTTON_PRESS:
				puts("Received Button Press event\r\n");
			break;
			case PACKET_COMMAND_ID_SET_IMU_ID_RESP:
			break;
			case PACKET_COMMAND_ID_GET_STATUS_RESP:
				//debugStructure_sensor = (cmd_debugStructure_t*) (&packet->payload[3]);
				//printf("Err Cnt: %d\t,", debugStructure.receiveErrorCount);
				//printf("%d\r\n", debugStructure_sensor->accelReadErrorCount);
			break;
			default:
			break; 
		}
	}
}
