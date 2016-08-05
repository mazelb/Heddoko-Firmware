/*
 * sts_statusHeartbeat.h
 *
 * Created: 2/29/2016 3:54:57 PM
 *  Author: sean
 */ 


#ifndef STS_STATUSHEARTBEAT_H_
#define STS_STATUSHEARTBEAT_H_

#include "common.h"
#include "drv_gpio.h"
#include "chrg_chargeMonitor.h"
#include "sen_sensorHandler.h"

#pragma pack(push, 1)
typedef struct
{
	uint8_t chargeLevel;	// battery percentage
	chrg_batteryState_t chargerState;
	uint8_t usbCommState;	// 0 - no comm; 1 - comm detected
	uint8_t jackDetectState;		// detected jacks mask
	sensor_state_t streamState;
	uint32_t sensorMask;		// detected sensor mask
}subp_status_t;
#pragma pack(pop)

void sts_getSystemStatus(subp_status_t *sys_status);

#endif /* STS_STATUSHEARTBEAT_H_ */