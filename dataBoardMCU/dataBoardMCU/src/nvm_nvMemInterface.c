/*
 * nvm_nvMemInterface.c
 *
 * Created: 9/22/2016 9:41:25 AM
 *  Author: Hriday Mehta
 */ 

#include <asf.h>
#include "nvm_nvMemInterface.h"
#include "dbg_debugManager.h"

/*	Local variables	*/
nvmSettings_t defaultSettings = 
{
	.validSignature = NULL,
	.suitNumber = "Sxxxxx",
	.hapticEnable = true,
	.piezoEnable = true,
	.debugLevel = DBG_LOG_LEVEL_ERROR,
	.serverPortNumber = 0,
	.streamPortNumber = 0,
	.debugPortNumber = 0,
	.defaultWifiConfig =
	{
		.channel = NULL,
		.passphrase = NULL,
		.securityType = M2M_WIFI_SEC_INVALID,
		.ssid = NULL
	}
};

/*	Function definitions	*/

/***********************************************************************************************
 * nvm_writeToFlash(nvmSettings_t *p_settings, uint32_t size)
 * @brief write settings structure to flash
 * @param nvmSettings_t *p_settings: pointer to external structure holding settings data,
 *			uint32_t size: size of the external structure
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t nvm_writeToFlash(nvmSettings_t *p_settings, uint32_t size)
{
	if (p_settings == NULL)
	{
		// it is an invalid pointer, return
		return STATUS_FAIL;
	}
	
	p_settings->validSignature = NVM_SETTINGS_VALID_SIGNATURE;	// assign a valid signature before writing
	
	// write the data to flash
	if (flash_write_user_signature(p_settings, size) == 0)
	{
		// successful
		return STATUS_PASS;
	} 
	else
	{
		// failed to write
		dbg_printString(DBG_LOG_LEVEL_ERROR, "Failed to write to Flash\r\n");
		return STATUS_FAIL;
	}
}

/***********************************************************************************************
 * nvm_writeToFlash(nvmSettings_t *p_settings, uint32_t size)
 * @brief Read settings structure from flash
 * @param nvmSettings_t *p_settings: pointer to external structure holding settings data,
 *			uint32_t size: size of the external structure
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t nvm_readFromFlash(nvmSettings_t *p_settings, uint32_t size)
{
	if (p_settings == NULL)
	{
		// it is an invalid pointer, return
		return STATUS_FAIL;
	}
	
	// read the data from flash
	if (flash_read_user_signature(p_settings, size) == 0)
	{
		if (p_settings->validSignature != NVM_SETTINGS_VALID_SIGNATURE)
		{
			// the flash has not been written before.
			dbg_printString(DBG_LOG_LEVEL_WARNING, "Flash has never been written to\r\n");
			// write the default settings 
			
			return STATUS_FAIL;
		}
		return STATUS_PASS;
	} 
	else
	{
		// failed to write
		dbg_printString(DBG_LOG_LEVEL_ERROR, "Failed to read Flash\r\n");
		return STATUS_FAIL;
	}
}