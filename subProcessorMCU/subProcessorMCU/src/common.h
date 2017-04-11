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

#define VERSION "0.0.0.1"
//#define CHARGER_TEST_MODE 
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



typedef enum 
{
	STATUS_PASS = 0,
	STATUS_FAIL = 1,
	STATUS_EOF = 2, //end of file, used in getChar	
	STATUS_EAGAIN = 3 //This is used in the get packet, is called when the packet is not complete
}status_t;

//TASK Dependent defines
//Higher priority means higher priority
#define TASK_CHRG_MON_STACK_SIZE			 (3072/sizeof(portSTACK_TYPE))
#define TASK_CHRG_MON_STACK_PRIORITY         (tskIDLE_PRIORITY + 2)

#define TASK_DATA_ROUTER_STACK_SIZE          (3072/sizeof(portSTACK_TYPE))
#define TASK_DATA_ROUTER_PRIORITY            (tskIDLE_PRIORITY + 6)

#define TASK_MANAGER_STACK_SIZE				 (2048/sizeof(portSTACK_TYPE))
#define TASK_MANAGER_PRIORITY				 (tskIDLE_PRIORITY + 4)

#define TASK_HEARTBEAT_STACK_SIZE            (1024/sizeof(portSTACK_TYPE))
#define TASK_HEARTBEAT_PRIORITY				 (tskIDLE_PRIORITY + 5)

#define TASK_COMMAND_PROC_STACK_SIZE         (2048/sizeof(portSTACK_TYPE))
#define TASK_COMMAND_PROC_PRIORITY			 (tskIDLE_PRIORITY + 5)

#define TASK_SENSOR_HANDLER_STACK_SIZE		 (3072/sizeof(portSTACK_TYPE))
#define TASK_SENSOR_HANDLER_PRIORITY		 (tskIDLE_PRIORITY + 6)

#define TASK_DATA_BOARD_MANAGER_STACK_SIZE	 (2048/sizeof(portSTACK_TYPE))
#define TASK_DATA_BOARD_MANAGER_PRIORITY	 (tskIDLE_PRIORITY + 3)

/* Board Init configuration */
#define WDT_PERIOD                        10000

#define POWER_BOARD_CMD_TOGGLE_JACKS 0xAA
#define POWER_BOARD_CMD_GET_TIME 0xAC

/*	task_dataProcessor.c	*/
#define PACKET_WAIT_TIMEOUT						22
#define PACKET_LOSS_COUNT_FOR_RECONNECT			20
#define PACKET_LOSS_COUNT_FOR_ERROR				500

#define WAKEUP_DELAY							(1 * SECONDS)
#define FORCED_SYSTEM_RESET_TIMEOUT				(10 * SECONDS)
#define SLEEP_ENTRY_WAIT_TIME					(1500)
#define MAX_IDLE_TIMEOUT						(5 * MINS)	
#define SD_INSERT_WAIT_TIMEOUT					(5 * SECONDS)	

//Time conversions defines
#define SECONDS									1000	//converts seconds to milli-seconds
#define MINS									60 * 1000	//converts minutes to milli-seconds

//EM7180 defines
#define	EM_TWI_MASTER	TWI0

//Charger percentages
#define BATTERY_PERCENT_LOW 15
#define BATTERY_PERCENT_CRITICAL 8
#define BATTERY_PERCENT_FAULT 3

//int itoa(int value, char* sp, int radix);


#endif /* COMMON_H_ */