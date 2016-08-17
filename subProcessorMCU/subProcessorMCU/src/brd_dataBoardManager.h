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

void dat_dataBoardManager(void *pvParameters);
void dat_sendPowerDownReq();

#endif /* BRD_DATABOARDMANAGER_H_ */