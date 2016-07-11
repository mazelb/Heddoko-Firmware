/*
 * ble_cypressModule.h
 *
 * Created: 2016-06-16 11:41:08 AM
 *  Author: Hriday Mehta
 */ 


#ifndef BLE_CYPRESSMODULE_H_
#define BLE_CYPRESSMODULE_H_

#include <string.h>
#include "common.h"
#include "drv_uart.h"
#include "cmd_commandProcessor.h"

#define MAX_BLE_COMMAND_PACKET_SIZE	2
#define MAX_RAW_DATA_PACKET_SIZE	20

typedef enum
{
	BLE_WIFI_DATA_SSID = 0,
	BLE_WIFI_DATA_PASSPHRASE,
	BLE_WIFI_DATA_SECURITY_TYPE,
	BLE_WIFI_DATA_ALL
}ble_wifi_data_categories_t;

status_t ble_init(drv_uart_config_t* uartConfig);
void ble_processPacket(rawPacket_t* packet);
void ble_send_gpsDataReq ();
void ble_send_wifiDataReq (ble_wifi_data_categories_t dataCategory);
void ble_send_rawDataReq ();
void ble_send_rawData (uint8_t *data);

#endif /* BLE_CYPRESSMODULE_H_ */