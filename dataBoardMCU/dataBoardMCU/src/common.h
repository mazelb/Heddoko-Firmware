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

#define VERSION "V0.1"
/*
* Changes from previous version:
* @brief: see VersionNotes.txt for details
*/

typedef enum 
{
	MODULE_SYSTEM_MANAGER=0,
	MODULE_SDCARD,
	MODULE_SENSOR_HANDLER,
	MODULE_WIFI,
	MODULE_COMMAND,
	MODULE_DEBUG,
	MODULE_NUMBER_OF_MODULES
	
}modules_t;

typedef enum 
{
	STATUS_PASS = 0,
	STATUS_FAIL = 1,
	STATUS_EOF = 2, //end of file, used in getChar	
	STATUS_EAGAIN = 3 //This is used in the get packet, is called when the packet is not complete
}status_t;


#endif /* COMMON_H_ */