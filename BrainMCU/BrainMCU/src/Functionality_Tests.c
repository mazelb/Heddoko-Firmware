/*
 * Functionality_Tests.c
 *
 * Created: 9/16/2015 3:21:27 PM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 

#include "Functionality_Tests.h"
#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "conf_board.h"
#include "DebugLog.h"

static char test_file_name[] = "0:sd_mmc_test.txt";

/**
 * SDWriteTest(void)
 * @brief Write test text to SD Card
 */
int SDWriteTest (void)
{
	uint8_t buf[] = "Test Content\r\n";
	UINT numberBytes;
	uint8_t result = SUCCESS;
	static FRESULT res;
	static FIL file_object;
	//WTF is this? 
	if (FR_INVALID_DRIVE != res)
	{
		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)test_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		
		if (res == FR_OK)
		{
			res = f_write(&file_object, buf, sizeof(buf), &numberBytes );
			//printf("Wrote %d bytes\r\n", numberBytes);		//Debug Test to print number of bytes written
			if (res == FR_OK)
			{
				//printf("Write Successful\r\n");
				DebugLogBufPrint("Write Successful\r\n");
			}
			else
			{
				result = WRITE_FAILED;
				//printf("Error: Write failed\r\n");
				DebugLogBufPrint("Error: Write failed\r\n");
				return result;
			}
			
			res = f_close(&file_object);
			
			if (res == FR_OK)
			{
				//printf("Close Successful\r\n");
				DebugLogBufPrint("Close Successful\r\n");
			}
			else
			{
				result = CANNOT_CLOSE;
				//printf("Error: Close Failed\r\n");
				DebugLogBufPrint("Error: Close Failed\r\n");
				return result;
			}
		}
		
		else
		{
			result = CANNOT_OPEN;
			//printf("Error: Cannot Open file\r\n");
			DebugLogBufPrint("Error: Cannot Open file\r\n");
			return result;
		}
	}
	
	else
	{
		result = INVALID_DRIVE;
		//printf("Error: Invalid drive\r\n");
		DebugLogBufPrint("Error: Invalid drive\r\n");
		return result;
	}
	return result;
}

/**
 * SDReadTest(void)
 * @brief Read test text from SD Card
 */
int SDReadTest(void)
{
	uint8_t buf[] = "Test Content\r\n";
	static uint8_t data_buffer[DATA_SIZE];
	UINT byte_to_read, byte_read;
	uint8_t result = SUCCESS;
	static FRESULT res;
	static FIL file_object;
	//printf("Opening SD Card to read\r\n");
	DebugLogBufPrint("Opening SD Card to read\r\n");
	test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&file_object, (char const *)test_file_name, FA_READ);
	if (res != FR_OK)
	{
		result = CANNOT_OPEN;
		//printf("Error: Cannot Open file\r\n");
		DebugLogBufPrint("Error: Cannot Open file\r\n");
		return result;
	}
	
	//printf("Reading from SD\r\n");
	DebugLogBufPrint("Reading from SD\r\n");
	memset(data_buffer, 0, DATA_SIZE);
	byte_to_read = file_object.fsize;

	for (int i = 0; i < byte_to_read; i += DATA_SIZE)
	{
		res = f_read(&file_object, data_buffer, DATA_SIZE, &byte_read);
		if (res != FR_OK)
		{
			result = READ_FAILED;
			//printf("Error: Cannot Open file\r\n");
			DebugLogBufPrint("Error: Cannot Open file\r\n");
			return result;
		}
	}
	
	//printf("Closing the file\r\n");
	DebugLogBufPrint("Closing the file\r\n");
	res = f_close(&file_object);
	if (res != FR_OK)
	{
		result = CANNOT_CLOSE;
		//printf("Error: Cannot Close file\r\n");
		DebugLogBufPrint("Error: Cannot Close file\r\n");
		return result;
	}
	
	printf("%s", data_buffer);
	
	if (memcmp(data_buffer, buf, 14) == 0)
	{
		result = SUCCESS;
		//printf("Compare Success\r\n");
		DebugLogBufPrint("Compare Success\r\n");
		return result;
	}
	else
	{
		result = UNKNOWN_ERROR;
		//printf("Error: Compare Failed\r\n");
		DebugLogBufPrint("Error: Compare Failed\r\n");
		return result;
	}
}