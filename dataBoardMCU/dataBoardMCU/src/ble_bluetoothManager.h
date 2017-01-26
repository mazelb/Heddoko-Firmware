/*
 * ble_bluetoothManager.h
 *
 * Created: 8/17/2016 4:40:58 PM
 *  Author: sean
 */ 


#ifndef BLE_BLUETOOTHMANAGER_H_
#define BLE_BLUETOOTHMANAGER_H_

#include "net_wirelessNetwork.h"

void ble_bluetoothManagerTask(void *pvParameters);
void ble_startFastAdv();
void ble_sendWiFiConfig(net_wirelessConfig_t *wiFiConfig);
void ble_sendRawData(uint8_t *data, uint8_t size);
void ble_wifiDataReq();
void ble_rawDataReq();

typedef struct  
{
    net_wirelessConfig_t* wirelessConfig;
    char* serialNumber;
    char* fwVersion; //update this to define
    char* hwRevision;
    char* modelString; //the model of the brainpack  
}ble_moduleConfig_t;

#endif /* BLE_BLUETOOTHMANAGER_H_ */