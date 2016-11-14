/*
 * cfg_configurationManager.h
 *
 * Created: 11/9/2016 1:12:54 PM
 *  Author: sean
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