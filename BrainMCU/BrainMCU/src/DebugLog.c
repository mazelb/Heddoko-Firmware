/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
/*
 * DebugLog.c
 *
 * Created: 9/23/2015 2:52:02 PM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#include "DebugLog.h"
#include "common.h"
#include <string.h>
#include "drv_uart.h"
#include "task_commandProc.h"

extern drv_uart_config_t uart0Config;

UINT numberBytes;
extern uint32_t sgSysTickCount;
static int size=0;
static FIL debug_file_object;
static char DebugFileName[] = "0:DebugLog.txt";
static bool sgSDWriteLockBit = true, sgDebugLogAutoSave = false;
struct DEBUGLOG DebugLog;
/*	RTOS variables, semaphores, Mutexes, functions	*/
xSemaphoreHandle DebugLogSemaphore;

/**
 * \brief Create a file DebugLog.txt for logging Debug Data 
 */
void DebugLogCreate(void)
{
	static FRESULT res;
	DebugFileName[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&debug_file_object, (char const *)DebugFileName, FA_OPEN_ALWAYS | FA_WRITE);
	if (res == FR_OK)
	{
		debugPrintString("Debug Log open successful\r\n");
	}
	else
	{
		debugPrintString("Debug Log open unsuccessful\r\n");
	}
	
	res = f_lseek(&debug_file_object, debug_file_object.fsize);
}

/**
 * \brief Save data from buffer to DebugLog.txt
 */
void DebugLogSave(void)
{
	static FRESULT res;
	DebugFileName[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_sync(&debug_file_object);
}

/**
 * \brief This function prints the input string to a buffer which is later stored in SD Card
 */
void DebugLogBufPrint(const char* InputString)
{
	sgSDWriteLockBit = false;
	if (DebugLogSemaphore != NULL)		//Check if Semaphore is defined
	{
		if ((xSemaphoreTake(DebugLogSemaphore, (portTickType) 1000) == pdTRUE) )		//Semaphore available
		{
			//size = strlen(InputString);
			//memcpy(&(DebugLog.DebugLogBuf[DebugLog.DebugLogBufHead]), InputString, size);	//Store the input String to the defined DebugLog buffer
			//printf("%s", &(DebugLog.DebugLogBuf[DebugLog.DebugLogBufHead]));		//Debug Print function
			//DebugLog.DebugLogBufHead += size;
			//size = snprintf(&(DebugLog.DebugLogBuf[DebugLog.DebugLogBufHead]), (DATA_SIZE - DebugLog.DebugLogBufHead), "%d: %s", sgSysTickCount, InputString);
			DebugLog.DebugLogBufHead += size;
			InputString = 0;	//Clear the variable to ensure no duplication
			
			if (DebugLog.DebugLogBufHead >= DATA_SIZE)	//Reset the Head to the beginning of array
			{
				//DebugLog.DebugLogBufHead = 0;
			}
			
			if(xSemaphoreGive(DebugLogSemaphore) != pdTRUE)
			{
				//This is recursive!
				//debugPrintString("Semaphore not released\r\n");
			}
		}
		else	//Semaphore unavailable
		{
			//This is recursive!
			//debugPrintString("Semaphore Unavailable to Debug Buffer function\r\n");			
		}
	}
}

/**
 * \brief This task monitors the Debug print functionality - Storing to SD Card
 */
void TaskDebugLog(void *pvParameters)
{
	UNUSED(pvParameters);
	char vDebugBufCopy[] = {0x0A};
	char vDataCount=0, vDataCompleteCount=0;
	UINT numberBytes;
	status_t result = STATUS_PASS;
	char* bufptr = DebugLog.DebugLogBuf;
	//while (sgSDWriteLockBit == true)	//Wait until SD card initialization
	//{
		//;
	//}

	for (;;) 
	{
		if (DebugLogSemaphore != NULL)		//Check if Semaphore is defined
		{
			if ((xSemaphoreTake(DebugLogSemaphore, (portTickType) 1000) == pdTRUE) )		//Semaphore available
			{
				
				/*	Store the DebugLog data to SD Card	*/
				if(sgSDWriteLockBit == false)		// Check if SD card initialized
				{
					//vDataCount = 0;
					//if((DebugLog.DebugLogBufHead - DebugLog.DebugLogBufTail) > 0)
					//{
						////vDataCount = strcspn(&(DebugLog.DebugLogBuf[DebugLog.DebugLogBufTail]), vDebugBufCopy);		//Find the position of line termination in the buffer
						////vDataCount += 1;
					//}
					//
					////if(vDataCount > 0)
					////{
						////printf("vDataCount %d\r\n", vDataCount);		//Debug print function
					//
						//res = f_write(&debug_file_object, &(DebugLog.DebugLogBuf[DebugLog.DebugLogBufTail]), (DebugLog.DebugLogBufHead - DebugLog.DebugLogBufTail), &numberBytes);
						//res = f_sync(&debug_file_object);
						//DebugLog.DebugLogBufHead = 0;
						//DebugLog.DebugLogBufTail = 0;
						////DebugLog.DebugLogBufTail += vDataCount;
					////}
					//
					////if (DebugLog.DebugLogBufTail == DebugLog.DebugLogBufTail)
					////{
						////DebugLog.DebugLogBufTail = 0;
					////}
				}
				else
				{
					vTaskDelay(1000);
				}
				
				if(xSemaphoreGive(DebugLogSemaphore) != pdTRUE)
				{
					debugPrintString("Semaphore not released\r\n");
				}
			}
			else	//Semaphore unavailable
			{
				debugPrintString("Semaphore Unavailable to print task\r\n");
			}
		}
		vTaskDelay(1000);
	}
}