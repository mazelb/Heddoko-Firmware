/*
 * task_sdCardWrite.h
 *
 * Created: 10/19/2015 8:21:00 AM
 *  Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#ifndef TASK_SDCARDWRITE_H_
#define TASK_SDCARDWRITE_H_
#include "common.h"
#define SD_CARD_BUFFER_SIZE 3072
#define DEBUG_LOG_BUFFER_SIZE 512
#define SD_CARD_FILENAME_LENGTH 150
#define DEBUG_LOG_MAX_FILE_SIZE 2000000ul

//Log file creation algorithms
#define FILE_CREATION_ALGO		INDIVIDUAL_DIRECTORY
#define COMMON_INDEX			0
#define INDIVIDUAL_INDEX		1
#define INDIVIDUAL_DIRECTORY	2

void task_sdCardHandler(void *pvParameters);
status_t task_sdCardWriteEntry(char* entry, size_t length);
status_t task_debugLogWriteEntry(char* entry, size_t length);
status_t task_sdCard_OpenNewFile();
status_t task_debugLog_OpenFile();
status_t task_sdCard_CloseFile();
status_t task_debugLog_CloseFile();
status_t task_debugLog_ChangeFile();
Bool task_sdCard_isDataLogOpen();

#endif /* TASK_SDCARDWRITE_H_ */