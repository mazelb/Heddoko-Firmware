/**
* @file sys_systemManager.h
* @brief: A free running task which listens to the other modules and sends commands, starts other task threads.
* @author Sean Cloghesy
* @date May 2016
* Copyright Heddoko(TM) 2016, all rights reserved
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
	SYSTEM_STATE_CONNECTING,
	SYSTEM_STATE_WAITING_FOR_CAL,
	SYSTEM_STATE_CALIBRATION,
	SYSTEM_STATE_IDLE,
	SYSTEM_STATE_ERROR,
	SYSTEM_STATE_RECORDING,
	SYSTEM_STATE_STREAMING,
	SYSTEM_STATE_SYNCING,
    SYSTEM_STATE_FIRMWARE_UPDATE
}sys_manager_systemState_t;


void sys_systemManagerTask(void* pvParameters);


#endif /* SYS_SYSTEMMANAGER_H_ */