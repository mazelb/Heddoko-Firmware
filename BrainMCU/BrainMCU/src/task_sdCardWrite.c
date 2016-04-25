/*
 * task_sdCardWrite.c
 *
 * Created: 10/19/2015 8:20:39 AM
 *  Author: Sean Cloghesy
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 
#include <asf.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task_sdCardWrite.h"
#include "task_commandProc.h"
#include "task_stateMachine.h"
#include "settings.h"
#include "drv_uart.h"

extern brainSettings_t brainSettings; 
extern drv_uart_config_t uart0Config;
extern nvmSettings_t nvmSettings; 
extern uint32_t sgSysTickCount;
xSemaphoreHandle semaphore_sdCardWrite = NULL, semaphore_fatFsAccess = NULL;
volatile char sdCardBuffer[SD_CARD_BUFFER_SIZE] = {0}, debugLogBuffer[DEBUG_LOG_BUFFER_SIZE] = {0};
volatile uint32_t sdCardBufferPointer = 0, debugLogBufferPointer = 0;
volatile uint32_t totalBytesWritten = 0, debugLogTotalBytesWritten = 0; //the total bytes written to the file
volatile char tempBuf[SD_CARD_BUFFER_SIZE] = {0}, debugLogTempBuf[DEBUG_LOG_BUFFER_SIZE] = {0};
FIL dataLogFile_obj, debugLogFile_Obj;
volatile Bool dataLogFileOpen = false, debugLogFileOpen = false;  
uint8_t closeLogFileFlag = 0, closeDebugLogFileFlag = 0; 
volatile char dataLogFileName[SD_CARD_FILENAME_LENGTH] = {0};
	
char debugLogNewFileName[] = "0:DebugLog.txt", debugLogOldFileName[] = "0:DebugLog_old.txt";
	
void task_sdCardHandler(void *pvParameters)
{
	
	uint32_t numBytesToWrite = 0, numBytesWritten = 0;
	uint32_t debugNumBytesToWrite = 0, debugNumBytesWritten = 0;
	uint32_t numBytes = 0, debugNumBytes = 0;
	semaphore_sdCardWrite = xSemaphoreCreateMutex();
	semaphore_fatFsAccess = xSemaphoreCreateMutex();
	static FRESULT res = FR_OK;
	uint32_t writeStart = 0;
	uint32_t maxWriteTime = 0; 
	dataLogFileName[0] = LUN_ID_SD_MMC_0_MEM + '0';
	while(1)
	{
		//take semaphore and copy data to a temporary buffer.
		writeStart = sgSysTickCount;
		if(xSemaphoreTake(semaphore_sdCardWrite,10) == true)
		{
			if(sdCardBufferPointer > 0 && sdCardBufferPointer <= SD_CARD_BUFFER_SIZE)
			{
				memcpy(tempBuf,sdCardBuffer,sdCardBufferPointer);
				numBytesToWrite = sdCardBufferPointer;
				sdCardBufferPointer = 0;
			}
			
			if (debugLogBufferPointer > 0 && debugLogBufferPointer <= DEBUG_LOG_BUFFER_SIZE)
			{
				memcpy(debugLogTempBuf, debugLogBuffer, debugLogBufferPointer);
				debugNumBytesToWrite = debugLogBufferPointer;
				debugLogBufferPointer = 0;
			}
			xSemaphoreGive(semaphore_sdCardWrite);
		}
		//if the close file flag is set, close the file. 
		if(closeLogFileFlag == 1)
		{
			f_close(&dataLogFile_obj); 
			dataLogFileOpen = false; 
			//clear the flag. 
			closeLogFileFlag = 0;
		}
		if(closeDebugLogFileFlag == 1)
		{
			f_close(&debugLogFile_Obj);
			debugLogFileOpen = false;
			//clear the flag
			closeDebugLogFileFlag = 0;
		}
		
		if (debugLogFile_Obj.fsize >= DEBUG_LOG_MAX_FILE_SIZE)	//Check if the file is above its size limit
		{
			//Close the current file
			f_close(&debugLogFile_Obj);
			debugLogFileOpen = false;
			//Rename and switch the file
			task_debugLog_OpenFile();
		}
		
		if (xSemaphoreTake(semaphore_fatFsAccess, 1) == true)
		{
			//if the data file is open, then write to log
			if(dataLogFileOpen)
			{	
				if(numBytesToWrite > 0)
				{
					numBytesWritten = 0;			
					while(numBytesToWrite > 0)
					{			
						res = f_write(&dataLogFile_obj,  (void*)(tempBuf+numBytesWritten), numBytesToWrite, &numBytes);
						if (res != FR_OK)
						{								
							if(res == FR_DISK_ERR)
							{
								debugPrintString("Write to log file failed: Disk Error\r\n");	
							}
							else
							{
								debugPrintString("Write to log file failed\r\n");	
							}
							//this is a serious error, break the loop. 								
							break; 
						}				
						numBytesToWrite -= numBytes;
						numBytesWritten += numBytes;
						totalBytesWritten += numBytes;
						vTaskDelay(1);
					}
					res = f_sync(&dataLogFile_obj); //sync the file
					if(res != FR_OK)
					{
						debugPrintString("file sync failed\r\n");
					}
					vTaskDelay(1);
				}
			}
		
			if(debugLogFileOpen)
			{
				if(debugNumBytesToWrite > 0)
				{
					debugNumBytesWritten = 0;
					while(debugNumBytesToWrite > 0)
					{
						res = f_write(&debugLogFile_Obj,  (void*)(debugLogTempBuf+debugNumBytesWritten), debugNumBytesToWrite, &debugNumBytes);
						if (res != FR_OK)
						{								
							if(res == FR_DISK_ERR)
							{
								debugPrintString("Write to debug log file failed: Disk Error\r\n");	
							}
							else
							{
								debugPrintString("Write to debug log file failed\r\n");	
							}
							//this is a serious error, break the loop. 								
							break; 
						}					
						debugNumBytesToWrite -= debugNumBytes;
						debugNumBytesWritten += debugNumBytes;
						debugLogTotalBytesWritten += debugNumBytes;
						vTaskDelay(1);
					}
					res = f_sync(&debugLogFile_Obj); //sync the file
					if(res != FR_OK)
					{
						printf("debug sync failed with code %d\r\n", res);
					}
					vTaskDelay(1);
				}
			}
			xSemaphoreGive(semaphore_fatFsAccess);
		}
		else
		{
			debugPrintString("Waiting for semaphore to write to SD-card\r\n");
		}
		if(brainSettings.debugPrintsEnabled)
		{		
			if((sgSysTickCount - writeStart) > maxWriteTime)
			{
				maxWriteTime = sgSysTickCount - writeStart; 
				debugPrintStringInt("new max write Time\r\n",maxWriteTime);
			}
		}
		//only delay if the write time was less than 200, or else we'll get a buffer error. 
		if((sgSysTickCount - writeStart) < 200)
		{
			vTaskDelay(100);
		}
		
		
	}
}
status_t task_sdCardWriteEntry(char* entry, size_t length)
{
	status_t status = STATUS_PASS; 
	
	//take the semaphore	
	if(xSemaphoreTake(semaphore_sdCardWrite,5) == true)
	{
		//copy data to sdCard buffer, make sure we have room first
		if(sdCardBufferPointer + length < SD_CARD_BUFFER_SIZE)
		{
			if(nvmSettings.enableCsvFormat == 0)
			{
				int i = 0;
				for(i=0; i<length; i++)
				{
					(sdCardBuffer+sdCardBufferPointer)[i] = entry[i] + 0x80; //add 128 to make it not ascii
				}
			}
			else
			{
				memcpy(sdCardBuffer+sdCardBufferPointer,entry,length);
			}
			sdCardBufferPointer += length;
		}
		else
		{
			status = STATUS_FAIL;
			
		}
		xSemaphoreGive(semaphore_sdCardWrite);
		if(status == STATUS_FAIL)
		{
			debugPrintString("Write failed: Buffer full\r\n");
		}
	}
	else
	{		
		status = STATUS_FAIL; 
		debugPrintString("Write failed: semaphore locked\r\n");
	}	

	return status; 
}

status_t task_debugLogWriteEntry(char* entry, size_t length)
{
	status_t status = STATUS_PASS;
	//take the semaphore
	if(semaphore_sdCardWrite == NULL)
	{
		return STATUS_FAIL; 
	}	
	if(xSemaphoreTake(semaphore_sdCardWrite,5) == true)
	{
		//copy data to sdCard buffer, make sure we have room first
		if(debugLogBufferPointer + length < DEBUG_LOG_BUFFER_SIZE)
		{
			memcpy(debugLogBuffer+debugLogBufferPointer,entry,length);
			debugLogBufferPointer += length;
		}
		else
		{
			status = STATUS_FAIL;
		}
		xSemaphoreGive(semaphore_sdCardWrite);
	}
	else
	{
		status = STATUS_FAIL;
	}	
	return status;
	
}

status_t task_sdCard_OpenNewFile()
{
	uint8_t data_buffer[100], vFileIndexAlgoCount = 0;
	char fileIndexLog[SD_CARD_FILENAME_LENGTH] = "0:logIndex.dat";
	char dirName[SD_CARD_FILENAME_LENGTH] = "0:MovementLog"; 
	char dirPath[] = "0:MovementLog/";
	DIR dir;
	char logFileName[SD_CARD_FILENAME_LENGTH] = {0};
	char temp[153] = {0}; 
	uint32_t byte_to_read, byte_read, bytes_written;
	FRESULT res;
	uint32_t fileIndexNumber = 0, maxFileIndex = 0, fileIndexJumpCount = 0, sgSysTickCountOld = 0; 
	FIL indexFile_obj;
	FILINFO vDataLogFileInfo;
	bool exitFileSearchLoop = FALSE;
	status_t status = STATUS_PASS; 
	int avg_time = 0;
	//if the log file is open, then return an error
	if(dataLogFileOpen == true)
	{
		status = STATUS_FAIL; 
		return status; 
	}	
	if (xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
	{	
		//get the file index for the newly created file
		//open the file that contains the index numbers
		#if (FILE_CREATION_ALGO == COMMON_INDEX) 
		res = f_open(&indexFile_obj, (char const *)fileIndexLog, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
		if (res != FR_OK)
		{		
			status = STATUS_FAIL;
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
		
		#elif (FILE_CREATION_ALGO == INDIVIDUAL_INDEX)
		//Check for the latest index number
		sgSysTickCountOld = sgSysTickCount;
		if (nvmSettings.enableCsvFormat == 0)
		{
			snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "0:%s_%s%05d.dat",brainSettings.suitNumber, brainSettings.fileName, maxFileIndex);
		}
		else
		{
			snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "0:%s_%s%05d.csv",brainSettings.suitNumber, brainSettings.fileName, maxFileIndex);
		}
		
		res = f_stat((char const *)dataLogFileName, &vDataLogFileInfo);
		if (res != FR_OK)
		{
			//if the does not file exists
			exitFileSearchLoop = TRUE;
			status = STATUS_PASS;
		}
		else
		{
			//if file exists, start search algorithm
			maxFileIndex = 10000;
			fileIndexJumpCount = 10000;
		}
		
		while (!exitFileSearchLoop)
		{
			if(nvmSettings.enableCsvFormat == 0)
			{
				snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "0:%s_%s%05d.dat",brainSettings.suitNumber, brainSettings.fileName, maxFileIndex);
			}
			else
			{
				snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "0:%s_%s%05d.csv",brainSettings.suitNumber, brainSettings.fileName, maxFileIndex);
			}
		
			res = f_stat((char const *)dataLogFileName, &vDataLogFileInfo);
			if (res == FR_OK)
			{
				//if the file exists
				maxFileIndex += fileIndexJumpCount; 
				vFileIndexAlgoCount++;
				if (vFileIndexAlgoCount >9)
				{
					//assert error and break the loop
					status = STATUS_FAIL;
					exitFileSearchLoop = TRUE;
					break;
				}
				if (fileIndexJumpCount == 0)
				{
					//desired file found, exit the check
					fileIndexNumber = maxFileIndex + 1;	//assign the file index number
					status = STATUS_PASS;
					exitFileSearchLoop = TRUE;
					break;
				}
			}
			else
			{
				//file does not exist
				maxFileIndex -= fileIndexJumpCount;
				fileIndexJumpCount /= 10;
				maxFileIndex += fileIndexJumpCount; 
				vFileIndexAlgoCount = 0;
			}
		}
		avg_time = sgSysTickCount - sgSysTickCountOld;
		//latest index calculation complete
		
		#else
		snprintf(dirName, SD_CARD_FILENAME_LENGTH, "0:%s", brainSettings.fileName);
		res = f_opendir(&dir, &dirName);	//open the specified directory
		if (res == FR_NO_PATH)
		{
			res = f_mkdir(&dirName);	//the requested directory doesn't exist, create new one
			if (res != FR_OK)
			{
				status = STATUS_FAIL;
				debugPrintString("Failed on creating new movement log directory\r\n");
			}
		}
		snprintf(logFileName, SD_CARD_FILENAME_LENGTH, "%s/%s", dirName, &fileIndexLog[2]);
		strncpy(fileIndexLog, logFileName, sizeof(logFileName));
		res = f_open(&indexFile_obj, (char const *)fileIndexLog, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
		if (res != FR_OK)
		{
			status = STATUS_FAIL;
			debugPrintString("Failed on creating new movement log file\r\n");
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
		#endif
		
		if(status == STATUS_PASS)
		{
			//create the filename
			if(nvmSettings.enableCsvFormat == 0)
			{
				snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "%s/%s_%s%05d.dat",dirName, brainSettings.suitNumber, brainSettings.fileName, fileIndexNumber); 
			}
			else
			{
				snprintf(dataLogFileName, SD_CARD_FILENAME_LENGTH, "%s/%s_%s%05d.csv",dirName, brainSettings.suitNumber, brainSettings.fileName, fileIndexNumber); 
			}
			
			if (f_open(&dataLogFile_obj, (char const *)dataLogFileName, FA_OPEN_ALWAYS | FA_WRITE) == FR_OK)
			{
				debugPrintString(brainSettings.imuSet);
				strncpy(temp, dataLogFileName, sizeof(dataLogFileName));
				strncat(temp, "\r\n", 2);
				debugPrintString(temp);
				res = f_lseek(&dataLogFile_obj, dataLogFile_obj.fsize);
				dataLogFileOpen = true;
			}
			else
			{
				debugPrintString("log failed to open\r\n");
				status = STATUS_FAIL; 
			}
		}
		xSemaphoreGive(semaphore_fatFsAccess);
	}
	else
	{
		debugPrintString("Can't get semaphore to open log file\r\n");
		status = STATUS_FAIL;
	}
	return status; 
}

status_t task_debugLog_OpenFile()
{
	char debugLogFileName[20] = {0};
	uint8_t vDebugLogFileIndex = 0;
	FRESULT res;
	FILINFO vDebugLogFileInfo;
	status_t status = STATUS_PASS;
	
	//if the log file is open, then return an error
	if(debugLogFileOpen == true)
	{
		status = STATUS_FAIL;
		return status;
	}	
	if(xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
	{
		res = f_stat(debugLogNewFileName, &vDebugLogFileInfo);
		if (res == FR_OK)
		{
			//file exist, do a check on size
			if (vDebugLogFileInfo.fsize >= DEBUG_LOG_MAX_FILE_SIZE)
			{
				if(task_debugLog_ChangeFile() != STATUS_PASS)
				{
					status = STATUS_FAIL;
				}
			}
		}
		else if (res == FR_NO_FILE)
		{
			debugPrintString("File does not exist, creating new file\r\n");
		}
		else
		{			
			debugPrintString("DebugLog file open failed\r\n");
		}
		if(status == STATUS_PASS)
		{
			//Open the Log file DebugLog.txt
			res = f_open(&debugLogFile_Obj, (char const *)debugLogNewFileName, FA_OPEN_ALWAYS | FA_WRITE);
			if (res == FR_OK)
			{
				debugPrintString("\r\n\r\n\r\nProgram start Brain Pack " VERSION " \r\n");
				debugPrintString("DebugLog open\r\n");
			}
			else
			{
				debugPrintString("DebugLog failed to open\r\n");
				status = STATUS_FAIL;
			}			
		}
		if(status == STATUS_PASS)
		{
			res = f_lseek(&debugLogFile_Obj, debugLogFile_Obj.fsize);
			debugLogFileOpen = true;
			//the file is open and we're good to go, the status should be PASS here. 	
		}
		xSemaphoreGive(semaphore_fatFsAccess);	
	}
	else
	{
		debugPrintString("Can't get semaphore to open DebugLog file\r\n");
		status = STATUS_FAIL; 
	}
	return status; 
}

status_t task_sdCard_CloseFile()
{
	status_t status = STATUS_PASS; 
	//if the filelog is not open, return and error
	if(dataLogFileOpen == false)
	{
		return STATUS_FAIL;
	}		
	if(xSemaphoreTake(semaphore_fatFsAccess,200) == true)
	{
		//set the flag to have the main sd card thread close the file. 
		//we don't want to close the file in the middle of a write. 
		closeLogFileFlag = 1; 
		xSemaphoreGive(semaphore_fatFsAccess);
	}
	else
	{		
		status = STATUS_FAIL;
	}
	return status; 	
}

status_t task_debugLog_CloseFile()
{
	status_t status = STATUS_PASS; 
	//if DebugLog file is not open, return error
	if (debugLogFileOpen == false)
	{
		return STATUS_FAIL;
	}
	vTaskDelay(200);	//wait to write everything before closing the file
	if (xSemaphoreTake(semaphore_fatFsAccess, 100) == true)
	{
		//set the flag to have the main sd card thread close the file
		//we don't want to close the file in the middle of a write
		closeDebugLogFileFlag = 1;
		xSemaphoreGive(semaphore_fatFsAccess);
	}
	else
	{		
		status = STATUS_FAIL;
	}
	return status; 
	
}

status_t task_debugLog_ChangeFile()
{	
	//the file size exceeds the limit, change the log file
	//it is assumed that this file is only called when the file system semaphore is taken. 
	FRESULT res;
	FILINFO vDebugLogFileInfo;
	status_t status = STATUS_PASS;
	//check if the old file exits
	res = f_stat(debugLogOldFileName, &vDebugLogFileInfo);
	if (res == FR_OK)
	{
		//delete the old file before renaming the current file
		f_unlink(debugLogOldFileName);
	}
	//TODO can you really change the file name without closing it first?
	//rename the current file to the name of old file
	res = f_rename(&debugLogNewFileName[2], &debugLogOldFileName[2]);
	if (res != FR_OK)
	{
		debugPrintString("Rename of Debug Log file failed\r\n");
		status = STATUS_FAIL;
	}
	return status;
}

Bool task_sdCard_isDataLogOpen()
{
	return dataLogFileOpen; 
}