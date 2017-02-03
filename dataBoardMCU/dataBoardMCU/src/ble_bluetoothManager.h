/**
* @file ble_bluetoothManager.h
* @brief This file contains all code pertaining to the functionality with the on board BLE
* Module. The interface with this module should be done explicitly through the messenger
* module.
* @author Sean Cloghesy (sean@heddoko.com)
* @date August 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/

#ifndef BLE_BLUETOOTHMANAGER_H_
#define BLE_BLUETOOTHMANAGER_H_

#include "net_wirelessNetwork.h"

void ble_bluetoothManagerTask(void *pvParameters);

typedef struct  
{
    net_wirelessConfig_t* wirelessConfig;
    char* serialNumber;
    char* fwVersion; //update this to define
    char* hwRevision;
    char* modelString; //the model of the brainpack  
}ble_moduleConfig_t;

#endif /* BLE_BLUETOOTHMANAGER_H_ */