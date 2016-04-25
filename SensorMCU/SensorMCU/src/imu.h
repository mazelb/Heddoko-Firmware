/*
 * imu.h
 *
 * Created: 3/14/2016 1:13:59 PM
 *  Author: Hriday Mehta
 */ 


#ifndef IMU_H_
#define IMU_H_

#include "asf.h"
#include "arm_math.h"
#include "common.h"

//Head, pitch, roll
//#define HPR	//undefine to switch back to quaternion data

#define NO_OF_EM_SENSORS				2

#define EM_EVENT_STATUS_REGISTER		0x35
#define EM_RESET_REQUEST_REGISTER		0x9B
#define EM_SENTRAL_STATUS_REGISTER		0x37
#define EM_ALGORITHM_CONTROL_REGISTER	0x54
#define EM_MAG_RATE_CONFIG_REGISTER		0x55
#define EM_ACCEL_RATE_CONFIG_REGISTER	0x56
#define EM_GYRO_RATE_CONFIG_REGISTER	0x57
#define EM_INTERRUPT_CONFIG_REGISTER	0x33
#define EM_RUN_REQUEST_REGISTER			0x34

#define EM_QUATERNION_RESULT_ADDRESS	0x00

#define EM_HPR_OUTPUT_ENABLE_MASK		0x04

#define EM_EEPROM_SUCCESS_MASK			0x0B
#define EM_NO_EEPROM_ERROR_MASK			0x18
#define EM_EEPROM_UPLOAD_ERROR_MASK		0x0D
#define EM_RESET_STATUS_MASK			0x01
#define EM_ERROR_STATUS_MASK			0x02
#define EM_RESULT_MASK_QUATERNION		0X04
#define EM_RESULT_MASK_MAG				0x08
#define EM_RESULT_MASK_ACCEL			0x10
#define EM_RESULT_MASK_GYRO				0x20

#define EM_RESET_REQUEST_FLAG			0x01
#define EM_RUN_REQUEST_FLAG				0x01
#define EM_CLEAR_RUN_REQUEST_FLAG		0x00
#define EM_RESET_INT_FLAG				0x01
#define EM_ERROR_INT_FLAG				0x02
#define EM_QUATERNION_RESULT_INT_FLAG	0x04
#define EM_MAG_RESULT_INT_FLAG			0x08
#define EM_ACCEL_RESULT_INT_FLAG		0x10
#define EM_GYRO_RESULT_INT_FLAG			0x20

//EM output data rate (ODR)
#define EM_ACCEL_OUPUT_DATA_RATE		0x0a	//The register value should be 1/10th of desired rate
#define EM_GYRO_OUPUT_DATA_RATE			0x14	//The register value should be 1/10th of desired rate
#define EM_MAG_OUPUT_DATA_RATE			0x1e

//frame packet length
#define EM_TOTAL_PACKET_LENGTH			EM_PACKET_QUATERNION
#define EM_PACKET_LENGTH				EM_QUATERNION_SNIPPET_NUMBER
#define EM_QUATERNION_SNIPPET_NUMBER	4
#define EM_PACKET_QUATERNION			(EM_QUATERNION_SNIPPET_NUMBER*EM_PACKET_DATA_SIZE)+(EM_QUATERNION_SNIPPET_NUMBER)	//including the end null character
#define EM_HPR_SNIPPET_NUMBER			3	
#define EM_PACKET_HPR					(EM_HPR_SNIPPET_NUMBER*EM_PACKET_DATA_SIZE)+(EM_HPR_SNIPPET_NUMBER)	//including the end null character
#define EM_PACKET_DATA_SIZE				8


/*	IMU packet from EM7180	*/
#pragma pack(push, 1)
typedef struct
{
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
}imuFrame_t;
#pragma	pack(pop)

#endif /* IMU_H_ */