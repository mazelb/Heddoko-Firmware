/*
 * common.h
 *
 * Created: 9/21/2015 8:34:24 AM
 *  Author: sean
 * @brief: Includes all the generic values used by all files
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#ifndef COMMON_H_
#define COMMON_H_

#define VERSION "V0.1b"

#define ALL_INTERRUPT_MASK  0xffffffff
#define TRUE 1
#define FALSE 0

/** Baudrate setting for all : 115200 */
#define CONF_BAUDRATE   115200
/** Char setting     : 8-bit character length (don't care for UART) */
#define CONF_CHARLENGTH US_MR_CHRL_8_BIT
/** Parity setting   : No parity check */
#define CONF_PARITY     UART_MR_PAR_NO
/** Stopbit setting  : No extra stopbit, i.e., use 1 (don't care for UART) */
#define CONF_STOPBITS   US_MR_NBSTOP_1_BIT

//#define ENABLE_DEBUG_DATA	//enable free running counter instead of actual imu Data.

typedef enum 
{
	STATUS_PASS = 0,
	STATUS_FAIL = 1,
	STATUS_EOF = 2 //end of file, used in getChar	
}status_t;

typedef struct
{
	uint8_t sensorId;
	uint8_t serialNumber[16];
	bool setupModeEnabled;
	bool enableHPR;
	uint32_t baud;
}sensorSettings_t;

/* Board Init configuration */
#define WDT_PERIOD                        10000


/*	task_dataProcessor.c	*/
#define PACKET_WAIT_TIMEOUT						22
#define PACKET_LOSS_COUNT_FOR_RECONNECT			20
#define PACKET_LOSS_COUNT_FOR_ERROR				500

#define WAKEUP_DELAY							(1 * SECONDS)
#define FORCED_SYSTEM_RESET_TIMEOUT				(10 * SECONDS)
#define SLEEP_ENTRY_WAIT_TIME					(4 * SECONDS)
#define MAX_IDLE_TIMEOUT						(5 * MINS)	
#define SD_INSERT_WAIT_TIMEOUT					(5 * SECONDS)	

//Time conversions defines
#define SECONDS									1000	//converts seconds to milli-seconds
#define MINS									60 * 1000	//converts minutes to milli-seconds

int itoa(int value, char* sp, int radix);

#endif /* COMMON_H_ */