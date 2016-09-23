/*
 * nvm_nvMemInterface.h
 *
 * Created: 9/22/2016 9:41:06 AM
 *  Author: Hriday Mehta
 */ 


#ifndef NVM_NVMEMINTERFACE_H_
#define NVM_NVMEMINTERFACE_H_

#include "common.h"
#include "net_wirelessNetwork.h"

#define NVM_SETTINGS_VALID_SIGNATURE	0x900df00d
#define NVM_MAX_SUIT_NAME_SIZE			10

typedef struct  
{
	uint32_t validSignature;					// to check if the settings have been previously written
	char suitNumber[NVM_MAX_SUIT_NAME_SIZE];	// suit number in the form of Sxxxxx (viz. S12345)
	uint8_t debugLevel;							// default debug level for the particular brain pack
	bool hapticEnable;							// enable / disable haptic motor
	bool piezoEnable;							// enable / disable piezo buzzer
	net_wirelessConfig_t defaultWifiConfig;		// default wifi configuration 
	uint16_t serverPortNumber;					// default server port number
	uint16_t streamPortNumber;					// default stream port number
	uint16_t debugPortNumber;					// default debug port number
	uint16_t advPortNumber;						// default advertisement port number
}nvmSettings_t;

status_t nvm_writeToFlash(nvmSettings_t *p_settings, uint32_t size);
status_t nvm_readFromFlash(nvmSettings_t *p_settings, uint32_t size);
status_t nvm_eraseFlash(void);

#endif /* NVM_NVMEMINTERFACE_H_ */