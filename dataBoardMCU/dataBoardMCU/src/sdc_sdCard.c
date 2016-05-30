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

//Static function forward declarations
bool sdCardPresent();
//initializes the SD card, and mounts the drive
status_t initializeSdCard();
//uninitializes the SD card, and puts the drive in an uninitialized state. 
status_t unInitializeSdCard();

static bool sd_mmc_ejected[2] = {false, false};
volatile char dataLogBufferA[DATALOG_MAX_BUFFER_SIZE] = {0} , dataLogBufferB[DATALOG_MAX_BUFFER_SIZE] = {0};
volatile char debugLogBufferA[DEBUGLOG_MAX_BUFFER_SIZE] = {0} , debugLogBufferB[DEBUGLOG_MAX_BUFFER_SIZE] = {0};
FIL dataLogFile_obj, debugLogFile_Obj;
xSemaphoreHandle semaphore_fatFsAccess = NULL;
char debugLogNewFileName[] = "sysHdk", debugLogOldFileName[] = "sysHdk_old";
char dataLogFileName[SD_CARD_FILENAME_LENGTH] = {0};

sdc_file_t dataLogFile = 
{
	.bufferIndexA = 0, 
	.bufferIndexB = 0,
	.bufferPointerA = dataLogBufferA,
	.bufferPointerB = dataLogBufferB,
	.bufferSize = DATALOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileOpen = false,
	.activeBuffer = 0
};

sdc_file_t debugLogFile = 
{
	.bufferIndexA = 0,
	.bufferIndexB = 0,
	.bufferPointerA = debugLogBufferA,
	.bufferPointerB = debugLogBufferB,
	.bufferSize = DEBUGLOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileOpen = false,
	.activeBuffer = 0
};

void sdc_sdCardTask(void *pvParameters)
{
	FRESULT res;
	UINT numBytesWritten = 0;
	uint32_t totalBytesWritten = 0, totalBytesToWrite = 0;
	status_t status;
	
	// initialize the file structures
	initializeSdCard();
	semaphore_fatFsAccess = xSemaphoreCreateMutex();
	
	while (1)
	{
		if (xSemaphoreTake(semaphore_fatFsAccess, 1) == true)
		{
			if (dataLogFile.fileOpen)
			{
				//Data Log
				dataLogFile.activeBuffer = ! dataLogFile.activeBuffer;
				if (dataLogFile.activeBuffer == 0)
				{
					//use buffer B as we just toggled the buffer
					if (dataLogFile.bufferIndexB > 0)
					{
						totalBytesToWrite = dataLogFile.bufferIndexB;
						numBytesWritten = 0;
						totalBytesWritten = 0;
						while (totalBytesToWrite > 0)
						{
							res = f_write(&dataLogFile.fileObj, (void*)dataLogFile.bufferPointerB + totalBytesWritten, 
																totalBytesToWrite,	&numBytesWritten);
							if (res != FR_OK)
							{
								puts("Write to Datalog failed\r");
								status = STATUS_FAIL;
								break;
							}
							res = f_sync(&dataLogFile.fileObj);
							totalBytesToWrite -= numBytesWritten;
							totalBytesWritten += numBytesWritten;
							vTaskDelay(1);
						}
						if (totalBytesToWrite == 0)
						{
							dataLogFile.bufferIndexB = 0;
						}
					}
				}
				else
				{
					//use buffer A as we just toggled the buffer
					if (dataLogFile.bufferIndexA > 0)
					{
						totalBytesToWrite = dataLogFile.bufferIndexA;
						numBytesWritten = 0;
						totalBytesWritten = 0;
						while (totalBytesToWrite > 0)
						{
							res = f_write(&dataLogFile.fileObj, (void*)dataLogFile.bufferPointerA + totalBytesWritten, 
																totalBytesToWrite, &numBytesWritten);
							if (res != FR_OK)
							{
								puts("Write to Datalog from buffer A failed\r");
								status = STATUS_FAIL;
								break;
							}
							res = f_sync(&dataLogFile.fileObj);
							totalBytesToWrite -= numBytesWritten;
							totalBytesWritten += numBytesWritten;
							vTaskDelay(1);
						}
						if (totalBytesToWrite == 0)
						{
							dataLogFile.bufferIndexA = 0;
						}
					}
				}
			}
			
			/*	DebugLog section	*/
			if (debugLogFile.fileOpen)
			{
				//Debug Log
				debugLogFile.activeBuffer = ! debugLogFile.activeBuffer;
				if (debugLogFile.activeBuffer == 0)
				{
					//use buffer B as we just toggled the buffer
					if (debugLogFile.bufferIndexB > 0)
					{
						totalBytesToWrite = debugLogFile.bufferIndexB;
						totalBytesWritten = 0;
						numBytesWritten = 0;
						while (totalBytesToWrite > 0)
						{
							res = f_write(&debugLogFile.fileObj, (void*)debugLogFile.bufferPointerB + totalBytesWritten,
																	totalBytesToWrite,	&numBytesWritten);
							if (res != FR_OK)
							{
								puts("Write to Debuglog failed\r");
								status = STATUS_FAIL;
							}
							res = f_sync(&debugLogFile.fileObj);
							totalBytesToWrite -= numBytesWritten;
							totalBytesWritten += numBytesWritten;
							vTaskDelay(1);
						}
						if (totalBytesToWrite == 0)
						{
							debugLogFile.bufferIndexB = 0;
						}
					}
				}
				else
				{
					//use buffer A as we just toggled the buffer
					if (debugLogFile.bufferIndexA > 0)
					{
						totalBytesToWrite = debugLogFile.bufferIndexA;
						totalBytesWritten = 0;
						numBytesWritten = 0;
						while (totalBytesToWrite > 0)
						{
							res = f_write(&debugLogFile.fileObj, (void*)debugLogFile.bufferPointerA + totalBytesWritten, 
																	totalBytesToWrite, &numBytesWritten);
							if (res != FR_OK)
							{
								puts("Write to Debuglog from buffer A failed\r");
								status = STATUS_FAIL;
							}
							res = f_sync(&debugLogFile.fileObj);
							totalBytesToWrite -= numBytesWritten;
							totalBytesWritten += numBytesWritten;
							vTaskDelay(1);
						}
						if (totalBytesToWrite == 0)
						{
							debugLogFile.bufferIndexA = 0;
						}
					}
				}
			}
			
			xSemaphoreGive(semaphore_fatFsAccess);
		}
		else
		{
			puts("Failed to get semaphore\r");
		}
		vTaskDelay(100);
	}
}

status_t sdc_writeToFile(sdc_file_t* fileObject, char* data, size_t size)
{
	status_t status = STATUS_PASS;
	
	//if (xSemaphoreTake(semaphore_fatFsAccess) ==  true)
	//{
		if (fileObject->activeBuffer == 0)
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
		//xSemaphoreGive(semaphore_fatFsAccess);
	//}
	//else
	//{
		//status = STATUS_FAIL;
	//}
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
	uint8_t data_buffer[100] = {0};
	uint16_t fileIndexNumber = 0, byte_read = 0, bytes_written = 0;
	DIR dir;
	FIL indexFile_obj;
	
	//return if the file is already open
	if (fileObject->fileOpen == true)
	{
		status = STATUS_FAIL; 
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
			res = f_open(&fileObject->fileObj, (char const*)logFileName, FA_OPEN_ALWAYS | FA_WRITE);
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
		else
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
				
				if (f_open(&fileObject->fileObj, (char const *)dataLogFileName, FA_OPEN_ALWAYS | FA_WRITE) == FR_OK)
				{
					puts(dataLogFileName);
					res = f_lseek(&fileObject->fileObj, fileObject->fileObj.fsize);
					fileObject->fileOpen = true;
				}
				else
				{
					puts("log failed to open\r\n");
					status = STATUS_FAIL;
				}
			}
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

//Ctrl_status sd_mmc_test_unit_ready(uint8_t slot)	//TODO: this function has to be defined here as it is no longer a part of library.
//{
	//switch (sd_mmc_check(slot))
	//{
		//case SD_MMC_OK:
		//if (sd_mmc_ejected[slot])
		//{
			//return CTRL_NO_PRESENT;
		//}
		//if (sd_mmc_get_type(slot) & (CARD_TYPE_SD | CARD_TYPE_MMC))
		//{
			//return CTRL_GOOD;
		//}
		//// It is not a memory card
		//return CTRL_NO_PRESENT;
//
		//case SD_MMC_INIT_ONGOING:
		//return CTRL_BUSY;
//
		//case SD_MMC_ERR_NO_CARD:
		//sd_mmc_ejected[slot] = false;
		//return CTRL_NO_PRESENT;
//
		//default:
		//return CTRL_FAIL;
	//}
//}

status_t initializeSdCard()
{
	static FATFS fs;
	static FATFS* fs1;	//pointer to FATFS structure used to check free space
	static FRESULT res;
	static DWORD freeClusters, freeSectors, totalSectors;
	status_t result = STATUS_FAIL;
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
}

bool sdCardPresent()
{
	return !(ioport_get_pin_level(SD_CD_PIN));	//The gpio is pulled low when card is inserted.
}