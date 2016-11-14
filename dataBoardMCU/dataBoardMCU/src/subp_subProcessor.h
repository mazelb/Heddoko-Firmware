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
#include "net_wirelessNetwork.h"

#define PACKET_QUEUE_LENGTH 10
#define MAX_NUMBER_OF_IMU_SENSORS 9

#define DATALOG_MAX_BUFFER_SIZE		8000
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
	int16_t Magnetic_x;
	int16_t Magnetic_y;
	int16_t Magnetic_z;
	int16_t Acceleration_x;
	int16_t Acceleration_y;
	int16_t Acceleration_z;
	int16_t Rotation_x;
	int16_t Rotation_y;
	int16_t Rotation_z;
    uint8_t frameStatus; 
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
	uint8_t chargerState; //BatteryLow = 0;	BatteryNominal = 1;	BatteryFull = 2; Charging = 3;
	uint8_t usbCommState; //0 = no comm detected, 1 = comm detected
	uint8_t jackDetectState; //mask indicating which jacks are connected.
	uint8_t streamState; //0 = Idle, 1 = Streaming, 2 = Error
	uint32_t sensorMask; //mask of which sensors have been detected.
}subp_status_t;
#pragma	pack(pop)

/*	Date and time structure for the power board	*/
#pragma pack(push, 1)
typedef struct
{
	uint32_t time;  // 0-6 bits: seconds from 0 to 59 in BCD
					// 8-14 bits: minutes from 0 to 59 in BCD
					// 16-21 bits: hours from (1 to 12) or (0 to 23) in BCD
					// 22 bit: 0 for AM, 1 for PM

	uint32_t date;  // 0-6 bits: century from 19 to 20 in BCD
					// 8-15 bits: year from 0 to 99 in BCD
					// 16-20 bits: month from 1 to 12 in BCD
					// 21-23 bits: day from 1 to 7 in BCD
					// 24-29 bits: date from 1 to 31 in BCD

}subp_dateTime_t;
#pragma pack(pop)

/* subprocessor configuration Structure */
typedef struct  
{
	uint8_t rate;
	uint32_t sensorMask; 
    char filename[100]; 
}subp_recordingConfig_t;

typedef struct 
{
    in_addr ipaddress; 
    uint16_t streamPort; 
}subp_streamConfig_t;

typedef struct  
{
    subp_recordingConfig_t* recordingConfig;
    subp_streamConfig_t* streamConfig;
}subp_moduleConfig_t;

void subp_subProcessorTask(void *pvParameters);
void subp_sendStringToUSB(char* string, size_t length);
void subp_sendForcedRestartMessage();

#endif /* SUBP_SUBPROCESSOR_H_ */