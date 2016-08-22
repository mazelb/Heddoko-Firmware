/*
 * dbg_debugManager.h
 *
 * Created: 7/12/2016 10:25:54 AM
 *  Author: sean
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
}dgb_debugLogLevel_t;

void dbg_debugTask(void* pvParameters);
void dgb_printf(dgb_debugLogLevel_t msgLogLevel, char *fmt, ...);
void dbg_printString(dgb_debugLogLevel_t msgLogLevel, char* string);



#endif /* DBG_DEBUGMANAGER_H_ */