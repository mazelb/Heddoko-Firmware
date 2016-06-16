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
//#include "imu.h"
//#include "pkt_packetParser.h"
//#include "cmd_commandProcessor.h"
#include "drv_gpio.h"

#define GPIO_RS485_DATA_DIRECTION_RE	DRV_GPIO_PIN_BLE_RST3
#define GPIO_RS485_DATA_DIRECTION_DE	DRV_GPIO_PIN_BLE_RST3

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

void task_SensorHandler(void *pvParameters); 
void task_protoPacketHandler(void *pvParameters); 

#endif /* TASK_NEWSENSORHANDLER_H_ */