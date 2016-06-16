/*
 * sdc_sdCard.c
 *
 * Created: 5/24/2016 11:17:49 AM
 * Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 
#include <asf.h>
#include "sdc_sdCard.h"
#include "string.h"
#include "sd_mmc_mem.h"
#include "msg_messenger.h"
#include "sys_systemManager.h"

//Static function forward declarations
bool sdCardPresent();
//initializes the SD card, and mounts the drive
status_t initializeSdCard();
//uninitializes the SD card, and puts the drive in an uninitialized state. 
status_t unInitializeSdCard();
//write calls from the main thread
status_t write_to_sd_card(sdc_file_t *fileObject);
//change the active buffer safely
void changeActiveBuffer(sdc_file_t *fileObject);
//register newly open file in the array
void registerOpenFile(sdc_file_t* fileObject);
//unregister closed file in the array
void unregisterOpenFile(sdc_file_t* fileObject);
//close all open files
void closeAllFiles();
//re-initialize the SD-card module and pass message
void cardInsertedCall();
//unmount the SD-card module and pass message
void cardRemovedCall();

FIL dataLogFile_obj, debugLogFile_Obj;
xSemaphoreHandle semaphore_fatFsAccess = NULL, semaphore_bufferAccess = NULL;
xQueueHandle queue_sdCard = NULL;
char debugLogNewFileName[] = "sysHdk", debugLogOldFileName[] = "sysHdk_old";
char dataLogFileName[SD_CARD_FILENAME_LENGTH] = {0};
open_files_t openFilesList;
	
//extern variables
extern sdc_file_t dataLogFile, debugLogFile;
extern system_status_t systemStatus;

static void processEvent(msg_message_t message)
{
	msg_sys_manager_t* sys_eventData;
	sys_eventData = (msg_sys_manager_t *) message.parameters;
	
	switch (message.type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
			if (message.source == MODULE_SYSTEM_MANAGER)
			{
				if ((sys_eventData->mountSD == false) && (sys_eventData->unmountSD == true))
				{
					if (systemStatus.sdCardState != SD_CARD_REMOVED)
						cardRemovedCall();
				}
				else if ((sys_eventData->mountSD == true) && (sys_eventData->unmountSD == false))
				{
					if ((sdCardPresent()) && (systemStatus.sdCardState != SD_CARD_INITIALIZED))	//TODO: make sure these conditions don't pose a problem
						cardInsertedCall();
				}
			}
		break;
		case MSG_TYPE_SDCARD_STATE:
		case MSG_TYPE_READY:
		case MSG_TYPE_ERROR:
		case MSG_TYPE_SDCARD_SETTINGS:
		case MSG_TYPE_WIFI_STATE:
		case MSG_TYPE_WIFI_SETTINGS:
		case MSG_TYPE_USB_CONNECTED:
		case MSG_TYPE_CHARGER_EVENT:
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		default:
		break;
	}
	
	free(message.parameters);
}

void sdc_sdCardTask(void *pvParameters)
{
	FRESULT res;
	UINT numBytesWritten = 0;
	uint32_t totalBytesWritten = 0, totalBytesToWrite = 0;
	status_t status = STATUS_PASS;
	bool sd_val = TRUE, sd_oldVal = FALSE;
	msg_message_t eventMessage;
	
	queue_sdCard = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_sdCard != 0)
	{
		msg_registerForMessages(MODULE_SDCARD, 0xff, queue_sdCard);
	}
	
	semaphore_fatFsAccess = xSemaphoreCreateMutex();
	semaphore_bufferAccess = xSemaphoreCreateMutex();
	
	sd_oldVal = ioport_get_pin_level(SD_CD_PIN);
	
	while (1)
	{
		sd_val = ioport_get_pin_level(SD_CD_PIN);
		if (sd_val != sd_oldVal)
		{
			if (sd_val == true)
			{
				cardRemovedCall();
			}
			else
			{
				cardInsertedCall();
			}
		}
		sd_oldVal = sd_val;
		
		if (xSemaphoreTake(semaphore_fatFsAccess, 1) == true)
		{
			for (int i = 0; i < MAX_OPEN_FILES; i++)
			{
				write_to_sd_card(openFilesList.openFilesArray[i]);
			}
			xSemaphoreGive(semaphore_fatFsAccess);
		}
		else
		{
			puts("Failed to get semaphore - SD-card main thread\r");
		}
		
		if (xQueueReceive(queue_sdCard, &eventMessage, 1) == true)
		{
			processEvent(eventMessage);
		}
		vTaskDelay(100);
	}
}

status_t write_to_sd_card(sdc_file_t *fileObject)
{
	FRESULT res;
	UINT numBytesWritten = 0;
	uint32_t totalBytesWritten = 0, totalBytesToWrite = 0;
	status_t status;
	
	if (fileObject->fileOpen)
	{
		if (xSemaphoreTake(semaphore_bufferAccess, 10) == true)
		{
			changeActiveBuffer(fileObject);
			xSemaphoreGive(semaphore_bufferAccess);
		}
		else
		{
			puts("Failed to get semaphore - Buffer read\r");
		}
		
		if (fileObject->activeBuffer == fileObject->bufferPointerA)
		{
			//use buffer B as we just toggled the buffer
			if (fileObject->bufferIndexB > 0)
			{
				totalBytesToWrite = fileObject->bufferIndexB;
				numBytesWritten = 0;
				totalBytesWritten = 0;
				while (totalBytesToWrite > 0)
				{
					res = f_write(&fileObject->fileObj, (void*)fileObject->bufferPointerB + totalBytesWritten,
					totalBytesToWrite,	&numBytesWritten);
					if (res != FR_OK)
					{
						puts("Write to SD-card failed\r");
						status = STATUS_FAIL;
						break;
					}
					res = f_sync(&fileObject->fileObj);
					totalBytesToWrite -= numBytesWritten;
					totalBytesWritten += numBytesWritten;
					vTaskDelay(1);
				}
				if (totalBytesToWrite == 0)
				{
					fileObject->bufferIndexB = 0;
				}
			}
		}
		else
		{
			//use buffer A as we just toggled the buffer
			if (fileObject->bufferIndexA > 0)
			{
				totalBytesToWrite = fileObject->bufferIndexA;
				numBytesWritten = 0;
				totalBytesWritten = 0;
				while (totalBytesToWrite > 0)
				{
					res = f_write(&fileObject->fileObj, (void*)fileObject->bufferPointerA + totalBytesWritten,
					totalBytesToWrite, &numBytesWritten);
					if (res != FR_OK)
					{
						puts("Write to SD-card failed\r");
						status = STATUS_FAIL;
						break;
					}
					res = f_sync(&fileObject->fileObj);
					totalBytesToWrite -= numBytesWritten;
					totalBytesWritten += numBytesWritten;
					vTaskDelay(1);
				}
				if (totalBytesToWrite == 0)
				{
					fileObject->bufferIndexA = 0;
				}
			}
		}
	}
}

status_t sdc_writeToFile(sdc_file_t* fileObject, void* data, size_t size)
{
	status_t status = STATUS_PASS;
	
	if (fileObject->fileOpen == false)	//only write to buffers if the file is open.
	{
		status = STATUS_FAIL;
		return status;
	}
	
	if (xSemaphoreTake(semaphore_bufferAccess, 1) ==  true)
	{
		if (fileObject->activeBuffer == fileObject->bufferPointerA)
		{
			if (fileObject->bufferIndexA + size < fileObject->bufferSize)
			{
				memcpy(fileObject->bufferPointerA + fileObject->bufferIndexA, data, size);
				fileObject->bufferIndexA += size;
			}
			else
			{	
				status = STATUS_FAIL;
			}
		}
		else
		{
			if (fileObject->bufferIndexB + size < fileObject->bufferSize)
			{
				memcpy(fileObject->bufferPointerB + fileObject->bufferIndexB, data, size);
				fileObject->bufferIndexB += size;
			}
			else
			{
				status = STATUS_FAIL;
			}
		}
		xSemaphoreGive(semaphore_bufferAccess);
	}
	else
	{
		status = STATUS_FAIL;
	}
	return status;
}

status_t sdc_readFromFile(sdc_file_t* fileObject, void* data, size_t fileOffset, size_t length)
{
	status_t status = STATUS_PASS;
	FRESULT res;
	UINT numBytesRead = 0;
	
	if (fileObject->fileOpen == true)
	{
		if (xSemaphoreTake(semaphore_fatFsAccess, 1) == true)
		{
			//assuming the file was already opened by calling sdc_openFile
			res = f_lseek(&fileObject->fileObj, (DWORD)fileOffset);
			res = f_read(&fileObject->fileObj, data, length, &numBytesRead);
			if (res != FR_OK)
			{
				status = STATUS_FAIL;
			}
			xSemaphoreGive(semaphore_fatFsAccess);
		}
		else
		{
			status = STATUS_FAIL;
		}
	}
	return status;
}

status_t sdc_openFile(sdc_file_t* fileObject, char* filename, sdc_FileOpenMode_t mode)
{
	status_t status = STATUS_PASS;
	FRESULT res;
	FILINFO vLogFileInfo;
	char fileIndexLog[SD_CARD_FILENAME_LENGTH] = "0:logIndex.dat";
	char logFileName[SD_CARD_FILENAME_LENGTH] = {0};
	char dirName[SD_CARD_FILENAME_LENGTH] = "0:MovementLog";
	char dirPath[] = "0:MovementLog/";
	uint8_t data_buffer[100] = {0}, openMode = 0;
	uint16_t fileIndexNumber = 0, byte_read = 0, bytes_written = 0;
	DIR dir;
	FIL indexFile_obj;
	
	//return if the file is already open
	if (fileObject->fileOpen == true)
	{
		status = STATUS_PASS;	//TODO: should we pass "already open" 
		return status;
	}
	
	//open a new file
	if (xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
	{
		if (mode == SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG)
		{
			//this is a debug log file
			//TODO: add the file swap feature after confirmation
			snprintf(logFileName, SD_CARD_FILENAME_LENGTH, "0:%s.bin", filename);
			res = f_open(&fileObject->fileObj, (char const*)logFileName, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
			if (res == FR_OK)
			{
				puts("DebugLog open\r");
				res = f_lseek(&fileObject->fileObj, fileObject->fileObj.fsize);
				fileObject->fileOpen = true;
				status = STATUS_PASS;
			}
			else
			{
				puts("Failed to open DebugLog\r");
				status = STATUS_FAIL;
			}
		}
		else if (mode == SDC_FILE_OPEN_READ_WRITE_DATA_LOG)
		{
			//this is a dataLog file
			snprintf(dirName, SD_CARD_FILENAME_LENGTH, "0:%s", filename);
			res = f_opendir(&dir, &dirName);	//open the specified directory
			if (res == FR_NO_PATH)
			{
				res = f_mkdir(&dirName);	//the requested directory doesn't exist, create new one
				if (res != FR_OK)
				{
					status = STATUS_FAIL;
					puts("Failed on creating new movement log directory\r");
					return status;
				}
			}
			snprintf(logFileName, SD_CARD_FILENAME_LENGTH, "%s/%s", dirName, &fileIndexLog[2]);
			strncpy(fileIndexLog, logFileName, sizeof(logFileName));
			res = f_open(&indexFile_obj, (char const *)fileIndexLog, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
			if (res != FR_OK)
			{
				status = STATUS_FAIL;
				puts("Failed on creating new movement log file\r");
			}
			if(status == STATUS_PASS)
			{
				//if the filesize is 0, it means it's never been created, set index to 1.
				if(indexFile_obj.fsize == 0)
				{
					fileIndexNumber = 1;
				}
				else
				{
					if(f_read(&indexFile_obj, (void*)data_buffer, 100, &byte_read) != FR_OK)
					{
						status = STATUS_FAIL;
					}
					if(status == STATUS_PASS)
					{
						sscanf(data_buffer,"%d\r\n",&fileIndexNumber);
						fileIndexNumber++;
					}
					
				}
			}
			if(status == STATUS_PASS)
			{
				//write the update index back to the file.
				sprintf(data_buffer, "%05d\r\n", fileIndexNumber);
				f_lseek(&indexFile_obj,0);
				if(f_write(&indexFile_obj, (void*)data_buffer,strlen(data_buffer),&bytes_written) != FR_OK)
				{
					status = STATUS_FAIL;
				}
				else
				{
					f_close(&indexFile_obj);
				}
			}
			
			if(status == STATUS_PASS)
			{
				//create the filename
				//if(nvmSettings.enableCsvFormat == 0)
				//{
					snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "%s/%s_%s%05d.dat",dirName, "S00001", filename, fileIndexNumber);
				//}
				//else
				//{
					//snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "%s/%s_%s%05d.csv",dirName, brainSettings.suitNumber, brainSettings.fileName, fileIndexNumber);
				//}
				
				res = f_open(&fileObject->fileObj, (char const *)dataLogFileName, FA_OPEN_ALWAYS | FA_WRITE);
				if (res == FR_OK)
				{
					puts(dataLogFileName);
					//res = f_lseek(&fileObject->fileObj, fileObject->fileObj.fsize);
					fileObject->fileOpen = true;
				}
				else
				{
					itoa(res, data_buffer, 10);
					puts(data_buffer);
					puts("Data log failed to open\r");
					status = STATUS_FAIL;
				}
			}
		}
		else
		{
			if (mode == SDC_FILE_OPEN_READ_WRITE_NEW)
			{
				openMode = FA_CREATE_NEW | FA_WRITE | FA_READ;
			}
			else if (mode == SDC_FILE_OPEN_READ_WRITE_APPEND)
			{
				openMode = FA_OPEN_ALWAYS | FA_WRITE | FA_READ;
			}
			else
			{
				openMode = FA_OPEN_EXISTING | FA_READ; 
			}
			snprintf(logFileName, SD_CARD_FILENAME_LENGTH, "0:%s.bin", filename);
			res = f_open(&fileObject->fileObj, (char const*)logFileName, openMode);
			if (res == FR_OK)
			{
				res = f_lseek(&fileObject->fileObj, fileObject->fileObj.fsize);
				fileObject->fileOpen = true;
				status = STATUS_PASS;
			}
			else
			{
				puts("Failed to open file\r");
				status = STATUS_FAIL;
			}
		}
		if (status == STATUS_PASS)
		{
			registerOpenFile(fileObject);
		}
		xSemaphoreGive(semaphore_fatFsAccess);
	}
	else
	{
		puts("Failed to get Semaphore to open file\r");
		return STATUS_FAIL;
	}
	
	return status;
}

status_t sdc_closeFile(sdc_file_t* fileObject)
{
	status_t status = STATUS_PASS;
	FRESULT res;
	//return if the file is already closed
	if (fileObject->fileOpen == false)
	{
		status = STATUS_FAIL; 
		return status;
	}
	
	if (xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
	{
		res = f_sync(&fileObject->fileObj);
		res = f_close(&fileObject->fileObj);
		if (res != FR_OK)
		{
			status = STATUS_FAIL;
		}
		fileObject->fileOpen = false;
		unregisterOpenFile(fileObject);
		xSemaphoreGive(semaphore_fatFsAccess);
	}
	else
	{
		puts("Failed to get semaphore - SD-card close file\r");
		status = STATUS_FAIL;
	}
	
	return status;
}

status_t initializeSdCard()
{
	static FATFS fs;
	static FATFS* fs1;	//pointer to FATFS structure used to check free space
	static FRESULT res;
	static DWORD freeClusters, freeSectors, totalSectors;
	status_t result = STATUS_PASS;
	Ctrl_status status = STATUS_FAIL;
	char test_file_name[] = "0:sd_mmc_test.txt";
	FIL file_object;
	
	gpio_configure_pin(SD_MMC_0_CD_GPIO, SD_MMC_0_CD_FLAGS);
	/* Configure HSMCI pins */
	gpio_configure_pin(PIN_HSMCI_MCCDA_GPIO, PIN_HSMCI_MCCDA_FLAGS);
	gpio_configure_pin(PIN_HSMCI_MCCK_GPIO, PIN_HSMCI_MCCK_FLAGS);
	gpio_configure_pin(PIN_HSMCI_MCDA0_GPIO, PIN_HSMCI_MCDA0_FLAGS);
	gpio_configure_pin(PIN_HSMCI_MCDA1_GPIO, PIN_HSMCI_MCDA1_FLAGS);
	gpio_configure_pin(PIN_HSMCI_MCDA2_GPIO, PIN_HSMCI_MCDA2_FLAGS);
	gpio_configure_pin(PIN_HSMCI_MCDA3_GPIO, PIN_HSMCI_MCDA3_FLAGS);
	
	sd_mmc_init();
	do
	{
		status = sd_mmc_test_unit_ready(0);
		if (CTRL_FAIL == status)
		{
			puts("Card install FAIL\n\r");
			puts("Please unplug and re-plug the card.\n\r");
			while (CTRL_NO_PRESENT != sd_mmc_check(0))
			{
				;
			}
		}
	} while (CTRL_GOOD != status);
	
	/*	Mount the SD card	*/
	if(status == CTRL_GOOD)
	{
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		if (res == FR_INVALID_DRIVE)
		{
			result = STATUS_FAIL;
			puts("Error: Invalid Drive\r");
			return result;
		}
		
		//Check the free space on card
		res = f_getfree("0:", &freeClusters, &fs1);
		if (res != FR_OK)
		{
			result = STATUS_FAIL;
			puts("Error: Cannot calculate free space\r");
			return result;
		}
		totalSectors = (fs1->n_fatent -2) * fs1->csize;	//only needed to calculate used space
		freeSectors = freeClusters * fs1->csize;	//assuming 512 bytes/sector
		if ((freeSectors/2) < 307200)
		{
			result = STATUS_FAIL;
			puts("Error: Low disk space on SD-card\r");
			return result;
		}
	}
	return result;
}

bool sdCardPresent()
{
	return !(ioport_get_pin_level(SD_CD_PIN));	//The gpio is pulled low when card is inserted.
}

void changeActiveBuffer(sdc_file_t *fileObject)
{
	//fileObject->activeBuffer = !fileObject->activeBuffer;
	if (fileObject->activeBuffer == fileObject->bufferPointerA)
	{
		fileObject->activeBuffer = fileObject->bufferPointerB;
	}
	else
	{
		fileObject->activeBuffer = fileObject->bufferPointerA;
	}
}

uint8_t checkEmptyLocation(open_files_t fileStruct)
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (fileStruct.openFilesArray[i] == NULL)
		{
			return i;
		}
	}
	return (MAX_OPEN_FILES + 1);	//to indicate error
}

void registerOpenFile(sdc_file_t* fileObject)
{
	uint8_t arrayIndex = 0;
	arrayIndex = checkEmptyLocation(openFilesList);	//check where the new pointer can be stored
	if (arrayIndex < MAX_OPEN_FILES)
	{
		openFilesList.openFilesArray[arrayIndex] = fileObject;
	}
	else
	{
		//TODO: add error if necessary
	}
}

void unregisterOpenFile(sdc_file_t* fileObject)
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)	//find where the pointer was stored and clear it
	{
		if (openFilesList.openFilesArray[i] == fileObject)
		{
			openFilesList.openFilesArray[i] = NULL;
			return;
		}
	}
}

void closeAllFiles()
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		sdc_closeFile(openFilesList.openFilesArray[i]);
	}
}

void cardInsertedCall()
{
	status_t status = STATUS_PASS;
	uint8_t eventData[2] = {0};
	msg_sd_card_state_t* messageData;
	
	messageData = malloc (sizeof(msg_sd_card_state_t));
	messageData->mounted = true;
	messageData->message = SD_CARD_INSERTED;
	messageData->errorCode = 0;
	msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SDCARD, MSG_TYPE_SDCARD_STATE, (void*)messageData);
	
	status = initializeSdCard();
	if (status == STATUS_PASS)
	{
		messageData = malloc (sizeof(msg_sd_card_state_t));
		messageData->mounted = true;
		messageData->message = SD_CARD_INITIALIZED;
		messageData->errorCode = 0;
		msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SDCARD, MSG_TYPE_READY, (void*)messageData);
	}
}

status_t unInitializeSdCard()
{
	closeAllFiles();
	f_mount(LUN_ID_SD_MMC_0_MEM, NULL);
	return STATUS_PASS;
}

void cardRemovedCall()
{
	status_t status = STATUS_PASS;
	uint8_t eventData[2] = {0};
	msg_sd_card_state_t* messageData;
	
	status = unInitializeSdCard();
	if (status == STATUS_PASS)
	{
		messageData = malloc(sizeof(msg_sd_card_state_t));
		messageData->message = SD_CARD_REMOVED;
		messageData->mounted = false;
		messageData->errorCode = 0;
		msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SDCARD, MSG_TYPE_ERROR, (void*)messageData);
	}
}