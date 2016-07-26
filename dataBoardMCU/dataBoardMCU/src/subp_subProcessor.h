/*
 * subp_subProcessor.h
 *
 * Created: 6/20/2016 10:33:59 AM
 *  Author: sean
 */ 


#ifndef SUBP_SUBPROCESSOR_H_
#define SUBP_SUBPROCESSOR_H_
#include "asf.h"
#include "common.h"

#define PACKET_QUEUE_LENGTH 10
#define MAX_NUMBER_OF_IMU_SENSORS 9
//Sub processor messages

#define PACKET_COMMAND_ID_SUBP_GET_STATUS 0x51
#define PACKET_COMMAND_ID_SUBP_GET_STATUS_RESP 0x52
#define PACKET_COMMAND_ID_SUBP_CONFIG 0x53
#define PACKET_COMMAND_ID_SUBP_STREAMING 0x54
#define PACKET_COMMAND_ID_SUBP_FULL_FRAME 0x55

/*	IMU packet*/
#pragma pack(push, 1)
typedef struct
{
	uint8_t sensorId;
	float32_t Quaternion_x;
	float32_t Quaternion_y;
	float32_t Quaternion_z;
	float32_t Quaternion_w;
	uint16_t Magnetic_x;
	uint16_t Magnetic_y;
	uint16_t Magnetic_z;
	uint16_t Acceleration_x;
	uint16_t Acceleration_y;
	uint16_t Acceleration_z;
	uint16_t Rotation_x;
	uint16_t Rotation_y;
	uint16_t Rotation_z;
}subp_imuFrame_t;
#pragma	pack(pop)

/*	Full IMU packet*/
#pragma pack(push, 1)
typedef struct
{
	uint8_t sensorCount;
	uint32_t timeStamp;
	subp_imuFrame_t frames[MAX_NUMBER_OF_IMU_SENSORS];		
}subp_fullImuFrameSet_t;
#pragma	pack(pop)


/*	Status structure for the power board*/
#pragma pack(push, 1)
typedef struct
{
	uint8_t chargeLevel;   //battery percentage 	
	uint8_t chargerState; //charging, battery full, battery low. 
	uint16_t sensorMask; //mask of which sensors have been detected.
}subp_status_t;
#pragma	pack(pop)


void subp_subProcessorTask(void *pvParameters);

#endif /* SUBP_SUBPROCESSOR_H_ */