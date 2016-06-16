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

typedef struct  
{
	uint8_t sysState;
	uint8_t sysErrCode;
	uint16_t modulesReadyMask;
	uint16_t modulesErrorMask;
	sd_message_type_t sdCardState;
	bool debugPrintsEnabled;
}system_status_t;

void systemManager(void* pvParameters);
void __attribute__((optimize("O0"))) debugPrintString(char* str);
void __attribute__((optimize("O0"))) debugPrintStringInt(char* str, int number);

#endif /* SYS_SYSTEMMANAGER_H_ */