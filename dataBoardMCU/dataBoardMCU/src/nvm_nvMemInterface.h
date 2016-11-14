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
#include "subp_subProcessor.h"
#include "dbg_debugManager.h"

#define NVM_SETTINGS_VALID_SIGNATURE	0x900df00f
#define NVM_MAX_SUIT_NAME_SIZE			10
#pragma pack(push, 1)
typedef struct  
{
	uint32_t validSignature;					// to check if the settings have been previously written
	char serialNumber[NVM_MAX_SUIT_NAME_SIZE];	// serial number in the form of BPxxxxx (ex. BP12345)
	bool hapticEnable;							// enable / disable haptic motor
	bool piezoEnable;							// enable / disable piezo buzzer
	net_wirelessConfig_t defaultWifiConfig;		// wifi configuration 
	uint16_t serverPortNumber;					// server port number
	subp_recordingConfig_t recordingCfg;        // recording parameters 
	subp_streamConfig_t streamCfg;              // stream configuration 
	uint16_t advPortNumber;						// advertisement port number
    dbg_debugConfiguration_t debugCfg;
}nvmSettings_t;
#pragma	pack(pop)
status_t nvm_writeToFlash(nvmSettings_t *p_settings);
status_t nvm_readFromFlash(nvmSettings_t *p_settings);
status_t nvm_eraseFlash(void);

#endif /* NVM_NVMEMINTERFACE_H_ */