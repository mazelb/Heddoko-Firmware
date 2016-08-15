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

//#define ENABLE_SENSORS_DEBUG_MODE

#define GPIO_RS485_DATA_DIRECTION_RE	DRV_GPIO_PIN_GPIO
#define GPIO_RS485_DATA_DIRECTION_DE	DRV_GPIO_PIN_GPIO

#define SENSOR_BUS_SPEED_HIGH			921600
#define SENSOR_BUS_SPEED_LOW			460800

//#define ENABLE_SENSOR_PACKET_TEST		//enable this define to switch to the debug packet and check the integrity of data

typedef enum
{
	SENSOR_IDLE = 0x00,
	SENSOR_STREAMING,
	SENSOR_ERROR,
}sensor_state_t;

typedef enum
{
	SENSOR_DISCONNECTED = 0,
	SENSOR_PACKET_INCOMPLETE,
}sensor_error_code_t;

void sen_sensorHandlerTask(void *pvParameters);
sensor_state_t sen_getSensorState(void);
uint32_t sen_getDetectedSensors(void);
void sen_enableSensorStream(bool enable);

#endif /* SEN_SENSORHANDLER_H_ */