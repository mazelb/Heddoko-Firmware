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

#define VERSION "V0.1"
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


//interprocessor message types
#define PACKET_TYPE_MASTER_CONTROL 0x01
#define PACKET_TYPE_IMU_SENSOR	   0x03
#define PACKET_TYPE_PROTO_BUF	   0x04
#define PACKET_TYPE_SUB_PROCESSOR  0x05
#define PACKET_TYPE_BLE_MODULE	   0x06


//Interprocessor command IDs

/*	IMU Sensor module commands	*/
#define PACKET_CMD_ID_IMU_UPDATE					0x11
#define PACKET_CMD_ID_IMU_GET_FRAME					0x12
#define PACKET_CMD_ID_IMU_GET_FRAME_RESP			0x13
#define PACKET_CMD_ID_IMU_SETUP_MODE				0x14
#define PACKET_CMD_ID_IMU_BUTTON_PRESS				0x15
#define PACKET_CMD_ID_IMU_SET_IMU_ID				0x16
#define PACKET_CMD_ID_IMU_SET_IMU_ID_RESP			0x17
#define PACKET_CMD_ID_IMU_GET_STATUS				0x18
#define PACKET_CMD_ID_IMU_GET_STATUS_RESP			0x19
#define PACKET_CMD_ID_IMU_RESET_FAKE				0x20
#define PACKET_CMD_ID_IMU_UPDATE_FAKE				0x21
#define PACKET_CMD_ID_IMU_ENABLE_HPR				0x22
#define PACKET_CMD_ID_IMU_CHANGE_BAUD				0x23
#define PACKET_CMD_ID_IMU_SET_RATES					0x24
#define PACKET_CMD_ID_IMU_CHANGE_PADDING			0x25
/* Power Board / subprocessor commands */
#define PACKET_CMD_ID_SUBP_GET_STATUS				0x51
#define PACKET_CMD_ID_SUBP_GET_STATUS_RESP			0x52
#define PACKET_CMD_ID_SUBP_SET_CONFIG				0x53
#define PACKET_CMD_ID_SUBP_STREAM_ENABLE			0x54
#define PACKET_CMD_ID_SUBP_FULL_FRAME				0x55
#define PACKET_CMD_ID_SUBP_PWR_DOWN_REQ				0x56
#define PACKET_CMD_ID_SUBP_PWR_DOWN_RESP			0x57

/*	Bluetooth module commands	TODO Refactor these commands*/
#define PACKET_COMMAND_ID_GPS_DATA_REQ              0x41
#define PACKET_COMMAND_ID_GPS_DATA_RESP             0x42
//#define PACKET_COMMAND_ID_SSID_DATA_REQ             0x43
//#define PACKET_COMMAND_ID_SSID_DATA_RESP            0x44
//#define PACKET_COMMAND_ID_PASSPHRASE_DATA_REQ       0x45
//#define PACKET_COMMAND_ID_PASSPHRASE_DATA_RESP      0x46
//#define PACKET_COMMAND_ID_SECURITY_TYPE_DATA_REQ    0x47
//#define PACKET_COMMAND_ID_SECURITY_TYPE_DATA_RESP   0x48
#define PACKET_COMMAND_ID_DEFAULT_WIFI_DATA         0x48
#define PACKET_COMMAND_ID_ALL_WIFI_DATA_REQ			0x49
#define PACKET_COMMAND_ID_ALL_WIFI_DATA_RESP		0x4a
#define PACKET_COMMAND_ID_SEND_RAW_DATA_TO_BLE      0x4b
#define PACKET_COMMAND_ID_SEND_RAW_DATA_TO_MASTER   0x4c
#define PACKET_COMMAND_ID_GET_RAW_DATA_REQ          0x4d
#define PACKET_COMMAND_ID_GET_RAW_DATA_RESP         0x4e
#define PACKET_COMMAND_ID_START_FAST_ADV            0x4f

typedef enum 
{
	MODULE_SYSTEM_MANAGER=0,
	MODULE_SDCARD,
	MODULE_SENSOR_HANDLER,
	MODULE_WIFI,
	MODULE_COMMAND,
	MODULE_DEBUG,
	MODULE_SUB_PROCESSOR,
	MODULE_DATA_MANAGER,
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
#define TASK_SUB_PROCESS_MANAGER_STACK_SIZE		(3072/sizeof(portSTACK_TYPE))
#define	TASK_SUB_PROCESS_MANAGER_PRIORITY		(tskIDLE_PRIORITY + 3)
#define TASK_DEBUG_MANAGER_STACK_SIZE			(3072/sizeof(portSTACK_TYPE))
#define TASK_DEBUG_MANAGER_PRIORITY				(tskIDLE_PRIORITY + 3)

//static void _EXFUN (putStr, (char * str)) __attribute__((weakref ("puts")));

//Time conversions defines
#define SECONDS									1000		//converts seconds to milliseconds
#define MINS									60 * 1000	//converts minutes to milliseconds

#endif /* COMMON_H_ */