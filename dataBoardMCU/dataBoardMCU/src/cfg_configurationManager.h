/**
* @file cfg_configurationManager.h
* @brief This file contains all code dealing with the handling of configuration
* protocol buffer packets. This module runs a task that is directly responsible
* for opening the configuration socket when the brainpack is connected to wifi.
* The interface to and from the module is ideally through the intra-module messenger.
* @author Sean Cloghesy (sean@heddoko.com)
* @date November 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/
#include "common.h"
#ifndef CFG_CONFIGURATIONMANAGER_H_
#define CFG_CONFIGURATIONMANAGER_H_

typedef struct  
{
    char* serialNumber;
    char* firmwareVersion;    
    uint16_t configPort; 
    
}cfg_moduleConfig_t;

void cfg_configurationTask(void *pvParameters);



#endif /* CFG_CONFIGURATIONMANAGER_H_ */