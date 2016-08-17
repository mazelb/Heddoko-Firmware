/*
 * sen_sensorHandler.h
 *
 * Created: 7/28/2016 3:39:03 PM
 *  Author: Hriday Mehta
 */ 


#ifndef SEN_SENSORHANDLER_H_
#define SEN_SENSORHANDLER_H_

#include "common.h"
#include "drv_gpio.h"

//#define ENABLE_SENSORS_DEBUG_MODE		//enable this define to switch to the debug packet and check the integrity of data

#define GPIO_RS485_DATA_DIRECTION_RE	DRV_GPIO_PIN_RS485
#define GPIO_RS485_DATA_DIRECTION_DE	DRV_GPIO_PIN_RS485

#define SENSOR_BUS_SPEED_HIGH			921600
#define SENSOR_BUS_SPEED_LOW			460800

typedef enum
{
	SENSOR_IDLE = 0x00,
	SENSOR_STREAMING,
	SENSOR_ERROR,
}sensor_state_t;

typedef	enum
{
	COMMAND_ID_GET_FRAME = 0x00,
	COMMAND_ID_UPDATE,
	COMMAND_ID_SETUP_MODE,
	COMMAND_ID_GET_STATUS,
	COMMAND_ID_ENABLE_HPR,
	COMMAND_ID_CHANGE_BAUD,
	COMMAND_ID_CHANGE_PADDING,
	COMMAND_ID_SET_RATES,
	COMMAND_ID_SET_IMU_ID,
	COMMAND_ID_RESET_FAKE,
	COMMAND_ID_UPDATE_FAKE
}sensor_commands_t;

typedef enum
{
	SENSOR_DISCONNECTED = 0,
	SENSOR_PACKET_INCOMPLETE,
}sensor_error_code_t;

void sen_sensorHandlerTask(void *pvParameters);
sensor_state_t sen_getSensorState(void);
uint32_t sen_getDetectedSensors(void);
void sen_enableSensorStream(bool enable);
void sen_setConfig(uint8_t *data);
void sen_preSleepProcess();

#endif /* SEN_SENSORHANDLER_H_ */