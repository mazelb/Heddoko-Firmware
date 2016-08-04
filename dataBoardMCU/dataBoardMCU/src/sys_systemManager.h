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
	SYSTEM_STATE_RECORDING,
	SYSTEM_STATE_STREAMING,
	SYSTEM_STATE_SYNCHING
}sys_manager_systemState_t;


void sys_systemManagerTask(void* pvParameters);


#endif /* SYS_SYSTEMMANAGER_H_ */