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

#include "arm_math.h"

#define VERSION "0.0.0.1"
#define MODEL "Version 1.5"
#define HARDWARE_REV "Proto B"
/*
* Changes from previous version:
* @brief: see VersionNotes.txt for details
*/

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

#define MAX_DEBUG_STRING_LENGTH	200

#define SD_INSERT_WAIT_TIMEOUT	(5 * SECONDS)	


typedef enum 
{
	MODULE_SYSTEM_MANAGER=0,
	MODULE_SDCARD,
	MODULE_WIFI,
	MODULE_CONFIG_MANAGER,
	MODULE_DEBUG,
	MODULE_SUB_PROCESSOR,
	MODULE_BLE,
	MODULE_GPIO_MANAGER,
    MODULE_TFTP_CLIENT,
	MODULE_NUMBER_OF_MODULES
}modules_t;

typedef enum 
{
	STATUS_PASS = 0,
	STATUS_FAIL = 1,
	STATUS_EOF = 2, //end of file, used in getChar	
	STATUS_EAGAIN = 3 //This is used in the get packet, is called when the packet is not complete
}status_t;

#define TASK_SYSTEM_MANAGER_STACK_SIZE			(4072/sizeof(portSTACK_TYPE))
#define TASK_SYSTEM_MANAGER_PRIORITY			(tskIDLE_PRIORITY + 3)
#define TASK_SD_CARD_STACK_SIZE					(3072/sizeof(portSTACK_TYPE))
#define TASK_SD_CARD_PRIORITY					(tskIDLE_PRIORITY + 3)
#define TASK_SENSOR_HANDLER_STACK_SIZE			(3072/sizeof(portSTACK_TYPE))
#define TASK_SENSOR_HANDLER_PRIORITY			(tskIDLE_PRIORITY + 3)
#define TASK_SUB_PROCESS_MANAGER_STACK_SIZE		(6000/sizeof(portSTACK_TYPE))
#define	TASK_SUB_PROCESS_MANAGER_PRIORITY		(tskIDLE_PRIORITY + 3)
#define TASK_DEBUG_MANAGER_STACK_SIZE			(3072/sizeof(portSTACK_TYPE))
#define TASK_DEBUG_MANAGER_PRIORITY				(tskIDLE_PRIORITY + 3)

//static void _EXFUN (putStr, (char * str)) __attribute__((weakref ("puts")));

//Time conversions defines
#define SECONDS									1000		//converts seconds to milliseconds
#define MINS									60 * 1000	//converts minutes to milliseconds

#endif /* COMMON_H_ */
