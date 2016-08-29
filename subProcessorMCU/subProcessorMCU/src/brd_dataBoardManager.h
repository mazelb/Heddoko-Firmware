/*
 * brd_dataBoardManager.h
 *
 * Created: 8/1/2016 10:36:34 AM
 *  Author: Hriday Mehta
 */ 


#ifndef BRD_DATABOARDMANAGER_H_
#define BRD_DATABOARDMANAGER_H_

#include "common.h"

/*	Date and time structure for the power board	*/
#pragma pack(push, 1)
typedef struct
{
	uint32_t time;  // 0-6 bits: seconds from 0 to 59 in BCD
					// 8-14 bits: minutes from 0 to 59 in BCD
					// 16-21 bits: hours from (1 to 12) or (0 to 23) in BCD
					// 22 bit: 0 for AM, 1 for PM

	uint32_t date;  // 0-6 bits: century from 19 to 20 in BCD
					// 8-15 bits: year from 0 to 99 in BCD
					// 16-20 bits: month from 1 to 12 in BCD
					// 21-23 bits: day from 1 to 7 in BCD
					// 24-29 bits: date from 1 to 31 in BCD

}subp_dateTime_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	uint8_t chargeLevel;	// battery percentage
	uint8_t chargerState;
	uint8_t usbCommState;	// 0 - no comm; 1 - comm detected
	uint8_t jackDetectState;		// detected jacks mask
	uint8_t streamState;
	uint32_t sensorMask;		// detected sensor mask
}subp_status_t;
#pragma pack(pop)

void brd_dataBoardManager(void *pvParameters);
void brd_sendPowerDownReq();

#endif /* BRD_DATABOARDMANAGER_H_ */