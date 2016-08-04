/*
 * sdc_sdCard.h
 *
 * Created: 5/24/2016 11:18:08 AM
 * Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#ifndef SDC_SDCARD_H_
#define SDC_SDCARD_H_

#include <asf.h>
#include "common.h"

#define SD_CD_PIN					PIO_PC12_IDX


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

/***********************************************************************************************
 * sdc_sdCardTask(void *pvParameters)
 * @brief The task that handles communication with the SD card. This task initializes the SD card,
 *	and provides asynchronous writes to the SD card. 
 * @param pvParameters, null right now, but can change. 
 * @return void
 ***********************************************************************************************/
void sdc_sdCardTask(void *pvParameters);
/***********************************************************************************************
 * sdc_openFile(sdc_file_t* fileObject, char* filename, sdc_FileOpenMode_t mode);
 * @brief Opens a file based on which mode is set, and the filename. Populates the fileObject,
	allocates the memory for the file buffers
 * @param fileObject, filename, mode
 * @return void
 ***********************************************************************************************/
status_t sdc_openFile(sdc_file_t* fileObject, char* filename, sdc_FileOpenMode_t mode);
/***********************************************************************************************
 * sdc_writeToFile(sdc_file_t* fileObject, void* data, size_t size)
 * @brief Write to a file, this does not write directly to the file, but to a buffer.  
 * @param fileObject, data: pointer to data buffer, size: number of bytes to write
 * @return void
 ***********************************************************************************************/
status_t sdc_writeToFile(sdc_file_t* fileObject, void* data, size_t size);
status_t sdc_readFromFile(sdc_file_t* fileObject, void* data, size_t fileOffset, size_t length);
/***********************************************************************************************
 * sdc_closeFile(sdc_file_t* fileObject)
 * @brief Asynchronously closes the file, frees the memory used by the buffers.  
 * @param fileObject
 * @return void
 ***********************************************************************************************/
status_t sdc_closeFile(sdc_file_t* fileObject);
/***********************************************************************************************
 * sdc_checkFileSize(sdc_file_t* fileObject, size_t *size)
 * @brief Checks the size of the file specified  
 * @param fileObject, pointer to the size variable
 * @return void
 ***********************************************************************************************/
status_t sdc_checkFileSize(sdc_file_t* fileObject, size_t *size);

#endif /* SDC_SDCARD_H_ */