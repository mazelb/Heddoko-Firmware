/*
 * common.c
 *
 * Created: 9/21/2015 8:34:08 AM
 *  Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 
#include <asf.h>
#include <string.h>
#include "common.h"
#include "imu.h"
#include "nvm.h"

sensorSettings_t settings = 
{
	.settingsKey = 0,
	.sensorId = SENSOR_ID_DEFAULT,
	.serialNumber = {0,0,0,0,0,0,0,0,0,0,0,0},
	.setupModeEnabled = false,
	#ifdef HPR
	.enableHPR = 1,
	#else
	.enableHPR = 0,
	#endif
	.baud = 921600,
	.magRate = EM_MAG_OUPUT_DATA_RATE,
	.accelRate = EM_ACCEL_OUPUT_DATA_RATE,
	.gyroRate = EM_GYRO_OUPUT_DATA_RATE,
	.warmUpValid = 0, 
	.loadRangesOnBoot = 0,
	.loadWarmupOnBoot = 0,
	.algoControlReg = 0x00 //all options disabled on algorithm by default. 	
};
volatile uint32_t warmUpParameterValues[35] = {0}; 	
int itoa(int value, char* sp, int radix)
{
	 char tmp[16];		// be careful with the length of the buffer
	 char *tp = tmp;
	 int i;
	 unsigned v;

	 int sign = (radix == 10 && value < 0);
	 if (sign)
		 v = -value;
	 else
		v = (unsigned)value;

	 while (v || tp == tmp)
	 {
		 i = v % radix;
		 v /= radix;	// v/=radix uses less CPU clocks than v=v/radix does
		 if (i < 10)
			*tp++ = i+'0';
		 else
			*tp++ = i + 'a' - 10;
	 }

	 int len = tp - tmp;

	 if (sign)
	 {
		 *sp++ = '-';
		 len++;
	 }

	 while (tp > tmp)
	 *sp++ = *--tp;

	 return len;
}
void readUniqueId()
{
	uint8_t* localSerialNumber = 0x0080A00C; //this is the memory address where the serial number lives.
	memcpy(settings.serialNumber,localSerialNumber,16);
}
//writes all the settings to memory. 
void writeSettings()
{
	
	enum status_code error_code = STATUS_OK;
	//read the unique ID for the microcontroller
	readUniqueId();
	settings.settingsKey = SETTINGS_MASTER_KEY;
	uint8_t eepromArray[4][64] = {0}; 
	int i = 0;	
	//copy the settings to the array
	memcpy(eepromArray[0],&settings,sizeof(sensorSettings_t));
	//copy the key to the front of all memory addresses
	memcpy(eepromArray[1],&(settings.settingsKey),4);
	memcpy(eepromArray[2],&(settings.settingsKey),4);
	memcpy(eepromArray[3],&(settings.settingsKey),4);
	//copy the warm up values to the 3 remaining pages
	memcpy(&(eepromArray[1][4]),warmUpParameterValues,60); 
	memcpy(&(eepromArray[2][4]),&(warmUpParameterValues[15]),60); 
	memcpy(&(eepromArray[3][4]),&(warmUpParameterValues[30]),20); 
	//first erase the row
	do {
		error_code = nvm_erase_row(
		(uint32_t)(SETTINGS_NVM_PAGE));
	} while (error_code == STATUS_BUSY);
	
	for(i=0;i<4;i++)
	{
		do {
			error_code = nvm_write_buffer(
			(uint32_t)(SETTINGS_NVM_PAGE+(i*64)),
			eepromArray[i],NVMCTRL_PAGE_SIZE);
		} while (error_code == STATUS_BUSY);
		
			//do
			//{
				//error_code = nvm_execute_command(
				//NVM_COMMAND_WRITE_PAGE,
				//(uint32_t)(SETTINGS_NVM_PAGE), 0);
			//} while (error_code == STATUS_BUSY);
	}
	
	
	//do {
		//error_code = nvm_write_buffer(
		//(uint32_t)(SETTINGS_NVM_PAGE),
		//&settings,
		//sizeof(sensorSettings_t));
	//} while (error_code == STATUS_BUSY);
	
	//write the data to the buffer 
	//do
	//{
		//error_code = nvm_update_buffer(SETTINGS_NVM_PAGE, &settings,
		//0, sizeof(sensorSettings_t));
	//} while (error_code == STATUS_BUSY);	
	
	//commit the buffer to a page. 
	//do 
	//{
		//error_code = nvm_execute_command(
		//NVM_COMMAND_WRITE_PAGE,
		//(uint32_t)(SETTINGS_NVM_PAGE), 0);
	//} while (error_code == STATUS_BUSY);
	//
}
//TODO: This is pretty ugly, when there's time come back and refactor!
__attribute__((optimize("O0"))) status_t loadSettings()
{
	status_t result = STATUS_PASS; 
	sensorSettings_t tempSettings;
	uint32_t memBuf[16]; 
	uint32_t warmStartUpBuf[35] = {0};
	struct nvm_config config;
	struct nvm_parameters parameters;
	//struct nvm_parameters parameters;
	enum status_code error_code = STATUS_OK;
	nvm_get_config_defaults(&config);
	config.manual_page_write = false;

	/* Apply new NVM configuration */
	do {
		error_code = nvm_set_config(&config);
	} while (error_code == STATUS_BUSY);	
	
	do {
		error_code = nvm_read_buffer(
		(uint32_t)(SETTINGS_NVM_PAGE),
		&tempSettings,
		sizeof(sensorSettings_t));
	} while (error_code == STATUS_BUSY);
	
	if(error_code == STATUS_OK)
	{
		if(tempSettings.settingsKey == SETTINGS_MASTER_KEY)
		{
			memcpy(&settings,&tempSettings,sizeof(sensorSettings_t)); 
			 
		}
		else
		{
			//report a failure 
			result = STATUS_FAIL; 
		}
	}
	else
	{
		result = STATUS_FAIL; 
	}
	//read the first buffer of the warm startup
	do {
		error_code = nvm_read_buffer(
		(uint32_t)(SETTINGS_NVM_PAGE+(64*1)),
		memBuf,NVMCTRL_PAGE_SIZE);
	} while (error_code == STATUS_BUSY);
	if(error_code == STATUS_OK)
	{
		//check that the key is alright. 
		if(memBuf[0] == SETTINGS_MASTER_KEY)
		{
			//the key is fine, copy the data over. 
			memcpy(warmUpParameterValues, &(memBuf[1]),NVMCTRL_PAGE_SIZE-4);
		}
		else
		{
			result = STATUS_FAIL;
		}
	}
	else
	{
		result = STATUS_FAIL; 
	}

	do {
		error_code = nvm_read_buffer(
		(uint32_t)(SETTINGS_NVM_PAGE+(64*2)),
		memBuf,NVMCTRL_PAGE_SIZE);
	} while (error_code == STATUS_BUSY);
	
	if(error_code == STATUS_OK)
	{
		//check that the key is alright.
		if(memBuf[0] == SETTINGS_MASTER_KEY)
		{
			//the key is fine, copy the data over.
			memcpy(&(warmUpParameterValues[15]), &(memBuf[1]),NVMCTRL_PAGE_SIZE-4);
		}
		else
		{
			result = STATUS_FAIL;
		}
	}
	else
	{
		result = STATUS_FAIL;
	}
		
	do {
		error_code = nvm_read_buffer(
		(uint32_t)(SETTINGS_NVM_PAGE+(64*3)),
		memBuf,NVMCTRL_PAGE_SIZE);
	} while (error_code == STATUS_BUSY);
	
	if(error_code == STATUS_OK)
	{
		//check that the key is alright.
		if(memBuf[0] == SETTINGS_MASTER_KEY)
		{
			//the key is fine, copy the data over.
			memcpy(&(warmUpParameterValues[35]), &(memBuf[1]),20);
		}
		else
		{
			result = STATUS_FAIL;
		}
	}
	else
	{
		result = STATUS_FAIL;
	}
	return result; 
	
}


