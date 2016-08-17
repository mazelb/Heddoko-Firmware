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
#include "drv_gpio.h"
#include "dbg_debugManager.h"

/*	Static function forward declarations	*/
bool sdCardPresent();															//check if the SD-card is present
static void initSdGpio();																//initialize the GPIO associated with SD-card
static status_t initializeSdCard();													//initializes the SD card, and mounts the drive
static status_t unInitializeSdCard();													//uninitializes the SD card, and puts the drive in an uninitialized state. 
static status_t write_to_sd_card(sdc_file_t *fileObject);								//write calls from the main thread
static void changeActiveBuffer(sdc_file_t *fileObject);								//change the active buffer safely
static void registerOpenFile(sdc_file_t* fileObject);									//register newly open file in the array
static void unregisterOpenFile(sdc_file_t* fileObject);								//unregister closed file in the array
static void closeAllFiles();															//close all open files
static void cardInsertedCall();														//re-initialize the SD-card module and pass message
static void cardRemovedCall();															//unmount the SD-card module and pass message
static void checkCardDetectInt();														//check and reconfigure the SD-CD interrupt

/*	Local Variables	*/
xSemaphoreHandle semaphore_fatFsAccess = NULL;
xQueueHandle msg_queue_sdCard = NULL;
char debugLogNewFileName[] = "sysHdk", debugLogOldFileName[] = "sysHdk_old";
char dataLogFileName[SD_CARD_FILENAME_LENGTH] = {0};
sdc_file_t *openFilesArray[MAX_OPEN_FILES] = {NULL};
volatile bool sdInsertWaitTimeoutFlag = FALSE;
xTimerHandle sdTimeOutTimer = NULL;
volatile char serialNumber[] = "S00001";
volatile sd_card_status_t sdCardStatus = SD_CARD_REMOVED; 
	
/*	extern variables	*/


void vSdTimeOutTimerCallback( xTimerHandle xTimer )
{
	sdInsertWaitTimeoutFlag = TRUE;
}

static void processEvent(msg_message_t message)
{

	switch (message.type)
	{
		case MSG_TYPE_READY:
		case MSG_TYPE_ERROR:
		case MSG_TYPE_WIFI_STATE:
		case MSG_TYPE_USB_CONNECTED:
		case MSG_TYPE_CHARGER_EVENT:
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		default:
		break;
	}
	
	
}

void sdc_sdCardTask(void *pvParameters)
{
	FRESULT res;
	UINT numBytesWritten = 0;
	uint32_t totalBytesWritten = 0, totalBytesToWrite = 0;
	status_t status = STATUS_PASS;
	msg_message_t eventMessage;
	drv_gpio_pin_state_t sdCd_oldVal, sdCd_newVal;
	
	msg_queue_sdCard = xQueueCreate(10, sizeof(msg_message_t));
	if (msg_queue_sdCard != 0)
	{
		msg_registerForMessages(MODULE_SDCARD, 0xff, msg_queue_sdCard);
	}
	
	initSdGpio();
	semaphore_fatFsAccess = xSemaphoreCreateMutex();
	//initialize the SD card... Todo add check of CD first
	cardInsertedCall();

	drv_gpio_getPinState(DRV_GPIO_PIN_SD_CD, &sdCd_oldVal);
	while (1)
	{
		drv_gpio_getPinState(DRV_GPIO_PIN_SD_CD, &sdCd_newVal);
		if (sdCd_newVal != sdCd_oldVal)
		{
			if (sdCd_newVal == DRV_GPIO_PIN_STATE_LOW)
			{
				cardRemovedCall();
			}
			else
			{
				cardInsertedCall();
			}
			sdCd_oldVal = sdCd_newVal;
		}
		
		
		if (xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
		{
			for (int i = 0; i < MAX_OPEN_FILES; i++)
			{
				//check if there are files to write to...
				if(openFilesArray[i] != NULL)
				{
					write_to_sd_card(openFilesArray[i]);	
				}				
			}
			xSemaphoreGive(semaphore_fatFsAccess);
		}
		else
		{
			//dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to get semaphore - SD-card main thread\r");
		}
		
		if (xQueueReceive(msg_queue_sdCard, &eventMessage, 1) == true)
		{
			processEvent(eventMessage);
		}
		
		//checkCardDetectInt();
		vTaskDelay(100);
	}
}

status_t write_to_sd_card(sdc_file_t *fileObject)
{
	FRESULT res;
	UINT numBytesWritten = 0;
	uint32_t totalBytesWritten = 0, totalBytesToWrite = 0;
	status_t status;
	
	if (fileObject == NULL)
	{
		return STATUS_FAIL;
	}
	
	if (fileObject->fileOpen == TRUE)
	{
		if (xSemaphoreTake(fileObject->sem_bufferAccess, 10) == true)
		{
			changeActiveBuffer(fileObject);
			xSemaphoreGive(fileObject->sem_bufferAccess);
		}
		else
		{
			dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to get semaphore - Buffer read\r");
		}
		/// wow this isn't confusing at all. 
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
						dbg_printString(DBG_LOG_LEVEL_ERROR,"Write to SD-card failed\r\n");
						//msg_sendBroadcastMessageSimple(MODULE_SDCARD,MSG_TYPE_SDCARD_STATE, SD)
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
						dbg_printString(DBG_LOG_LEVEL_ERROR,"Write to SD-card failed\r");
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
	
	if (xSemaphoreTake(fileObject->sem_bufferAccess, 1) ==  true)
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
				dbg_printString(DBG_LOG_LEVEL_ERROR,"SD-card buffer A overflow\r\n");
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
				dbg_printString(DBG_LOG_LEVEL_ERROR,"SD-card buffer B overflow\r\n");
				status = STATUS_FAIL;
			}
		}
		xSemaphoreGive(fileObject->sem_bufferAccess);
	}
	else
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"SD-card no semaphore available\r\n");
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
	
	//check if the SD card has actually been mounted. 
	if(sdCardStatus != SD_CARD_MOUNTED)
	{
		return STATUS_FAIL;
	}
	
	//return if the file is already open
	if (fileObject->fileOpen == true)
	{
		status = STATUS_PASS;	//TODO: should we pass "already open" 
		return status;
	}
	//check if the file object semaphore has been initialized. 
	if(fileObject->sem_bufferAccess == NULL)
	{
		fileObject->sem_bufferAccess = xSemaphoreCreateMutex();
	}
	

	if (xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
	{
		/*	DEBUGLOG FILES	*/
		if (mode == SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG)
		{
			//this is a debug log file
			//TODO: add the file swap feature
			snprintf(logFileName, SD_CARD_FILENAME_LENGTH, "0:%s.bin", filename);
			res = f_open(&fileObject->fileObj, (char const*)logFileName, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
			if (res == FR_OK)
			{
				dbg_printString(DBG_LOG_LEVEL_VERBOSE,"DebugLog open\r");
				dbg_printString(DBG_LOG_LEVEL_VERBOSE,logFileName);
				res = f_lseek(&fileObject->fileObj, fileObject->fileObj.fsize);
				fileObject->fileOpen = true;
				status = STATUS_PASS;
			}
			else
			{
				dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to open DebugLog\r");
				status = STATUS_FAIL;
			}
		}
		
		/*	DATALOG FILES	*/
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
					dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed on creating new movement log directory\r");
					return status;
				}
			}
			snprintf(logFileName, SD_CARD_FILENAME_LENGTH, "%s/%s", dirName, &fileIndexLog[2]);
			strncpy(fileIndexLog, logFileName, sizeof(logFileName));
			res = f_open(&indexFile_obj, (char const *)fileIndexLog, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
			if (res != FR_OK)
			{
				status = STATUS_FAIL;
				dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed on creating new movement log file\r");
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
				snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "%s/%s_%s%05d.hsm",dirName, serialNumber, filename, fileIndexNumber);				
				res = f_open(&fileObject->fileObj, (char const *) dataLogFileName, FA_OPEN_ALWAYS | FA_WRITE);
				if (res == FR_OK)
				{
					dbg_printString(DBG_LOG_LEVEL_VERBOSE,dataLogFileName);
					//res = f_lseek(&fileObject->fileObj, fileObject->fileObj.fsize);
					fileObject->fileOpen = true;
				}
				else
				{
					status = STATUS_FAIL;
				}
			}
		}
		
		/*	ALL OTHER FILES	*/
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
			snprintf(logFileName, SD_CARD_FILENAME_LENGTH, "0:%s", filename);
			res = f_open(&fileObject->fileObj, (char const*)logFileName, openMode);
			if (res == FR_OK)
			{
				res = f_lseek(&fileObject->fileObj, fileObject->fileObj.fsize);
				fileObject->fileOpen = true;
				status = STATUS_PASS;
			}
			else
			{
				dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to open file\r");
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
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to get Semaphore to open file\r");
		return STATUS_FAIL;
	}
	
	return status;
}

status_t sdc_closeFile(sdc_file_t* fileObject)
{
	status_t status = STATUS_FAIL;
	FRESULT res;
	//return if the file is already closed
	if (fileObject->fileOpen == false)
	{
		status = STATUS_FAIL; 
		return status;
	}
	
	if (xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
	{
		if(sdCardStatus == SD_CARD_MOUNTED)
		{	
			res = f_sync(&fileObject->fileObj);
			res = f_close(&fileObject->fileObj);
			if (res == FR_OK)
			{			
				status = STATUS_PASS;
			}
		}
		
		xSemaphoreGive(semaphore_fatFsAccess);
	}
	else
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to get semaphore - SD-card close file\r");
		status = STATUS_FAIL;
	}
	//always do this... so we don't get stuck into a loop waiting for the SD card
	fileObject->fileOpen = false; 		
	unregisterOpenFile(fileObject);
	return status;
}

void initSdGpio()
{
	//gpio_configure_pin(SD_MMC_0_CD_GPIO, SD_MMC_0_CD_FLAGS);
	///* Configure HSMCI pins */
	//gpio_configure_pin(PIN_HSMCI_MCCDA_GPIO, PIN_HSMCI_MCCDA_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCCK_GPIO, PIN_HSMCI_MCCK_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA0_GPIO, PIN_HSMCI_MCDA0_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA1_GPIO, PIN_HSMCI_MCDA1_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA2_GPIO, PIN_HSMCI_MCDA2_FLAGS);
	//gpio_configure_pin(PIN_HSMCI_MCDA3_GPIO, PIN_HSMCI_MCDA3_FLAGS);
}

status_t  __attribute__((optimize("O0"))) initializeSdCard()
{
	static FATFS fs;
	static FATFS* fs1;	//pointer to FATFS structure used to check free space
	static FRESULT res;
	static DWORD freeClusters, freeSectors, totalSectors;
	status_t result = STATUS_PASS;
	Ctrl_status status = STATUS_FAIL;
	char test_file_name[] = "0:sd_mmc_test.txt";
	FIL file_object;
	
	//if (systemStatus.sdCardState == SD_CARD_INITIALIZED)
	//{
		//return STATUS_FAIL;
	//}
	
	if (!sdCardPresent())
	{
		sdInsertWaitTimeoutFlag = FALSE;
		return STATUS_FAIL;
	}
	drv_gpio_config_interrupt(DRV_GPIO_PIN_SD_CD, DRV_GPIO_INTERRUPT_LOW_EDGE);		//the card is present, reconfigure the interrupt.
	
	sd_mmc_init();
	sdTimeOutTimer = xTimerCreate("SD insert tmr", (SD_INSERT_WAIT_TIMEOUT/portTICK_RATE_MS), pdFALSE, NULL, vSdTimeOutTimerCallback);
	if (sdTimeOutTimer == NULL)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create SD card timer\r\n");
	}
	xTimerStart(sdTimeOutTimer, 0);
	
	do
	{
		status = sd_mmc_test_unit_ready(0);
		if (CTRL_FAIL == status)
		{			
			//TODO does this even need to be here... should we just go straight to SD card error?
			dbg_printString(DBG_LOG_LEVEL_ERROR,"SD Card needs to be re-installed\n\r");
			while ((CTRL_NO_PRESENT != sd_mmc_check(0)) && (sdInsertWaitTimeoutFlag == FALSE))
			{
				vTaskDelay(1);
			}
		}
	} while ((CTRL_GOOD != status) && (sdInsertWaitTimeoutFlag == FALSE));
	
	sdInsertWaitTimeoutFlag = FALSE;	//clear the flag for reuse
	xTimerStop(sdTimeOutTimer, 0);
	xTimerDelete(sdTimeOutTimer, 0);
	
	/*	Mount the SD card	*/
	if(status == CTRL_GOOD)
	{
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		if (res == FR_INVALID_DRIVE)
		{
			result = STATUS_FAIL;
			dbg_printString(DBG_LOG_LEVEL_ERROR,"Invalid Drive\r");
			return result;
		}
		
		//Check the free space on card
		//res = f_getfree("0:", &freeClusters, &fs1);
		//if (res != FR_OK)
		//{
			//result = STATUS_FAIL;
			//dbg_printString(DBG_LOG_LEVEL_ERROR,"Cannot calculate free space\r");
			//return result;
		//}
		//totalSectors = (fs1->n_fatent -2) * fs1->csize;	//only needed to calculate used space
		//freeSectors = freeClusters * fs1->csize;	//assuming 512 bytes/sector
		//if ((freeSectors/2) < 307200) //TODO wow, this is a great random number
		//{
			//result = STATUS_FAIL;
			//dbg_printString(DBG_LOG_LEVEL_ERROR,"Low disk space on SD-card\r");
			//return result;
		//}
	}
	else 
	{
		result = STATUS_FAIL;
	}

	return result;
}

bool sdCardPresent()
{
	//NOTE: Change as per the board in use
	return (ioport_get_pin_level(SD_CD_PIN));	//The GPIO is pulled high when card is inserted.
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

uint8_t checkEmptyLocation()
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (openFilesArray[i] == NULL)
		{
			return i;
		}
	}
	return (MAX_OPEN_FILES + 1);	//to indicate error
}

void registerOpenFile(sdc_file_t* fileObject)
{
	uint8_t arrayIndex = 0;
	arrayIndex = checkEmptyLocation();	//check where the new pointer can be stored
	if (arrayIndex < MAX_OPEN_FILES)
	{
		openFilesArray[arrayIndex] = fileObject;
	}
	else
	{
		//TODO: add error if necessary, no space to open new file
	}
}

void unregisterOpenFile(sdc_file_t* fileObject)
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)	//find where the pointer was stored and clear it
	{
		if (openFilesArray[i] == fileObject)
		{
			openFilesArray[i] = NULL;
			return;
		}
	}
}

void closeAllFiles()
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if(openFilesArray[i] != NULL)
		{		
			sdc_closeFile(openFilesArray[i]);
		}
	}
}

void cardInsertedCall()
{
	status_t status = STATUS_PASS;
	uint8_t eventData[2] = {0};
	//msg_sd_card_state_t* messageData;
	//
	//messageData = malloc (sizeof(msg_sd_card_state_t));
	//messageData->mounted = true;
	//messageData->message = SD_CARD_INSERTED;
	//messageData->errorCode = 0;
	//msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SDCARD, MSG_TYPE_SDCARD_STATE, (void*)messageData);
	//
	status = initializeSdCard();
	if(status == STATUS_PASS)
	{
		dbg_printString(DBG_LOG_LEVEL_VERBOSE,"SD-card Mounted\r\n");
		sdCardStatus = SD_CARD_MOUNTED;
	}
	else
	{
		dbg_printString(DBG_LOG_LEVEL_VERBOSE,"Failed to mount SD card\r\n");
		sdCardStatus = SD_CARD_MOUNT_ERROR;
	}
	msg_sendBroadcastMessageSimple(MODULE_SDCARD, MSG_TYPE_SDCARD_STATE, sdCardStatus);
	
	
}

status_t unInitializeSdCard()
{
	closeAllFiles();
	f_mount(LUN_ID_SD_MMC_0_MEM, NULL);
	sdCardStatus = SD_CARD_REMOVED;
	msg_sendBroadcastMessageSimple(MODULE_SDCARD, MSG_TYPE_SDCARD_STATE, sdCardStatus);
	return STATUS_PASS;
}

void cardRemovedCall()
{
	status_t status = STATUS_PASS;
	uint8_t eventData[2] = {0};
	//msg_sd_card_state_t* messageData;	
	dbg_printString(DBG_LOG_LEVEL_VERBOSE,"SD-card Removed\r\n");
	status = unInitializeSdCard();
}

void checkCardDetectInt()
{	
	if (drv_gpio_check_Int(DRV_GPIO_PIN_SD_CD) == 1)
	{
		drv_gpio_pin_state_t sdCdPinState;
		drv_gpio_getPinState(DRV_GPIO_PIN_SD_CD, &sdCdPinState);
		if (sdCdPinState == DRV_GPIO_PIN_STATE_HIGH)
		{
			
			//SD card not present, set the respective event
			cardRemovedCall();
			//reconfigure the SD-card interrupt to look for insertion of card
			drv_gpio_config_interrupt(DRV_GPIO_PIN_SD_CD, DRV_GPIO_INTERRUPT_LOW_EDGE);
		}
		else if (sdCdPinState == DRV_GPIO_PIN_STATE_LOW)
		{
			dbg_printString(DBG_LOG_LEVEL_VERBOSE,"SD-card Inserted\r\n");
			//SD card present or inserted, set the respective event
			cardInsertedCall();
			//drv_gpio_config_interrupt(DRV_GPIO_PIN_SD_CD, DRV_GPIO_INTERRUPT_LOW_EDGE);	//set in initializeSdCard()
		}
	}
}

