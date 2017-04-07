/**
* @file sdc_sdCard.h
* @brief Handler for the SD card
* @author Sean Cloghesy
* @date May 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/

#ifndef SDC_SDCARD_H_
#define SDC_SDCARD_H_

#include <asf.h>
#include "common.h"

#define SD_CD_PIN					PIO_PA17_IDX //TODO add ifdef for new HW 
#define SD_CARD_FILENAME_LENGTH		150
#define MAX_OPEN_FILES				10

typedef enum
{
	SD_FILE_PASS = 0,
	SD_FILE_FAIL,
	SD_FILE_ALREADY_OPEN		//TODO:TBD do we need this
}sd_file_state_t;

typedef enum
{
	SD_CARD_REMOVED = 0, //There is no SD Card Present (based on CD pin)
	SD_CARD_MOUNTED,	 //The SD card has been inserted and mounted
	SD_CARD_MOUNT_ERROR	 //An error has occurred during the mounting process on the SD card. 
}sd_card_status_t;

typedef struct 
{
	FIL fileObj;
	bool fileOpen;
	char fileName[SD_CARD_FILENAME_LENGTH];
	uint8_t* bufferPointerA;
	uint8_t* bufferPointerB;
	uint32_t bufferIndexA;
	uint32_t bufferIndexB;
	uint16_t bufferSize;
	uint8_t* activeBuffer;
	xSemaphoreHandle sem_bufferAccess;
}sdc_file_t;

typedef enum
{
	SDC_FILE_OPEN_READ_ONLY,
	SDC_FILE_OPEN_READ_WRITE_NEW,
	SDC_FILE_OPEN_READ_WRITE_APPEND,
	SDC_FILE_OPEN_READ_WRITE_DATA_LOG,
	SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG	
}sdc_FileOpenMode_t;


typedef struct  
{
    char* serialNumber;     
}sdc_moduleConfig_t;


void sdc_sdCardTask(void *pvParameters);

status_t sdc_openFile(sdc_file_t* fileObject, char* filename, sdc_FileOpenMode_t mode);

status_t sdc_writeToFile(sdc_file_t* fileObject, void* data, size_t size);
status_t sdc_writeDirectToFile(sdc_file_t* fileObject, void* data, size_t fileOffset, size_t length);
status_t sdc_readFromFile(sdc_file_t* fileObject, void* data, size_t fileOffset, size_t length);

status_t sdc_closeFile(sdc_file_t* fileObject);


sd_card_status_t sdc_getSdCardStatus(); 

#endif /* SDC_SDCARD_H_ */