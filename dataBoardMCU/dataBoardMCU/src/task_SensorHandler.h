/*
 * task_SensorHandler.h
 *
 * Created: 2016-04-18 12:43:59 PM
 *  Author: Hriday Mehta
 */ 


#ifndef TASK_SENSORHANDLER_H_
#define TASK_SENSORHANDLER_H_


#include <asf.h>
#include "common.h"
#include "drv_gpio.h"

#define GPIO_RS485_DATA_DIRECTION_RE	DRV_GPIO_PIN_BLE_RST3
#define GPIO_RS485_DATA_DIRECTION_DE	DRV_GPIO_PIN_BLE_RST3

#define PACKET_PADDING_LENGTH			UART_DMA_IMMEDIATE_PROCESSING_LENGTH
#define SENSOR_BUS_SPEED_HIGH			2000000
#define SENSOR_BUS_SPEED_LOW			460800
//#define ENABLE_SENSOR_PACKET_TEST		//enable this define to switch to the debug packet and check the integrity of data

typedef enum
{
	SENSOR_READY,
	SENSOR_STANDBY,
	SENSOR_NOT_PRESENT,
	SENSOR_COMM_ERROR,	//if it receives incomplete packets
}sensor_state_t;

typedef enum
{
	SENSOR_DISCONNECTED = 0,
	SENSOR_PACKET_INCOMPLETE,
}sensor_error_code_t;

void sendGetFrame(int sensorId);
void sendSetupModeEnable();
void sendUpdateCommand();
void sendGetDebugStatus();
void sendUpdateCommandFake();
void sendResetCommandFake();
void sendEnableHPR(uint8_t enable);
void sendChangeBaud(uint32_t baud);
void sendChangePadding(bool paddingEnable, uint8_t paddingLength);

void task_SensorHandler(void *pvParameters); 
void task_protoPacketHandler(void *pvParameters); 

#endif /* TASK_NEWSENSORHANDLER_H_ */