/*
 * task_emInterface.h
 *
 * Created: 2/16/2016 1:35:06 PM
 *  Author: Hriday Mehta
 */ 

#ifndef TASK_EMINTERFACE_H_
#define TASK_EMINTERFACE_H_

#include "common.h"
#include "drv_gpio.h"
#include "drv_i2c.h"

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

typedef struct  
{
	int taskId;
	int pollRate;	//in ms
	int isInit;
	slave_twi_config_t* slaveConfigArray[2];	//each bus can have max 2 devices
}task_em_config_t;

//function declarations
void task_emHandler(void *pvParameters);
int task_em_initializeImus(task_em_config_t* emConfig);
static int resetAndInitialize(slave_twi_config_t* slave_config);
status_t task_em_start(task_em_config_t* emConfig);
status_t task_em_stop(task_em_config_t* emConfig);

#endif /* TASK_EMINTERFACE_H_ */