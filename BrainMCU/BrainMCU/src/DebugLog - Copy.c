/*
 * DebugLog.c
 *
 * Created: 9/17/2015 11:11:29 AM
 *  Author: Hriday Mehta
 */ 

#include "DebugLog.h"
#include <string.h>

/**
 * \brief 
 */
void DebugLogCreate(void)
{
	DebugFileName[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&debug_file_object, (char const *)DebugFileName, FA_OPEN_ALWAYS | FA_WRITE);
	if (res == FR_OK)
	{
		printf("Debug Log open successful\r\n");
	}
	else
	{
		printf("Debug Log open unsuccessful\r\n");
	}
	
	res = f_lseek(&debug_file_object, debug_file_object.fsize);
}

/**
 * \brief This function prints the input string to a buffer which is later stored in SD Card
 */
void DebugLogBufPrint(char* InputString)
{
	sgSDWriteLockBit = false;
	if (DebugLogSemaphore != NULL)		//Check if Semaphore is defined
	{
		if ((xSemaphoreTake(DebugLogSemaphore, (portTickType) 1000) == pdTRUE) )		//Semaphore available
		{
			size = strlen(InputString);
			memcpy(&(DebugLog.DebugLogBuf[DebugLog.DebugLogBufHead]), InputString, size);	//Store the input String to the defined DebugLog buffer
			
			DebugLog.DebugLogBufHead = DebugLog.DebugLogBufHead + size;
			InputString = 0;	//Clear the variable to ensure no duplication
			
			if (DebugLog.DebugLogBufHead >= DATA_SIZE)	//Reset the Head to the beginning of array
			{
				DebugLog.DebugLogBufHead = 0;
			}
			
			if(xSemaphoreGive(DebugLogSemaphore) != pdTRUE)
			{
				printf("Semaphore not released\r\n");
			}
		}
		else	//Semaphore unavailable
		{
			printf("Semaphore Unavailable to Debug Buffer function\r\n");			
		}
	}
}

/**
 * \brief This task monitors the Debug print functionality - Storing to SD Card
 */
void TaskDebugLog(void *pvParameters)
{
	UNUSED(pvParameters);
	char vDebugBufCopy[20] = {0};
	char vDataCount=0, vDataCompleteCount=0;
	UINT numberBytes;
	uint8_t result = SUCCESS;
	
	while (sgSDWriteLockBit == true)
	{
		;
	}
	
	for (;;) 
	{
		if (DebugLogSemaphore != NULL)		//Check if Semaphore is defined
		{
			if ((xSemaphoreTake(DebugLogSemaphore, (portTickType) 1000) == pdTRUE) )		//Semaphore available
			{
				/*	Store the DebugLog data to SD Card	*/
				//if((sgSDWriteLockBit == false))
				//{
					//DebugFileName[0] = LUN_ID_SD_MMC_0_MEM + '0';
					//res = f_open(&file_object, (char const *)DebugFileName, FA_OPEN_EXISTING | FA_WRITE);
				//}
				//else
				//{
					//vTaskDelay(100);
				//}
				//
				//vDataCount = 0;
				//do
				//{
					//if (DebugLog.DebugLogBufTail >= DATA_SIZE)
					//{
						//DebugLog.DebugLogBufTail = 0;
					//}
					//
					//if (DebugLog.DebugLogBuf[DebugLog.DebugLogBufTail] != 0x00)
					//{
						//vDebugBufCopy[vDataCount] = DebugLog.DebugLogBuf[DebugLog.DebugLogBufTail];
						////putchar(vDebugBufCopy[vDataCount]);
						//
						//DebugLog.DebugLogBuf[DebugLog.DebugLogBufTail] = 0;
						//DebugLog.DebugLogBufTail++;
						//vDataCompleteCount++;
					//}
				//}while (vDataCount++ != 20);		//Does Storing of data to an SD card file on 20Bytes basis.
				
				vDataCount = 0;
				memcpy(&(vDebugBufCopy[vDataCount]), &(DebugLog.DebugLogBuf[DebugLog.DebugLogBufTail]), 20);
				//printf("%s", &(vDebugBufCopy[0]));
				DebugLog.DebugLogBufTail += 20;
			
				if(sgSDWriteLockBit == false)
				{
					res = f_open(&debug_file_object, (char const *)DebugFileName, FA_OPEN_ALWAYS | FA_WRITE);
					res = f_lseek(&debug_file_object, debug_file_object.fsize);
					res = f_write(&debug_file_object, vDebugBufCopy, (UINT) 20, &numberBytes);
					res = f_close(&debug_file_object);
				}
				else
				{
					vTaskDelay(10);
				}
				
				
				//res = f_sync(&file_object);
				
				//if((sgSDWriteLockBit == false) && (vDataCompleteCount >= 20))
				//{
					//DebugFileName[0] = LUN_ID_SD_MMC_0_MEM + '0';
					//res = f_open(&file_object, (char const *)DebugFileName, FA_OPEN_EXISTING | FA_WRITE);
				//
					//if (res == FR_OK)
					//{
						//res = f_write(&file_object, vDebugBufCopy, sizeof(vDebugBufCopy), &numberBytes );
						////printf("Wrote %d bytes\r\n", numberBytes);		//Debug Test to print number of bytes written
						//if (res == FR_OK)
						//{
							//printf("Debug Write Successful\r\n");
							////DebugLogBufPrint("Write Successful\r\n");
						//}
						//else
						//{
							//result = WRITE_FAILED;
							//printf("Error: Debug Write failed\r\n");
							////DebugLogBufPrint("Error: Write failed\r\n");
							//return result;
						//}
					//
						//res = f_close(&file_object);
					//
						//if (res == FR_OK)
						//{
							//printf("Debug Close Successful\r\n");
							////DebugLogBufPrint("Close Successful\r\n");
						//}
						//else
						//{
							//result = CANNOT_CLOSE;
							//printf("Error: Debug Close Failed\r\n");
							////DebugLogBufPrint("Error: Close Failed\r\n");
							//return result;
						//}
					//}
				//
					//else
					//{
						//result = CANNOT_OPEN;
						//printf("Error: Cannot Open Debug file\r\n");
						////DebugLogBufPrint("Error: Cannot Open file\r\n");
						//return result;
					//}
					//vDataCompleteCount=0;					
				//}
				//else
				//{
					//vTaskDelay(100);
				//}
				
				
				if(xSemaphoreGive(DebugLogSemaphore) != pdTRUE)
				{
					printf("Semaphore not released\r\n");
				}
			}
			else	//Semaphore unavailable
			{
				printf("Semaphore Unavailable to print task\r\n");
			}
		}
		vTaskDelay(10);
	}
}

