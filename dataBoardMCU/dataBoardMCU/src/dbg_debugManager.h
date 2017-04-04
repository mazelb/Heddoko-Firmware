/**
* @file dbg_debugManager.c
* @brief Contains code relevant to debug prints, logging and string command interpreter.
* The string command interpreter listens on connection to a wifi network, through the
* USB and debug com ports.
* @author Sean Cloghesy (sean@heddoko.com)
* @date July 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/

#ifndef DBG_DEBUGMANAGER_H_
#define DBG_DEBUGMANAGER_H_
#include "common.h"

#define DEBUGLOG_MAX_BUFFER_SIZE	100
#define DEBUG_LOG_MAX_FILE_SIZE		2000000ul
typedef enum
{
	DBG_LOG_LEVEL_ERROR,
	DBG_LOG_LEVEL_WARNING,
	DBG_LOG_LEVEL_VERBOSE,
	DBG_LOG_LEVEL_DEBUG
}dbg_debugLogLevel_t;
typedef enum 
{
    DBG_CMD_SOURCE_SERIAL,
    DBG_CMD_SOURCE_NET,
    DBG_CMD_SOURCE_USB
}dbg_commandSource_t;

typedef struct  
{
    dbg_debugLogLevel_t logLevel;
    uint16_t debugPort; 
}dbg_debugConfiguration_t;

void dbg_debugTask(void* pvParameters);
status_t dbg_processCommand(dbg_commandSource_t source, char* command, size_t cmdSize);
void dbg_printf(dbg_debugLogLevel_t msgLogLevel, char *fmt, ...);
void dbg_printString(dbg_debugLogLevel_t msgLogLevel, char* string);



#endif /* DBG_DEBUGMANAGER_H_ */