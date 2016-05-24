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

typedef struct 
{
	
	FIL fileObj,
	bool fileOpen,
	uint8_t* bufferPointerA,
	uint8_t* bufferPointerB,
	uint32_t bufferIndexA,
	uint32_t bufferIndexB,
	uint16_t bufferSize,		
}sdc_file_t;

typedef enum
{
	SDC_FILE_OPEN_READ_ONLY,
	SDC_FILE_OPEN_READ_WRITE_NEW,
	SDC_FILE_OPEN_READ_WRITE_APPEND,
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

#endif /* SDC_SDCARD_H_ */