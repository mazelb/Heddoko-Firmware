/*
 * sys_systemManager.h
 *
 * Created: 5/24/2016 3:37:27 PM
 *  Author: sean
 */ 


#ifndef SYS_SYSTEMMANAGER_H_
#define SYS_SYSTEMMANAGER_H_

#include <asf.h>
#include "sdc_sdCard.h"
#include "drv_uart.h"

typedef enum
{
	SYSTEM_STATE_SLEEP = 0,
	SYSTEM_STATE_INIT,
	SYSTEM_STATE_IDLE,
	SYSTEM_STATE_ERROR,
	SYSTEM_STATE_RECORDING
}sys_manager_systemState_t;

typedef struct  
{
	sys_manager_systemState_t sysState;
	uint8_t sysErrCode;
	uint16_t modulesReadyMask;
	uint16_t modulesErrorMask;
	sd_message_type_t sdCardState;
	bool debugPrintsEnabled;
}system_status_t;

typedef struct  
{
	drv_uart_config_t *sensor_port;		// com port to interface the sensor 
	drv_uart_config_t *ble_port;		// com port for communicating to BLE module
	drv_uart_config_t *wifi_port;		// com port allocated to the wifi module
	drv_uart_config_t *consoleUart;		// UART to print out the debug information
	drv_uart_config_t *dataOutUart;		// UART to print out the data
}system_port_config_t;

void systemManager(void* pvParameters);
void __attribute__((optimize("O0"))) debugPrintString(char* str);
void __attribute__((optimize("O0"))) debugPrintStringInt(char* str, int number);

#endif /* SYS_SYSTEMMANAGER_H_ */