/*
 * nvm_nvMemInterface.c
 *
 * Created: 9/22/2016 9:41:25 AM
 *  Author: Hriday Mehta
 */ 

#include <asf.h>
#include <string.h>
#include "nvm_nvMemInterface.h"

uint32_t memBuffer[128] = {0};

/*	Local variables	*/
nvmSettings_t defaultSettings =		// default settings that are loaded if the flash has not been written before
{
	.validSignature = NVM_SETTINGS_VALID_SIGNATURE,
	.serialNumber = "BPxxxxx"
};

/*	Function definitions	*/

/***********************************************************************************************
 * nvm_writeToFlash(nvmSettings_t *p_settings, uint32_t size)
 * @brief write settings structure to flash
 * @param nvmSettings_t *p_settings: pointer to external structure holding settings data,
 *			uint32_t size: size of the external structure
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t nvm_writeToFlash(nvmSettings_t *p_settings, uint32_t signature)
{
	uint32_t size = sizeof(nvmSettings_t); 
    if (p_settings == NULL)
	{
		// it is an invalid pointer, return
		return STATUS_FAIL;
	}
	//erase the user signature. 
    flash_erase_user_signature();
    
	p_settings->validSignature = signature;	// assign a valid signature before writing
	memcpy(memBuffer, p_settings, sizeof(nvmSettings_t)); 
	// write the data to flash
	if (flash_write_user_signature(memBuffer, 128) == 0)
	{
		// successful
		return STATUS_PASS;
	} 
	else
	{
		// failed to write
		//dbg_printString(DBG_LOG_LEVEL_ERROR, "Failed to write to Flash\r\n");
		return STATUS_FAIL;
	}
}

/***********************************************************************************************
 * nvm_readFromFlash(nvmSettings_t *p_settings, uint32_t size)
 * @brief Read settings structure from flash
 * @param nvmSettings_t *p_settings: pointer to external structure holding settings data,
 *			uint32_t size: size of the external structure
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t nvm_readFromFlash(nvmSettings_t *p_settings)
{
	uint32_t size = sizeof(nvmSettings_t)/ sizeof(uint32_t); 
    
    if (p_settings == NULL)
	{
		// it is an invalid pointer, return
		return STATUS_FAIL;
	}
	
	// read the data from flash
	if (flash_read_user_signature(memBuffer, 128) == 0)
	{
		memcpy(p_settings,memBuffer,sizeof(nvmSettings_t)); 
        if (p_settings->validSignature != NVM_SETTINGS_VALID_SIGNATURE && p_settings->validSignature != NVM_SETTINGS_NEW_FIRMWARE_FLAG)
		{
			// write the default settings 
			if (nvm_writeToFlash(&defaultSettings,NVM_SETTINGS_VALID_SIGNATURE)	== STATUS_PASS)
			{
				memcpy(p_settings, &defaultSettings, sizeof(nvmSettings_t));
				return STATUS_PASS;	// default settings written successfully
			}
			else
			{
				return STATUS_FAIL;	// failed to write default settings
			}
		}
		return STATUS_PASS;
	} 
	else
	{
		// failed to write
		//dbg_printString(DBG_LOG_LEVEL_ERROR, "Failed to read Flash\r\n");
		return STATUS_FAIL;
	}
}

/***********************************************************************************************
 * nvm_eraseFlash(void)
 * @brief Erase the flash
 * @param void
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t nvm_eraseFlash(void)
{
	flash_erase_user_signature();
	return STATUS_PASS;
}