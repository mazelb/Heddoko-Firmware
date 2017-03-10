/*
 * nvm_nvMemInterface.h
 *
 * Created: 9/22/2016 9:41:06 AM
 *  Author: Hriday Mehta
 */ 


#ifndef NVM_NVMEMINTERFACE_H_
#define NVM_NVMEMINTERFACE_H_

#include "common.h"

#define NVM_SETTINGS_VALID_SIGNATURE	0x900df00f
#define NVM_SETTINGS_NEW_FIRMWARE_FLAG  0xB16B00B5
#define NVM_MAX_SUIT_NAME_SIZE			10
#pragma pack(push, 1)
typedef struct  
{
	uint32_t validSignature;					// to check if the settings have been previously written
	char serialNumber[NVM_MAX_SUIT_NAME_SIZE];	// serial number in the form of BPxxxxx (ex. BP12345)
}nvmSettings_t;
#pragma	pack(pop)
status_t nvm_writeToFlash(nvmSettings_t *p_settings, uint32_t signature);
status_t nvm_readFromFlash(nvmSettings_t *p_settings);
status_t nvm_eraseFlash(void);

#endif /* NVM_NVMEMINTERFACE_H_ */