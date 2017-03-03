/*
 * sys_systemManager.c
 *
 * Created: 5/24/2016 3:37:09 PM
 *  Author: sean
 */ 
/*
 * @brief: A free running task which listens to the other modules and sends commands, starts other task threads.
 */

#include <asf.h>
#include <stdarg.h>
#include "sys_systemManager.h"
#include "common.h"
#include "string.h"
#include "msg_messenger.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "drv_led.h"
#include "subp_subProcessor.h"
#include "dbg_debugManager.h"
#include "net_wirelessNetwork.h"
#include "ble_bluetoothManager.h"
#include "drv_haptic.h"
#include "drv_piezo.h"
#include "gpm_gpioManager.h"
#include "nvm_nvMemInterface.h"
#include "cfg_configurationManager.h"
#include "tftp_fileTransferClient.h"

/* Global Variables */
xQueueHandle queue_systemManager = NULL;
sys_manager_systemState_t sgCurrentState = SYSTEM_STATE_INIT; 
static net_wifiState_t sysNetworkState = NET_WIFI_STATE_INIT;

drv_piezo_config_t piezoConfig =
{
	.gpioPin = DRV_GPIO_PIN_PIEZO_OUT
};
drv_piezo_noteElement_t noteElementsArray[] =
{
	{900, 300},
	//{000, 250},
	{2500, 300},
	//{000, 250},
	{4000, 300}
	//{000, 250}
};
drv_piezo_noteElement_t startRecordingTone[] =
{
    {2500, 150},
    //{000, 250},
    {3000, 150},
    //{000, 250},
    {3500, 150}
    //{000, 250}
};
drv_piezo_noteElement_t stopRecordingTone[] =
{
    {3500, 150},
    //{000, 250},
    {3000, 150},
    //{000, 250},
    {2500, 150}
    //{000, 250}
};
drv_piezo_noteElement_t errorTone[] =
{
    {800, 300},
    //{000, 250},
    {800, 300}
    //{000, 250},
};
drv_piezo_noteElement_t btnPress[] =
{
    {2500, 250},
    {3000, 250}    
};
drv_piezo_noteElement_t wifiTone[] =
{
    {2500, 300},
    //{000, 250},
    {2500, 300},
    //{000, 250},
    {2500, 300}
};
drv_piezo_noteElement_t heddokoTone[] =
{
    {800, 600},
    //{000, 100},
    {800, 600},
    {800, 600},
    {2000, 900}
    //{000, 100},
    //{1800, 1000}
};
drv_haptic_config_t hapticConfig =
{
	.hapticGpio = DRV_GPIO_PIN_HAPTIC_OUT,
	.onState = DRV_GPIO_PIN_STATE_HIGH,
	.offState = DRV_GPIO_PIN_STATE_LOW
};
drv_haptic_patternElement_t hapticPatternArray[] =
{
	{100, 1}
};

drv_haptic_patternElement_t buttonPress[] =
{
    {100, 100},{50, 50},{100, 100}
};
net_wirelessConfig_t wirelessConfiguration =
{
    .securityType = M2M_WIFI_SEC_WPA_PSK,
    .passphrase = "qwerty123",//"heddoko123",//"test$891",
    .ssid = "Heddoko_NoNetTest2Ghz",//"heddokoTestNet",//"HeddokoTest2ghz",
    .channel = 255, //default to 255 so it searches all channels for the signal
};
const char* versionStr = VERSION;
const char* modelStr = MODEL;
const char* hwRevisionStr = HARDWARE_REV;

nvmSettings_t currentSystemSettings; 
bool settingsChanges = false; 
subp_moduleConfig_t subProcessorSettings = {0,0}; 
net_moduleConfig_t wirelessNetworkSettings;    
sdc_moduleConfig_t sdCardSettings;  
cfg_moduleConfig_t configModuleSettings;
ble_moduleConfig_t bleModuleSettings;


/*	Local static functions	*/
static void processToggleRecordingEvent(uint32_t requestedState);
static void sendStateChangeMessage(sys_manager_systemState_t state);
static void processMessage(msg_message_t message);
static void processWifiControlEvent(uint32_t requestedState);
static void processButtonEvent(uint32_t data);
/*	Extern functions	*/
/*	Extern variables	*/


void sys_systemManagerTask(void* pvParameters)
{

	uint16_t count = 0;
	status_t status = STATUS_PASS;
	msg_message_t eventMessage;
	//initialize the GPIO here... possibly split it up later
	drv_gpio_initializeAll();
	drv_led_config_t ledConfiguration =
	{
		.redLed = DRV_GPIO_PIN_RED_LED,
		.greenLed = DRV_GPIO_PIN_GREEN_LED,
		.blueLed = DRV_GPIO_PIN_BLUE_LED
	};
	if(nvm_readFromFlash(&currentSystemSettings) != STATUS_PASS)
    {        
        //umm this is pretty bad, we should do something here.    
    }
    //copy over the settings to the module configuration structures.  
    subProcessorSettings.recordingConfig = &(currentSystemSettings.recordingCfg); 
    subProcessorSettings.streamConfig = &(currentSystemSettings.streamCfg); 
    wirelessNetworkSettings.advertisingInterval = 5;
    wirelessNetworkSettings.advertisingPort = &(currentSystemSettings.advPortNumber);
    wirelessNetworkSettings.configurationPort = &(currentSystemSettings.serverPortNumber); 
    wirelessNetworkSettings.serialNumber = currentSystemSettings.serialNumber; 
    
    sdCardSettings.serialNumber = currentSystemSettings.serialNumber;
    
    configModuleSettings.serialNumber = currentSystemSettings.serialNumber;
    configModuleSettings.configPort = currentSystemSettings.serverPortNumber; 
    
    bleModuleSettings.serialNumber = currentSystemSettings.serialNumber;
    bleModuleSettings.fwVersion = versionStr;
    bleModuleSettings.modelString = modelStr;
    bleModuleSettings.hwRevision = hwRevisionStr; 
    bleModuleSettings.wirelessConfig = &wirelessConfiguration;
    
    
    drv_led_init(&ledConfiguration);
	drv_led_set(DRV_LED_BLUE,DRV_LED_FLASH);
	
	drv_piezo_init(&piezoConfig);
	//drv_piezo_playPattern(noteElementsArray, (sizeof(noteElementsArray) / sizeof(drv_piezo_noteElement_t)));
	//drv_piezo_togglePiezo(false);
	drv_haptic_init(&hapticConfig);
	drv_haptic_playPattern(hapticPatternArray, (sizeof(hapticPatternArray) / sizeof(drv_haptic_patternElement_t)));
	
	vTaskDelay(200);
	queue_systemManager = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_systemManager != 0)
	{
		msg_registerForMessages(MODULE_SYSTEM_MANAGER, 0xff, queue_systemManager);
	}
	//start the other tasks
	if (xTaskCreate(dbg_debugTask, "dbg", TASK_DEBUG_MANAGER_STACK_SIZE, &(currentSystemSettings.debugCfg), TASK_DEBUG_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sub process handler task\r\n");
	}	
	if(xTaskCreate(gpm_gpioManagerTask, "gpm", (3000/sizeof(portSTACK_TYPE)), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
    	dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create gpm task\r\n");
	} 
	if (xTaskCreate(subp_subProcessorTask, "subp", TASK_SUB_PROCESS_MANAGER_STACK_SIZE, &subProcessorSettings, TASK_SUB_PROCESS_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sub process handler task\r\n");
	}
	if (xTaskCreate(sdc_sdCardTask, "sdc", TASK_SD_CARD_STACK_SIZE, &sdCardSettings, TASK_SD_CARD_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sd card task\r\n");
	}
	if(xTaskCreate(net_wirelessNetworkTask, "wif", (4000/sizeof(portSTACK_TYPE)), &wirelessNetworkSettings, tskIDLE_PRIORITY+4, NULL) != pdPASS)
	{
    	dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create wireless task\r\n");
	}    
	if(xTaskCreate(ble_bluetoothManagerTask, "ble", (4000/sizeof(portSTACK_TYPE)), &bleModuleSettings, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create ble task\r\n");
	}
	if(xTaskCreate(cfg_configurationTask, "cfg", (4000/sizeof(portSTACK_TYPE)), &configModuleSettings, tskIDLE_PRIORITY+3, NULL) != pdPASS)
	{
    	dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create cfg task\r\n");
	}
	if(xTaskCreate(tftp_FileTransferTask, "tftp", (4000/sizeof(portSTACK_TYPE)), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
    	dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create cfg task\r\n");
	}
    vTaskDelay(200); 
	sendStateChangeMessage(SYSTEM_STATE_INIT); 	
    
    
	while (1)
	{		
		if(xQueueReceive(queue_systemManager, &(eventMessage), 1) == true)
		{
			processMessage(eventMessage);
		}
		wdt_restart(WDT);		
		vTaskDelay(100);
	}
}

static void processMessage(msg_message_t message)
{
	subp_status_t* subpReceivedStatus = NULL; 
    switch(message.msgType)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		break;
		case MSG_TYPE_ERROR:
            drv_piezo_playPattern(errorTone, (sizeof(errorTone) / sizeof(drv_piezo_noteElement_t)));
            //drv_piezo_playPattern(errorTone, (sizeof(errorTone) / sizeof(drv_piezo_noteElement_t)));
            if(sgCurrentState == SYSTEM_STATE_RECORDING || sgCurrentState == SYSTEM_STATE_STREAMING)
            {
                //go back to idle if the error is from the subprocessor. 
                if(message.source == MODULE_SUB_PROCESSOR)
                {
                     msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
                     sgCurrentState = SYSTEM_STATE_IDLE;
                     if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
                     {
                         drv_led_set(DRV_LED_TURQUOISE, DRV_LED_SOLID);
                     }
                     else
                     {
                         drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);
                     }                                       
                }   
            }                
		break;
		case MSG_TYPE_SDCARD_STATE:
            if(message.data == SD_CARD_MOUNTED)
            {
                if(sgCurrentState == SYSTEM_STATE_INIT || sgCurrentState == SYSTEM_STATE_ERROR)
                {
                    sgCurrentState = SYSTEM_STATE_IDLE;
                    msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE); 
                    drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID); 
                }   
            }
            else
            {
                sgCurrentState = SYSTEM_STATE_ERROR;
                drv_piezo_playPattern(errorTone, (sizeof(errorTone) / sizeof(drv_piezo_noteElement_t)));             
                msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_ERROR);
                drv_led_set(DRV_LED_YELLOW, DRV_LED_FLASH);   
            }                                
		break;
		case MSG_TYPE_WIFI_STATE:
            if(message.data == NET_WIFI_STATE_CONNECTED)
            {
                if(sgCurrentState == SYSTEM_STATE_IDLE)
                {
                    drv_led_set(DRV_LED_TURQUOISE, DRV_LED_SOLID); 
                }
            }
            else if(message.data == NET_WIFI_STATE_DISCONNECTED)
            {
                if(sysNetworkState != NET_WIFI_STATE_CONNECTED)
                {
                    //since we were trying to connect, this is an error. 
                    drv_piezo_playPattern(errorTone, (sizeof(errorTone) / sizeof(drv_piezo_noteElement_t)));             
                }
                if(sgCurrentState == SYSTEM_STATE_IDLE)
                {
                    drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID); 
                }
            }
            sysNetworkState = message.data; 
            
		break;
		case MSG_TYPE_GPM_BUTTON_EVENT:
		{
			drv_haptic_playPattern(buttonPress, (sizeof(buttonPress) / sizeof(drv_haptic_patternElement_t)));
            processButtonEvent(message.data);			
		}
		break;
		case MSG_TYPE_SUBP_POWER_DOWN_REQ:
			if(settingsChanges == true)
            {
                nvm_writeToFlash(&currentSystemSettings, NVM_SETTINGS_VALID_SIGNATURE);    
            }                
			msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_SYSTEM_MANAGER, MSG_TYPE_SUBP_POWER_DOWN_READY,NULL);
		break;
        case MSG_TYPE_STREAM_CONFIG:
            //copy over the settings, set the flag to let the system know it needs to save the settings before powering down. 
            settingsChanges = true; 
            memcpy(&(currentSystemSettings.streamCfg),message.parameters, sizeof(subp_streamConfig_t));
        break;
        case MSG_TYPE_RECORDING_CONFIG:
            //copy over the settings, set the flag to let the system know it needs to save the settings before powering down.
            settingsChanges = true; 
            memcpy(&(currentSystemSettings.recordingCfg),message.parameters, sizeof(subp_recordingConfig_t));
        break;
        case MSG_TYPE_STREAM_REQUEST:
             dbg_printf(DBG_LOG_LEVEL_DEBUG,"Received Stream Request: %d\r\n",message.data);	
             if(message.data == 1)
             {            
                 if(sgCurrentState == SYSTEM_STATE_IDLE)
                 {
                     if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
                     {
                         msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_STREAMING);
                         sgCurrentState = SYSTEM_STATE_STREAMING;
                         drv_led_set(DRV_LED_PURPLE, DRV_LED_SOLID);
                         drv_piezo_playPattern(startRecordingTone, (sizeof(startRecordingTone) / sizeof(drv_piezo_noteElement_t)));
                     }
                 }
             }
             else
             {
                 if(sgCurrentState == SYSTEM_STATE_STREAMING)
                 {
                    msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
                    sgCurrentState = SYSTEM_STATE_IDLE;
                    if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
                    {
                        drv_led_set(DRV_LED_TURQUOISE, DRV_LED_SOLID);
                    }
                    else
                    {
                        drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);
                    }
                    
                    drv_piezo_playPattern(stopRecordingTone, (sizeof(stopRecordingTone) / sizeof(drv_piezo_noteElement_t)));                  
                 }
             }                                                              
        break; 
        case MSG_TYPE_WIFI_CONFIG:
            settingsChanges = true;  //set flag to have settings saved
            memcpy(&(currentSystemSettings.defaultWifiConfig),message.parameters, sizeof(net_wirelessConfig_t));
            
        break;
		case MSG_TYPE_SUBP_STATUS:
		    subpReceivedStatus = (subp_status_t*)message.parameters;		
		    if(sgCurrentState == SYSTEM_STATE_RECORDING || sgCurrentState == SYSTEM_STATE_STREAMING)
            {
                if(subpReceivedStatus->streamState != 0) //stream state not equal to idle
                {
                    if(subpReceivedStatus->sensorMask == 0)
                    {
                        //no sensors were detected, we need to report that theres an error and end the recording. 
                        drv_led_set(DRV_LED_YELLOW, DRV_LED_FLASH);  
                        drv_piezo_playPattern(errorTone, (sizeof(errorTone) / sizeof(drv_piezo_noteElement_t)));
                        msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
                        vTaskDelay(200); 
                        sgCurrentState = SYSTEM_STATE_IDLE; // go to error state. 
                        if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
                        {
                            drv_led_set(DRV_LED_TURQUOISE, DRV_LED_SOLID); 
                        }
                        else
                        {
                            drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);      
                        }
                        
                    }
                }   
            }                
		break;
        case MSG_TYPE_SET_SERIAL:
           //always write to flash when setting the serial number. 
           strncpy(currentSystemSettings.serialNumber, message.parameters, 10); 
           nvm_writeToFlash(&currentSystemSettings, NVM_SETTINGS_VALID_SIGNATURE);             
        break;      
        case MSG_TYPE_SAVE_SETTINGS:
            nvm_writeToFlash(&currentSystemSettings, NVM_SETTINGS_VALID_SIGNATURE);             
        break;  
        case MSG_TYPE_TOGGLE_RECORDING:
            processToggleRecordingEvent(message.data);
        break;
        case MSG_TYPE_WIFI_CONTROL:
            processWifiControlEvent(message.data); 
		break;
        case MSG_TYPE_REQUEST_STATE:
            //TODO Add some changes here
            sendStateChangeMessage(message.data);
            sgCurrentState = message.data; 
        break;
        case MSG_TYPE_FW_UPDATE_RESTART_REQUEST:
            nvm_writeToFlash(&currentSystemSettings, NVM_SETTINGS_NEW_FIRMWARE_FLAG);
            subp_sendForcedRestartMessage();
        break;
        default:
		break;		
	}
}

static void sendStateChangeMessage(sys_manager_systemState_t state)
{
	msg_message_t message = {.source = MODULE_SYSTEM_MANAGER, .data = state, .msgType = MSG_TYPE_ENTERING_NEW_STATE};
	msg_sendBroadcastMessage(&message);
}
static void processToggleRecordingEvent(uint32_t requestedState)
{
    if(requestedState == 1) //start recording TODO: Add enum
    {
        if(sgCurrentState == SYSTEM_STATE_IDLE)
        {
            //start recording
            msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_RECORDING);
            sgCurrentState = SYSTEM_STATE_RECORDING;
            drv_led_set(DRV_LED_RED, DRV_LED_SOLID);
            drv_piezo_playPattern(startRecordingTone, (sizeof(startRecordingTone) / sizeof(drv_piezo_noteElement_t)));
        }
    }
    else if(requestedState == 2) //stop recording TODO: Add enum
    {
        if(sgCurrentState == SYSTEM_STATE_RECORDING)
        {
            msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
            sgCurrentState = SYSTEM_STATE_IDLE;
            drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);
            drv_piezo_playPattern(stopRecordingTone, (sizeof(stopRecordingTone) / sizeof(drv_piezo_noteElement_t)));
        }
        else if(sgCurrentState == SYSTEM_STATE_STREAMING)
        {
            msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
            sgCurrentState = SYSTEM_STATE_IDLE;
            if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
            {
                drv_led_set(DRV_LED_TURQUOISE, DRV_LED_SOLID);
            }
            else
            {
                drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);
            }
            drv_piezo_playPattern(stopRecordingTone, (sizeof(stopRecordingTone) / sizeof(drv_piezo_noteElement_t)));
        }
    }
}
static void processWifiControlEvent(uint32_t requestedState)
{
    if(requestedState == 1)
    {
        drv_piezo_playPattern(wifiTone, (sizeof(wifiTone) / sizeof(drv_piezo_noteElement_t)));            
        if(sysNetworkState != NET_WIFI_STATE_CONNECTED)
        {
            net_connectToNetwork(&(currentSystemSettings.defaultWifiConfig));
        }
    }
    else
    {
        if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
        {
            net_disconnectFromNetwork();
        }    
    }
 
}    
static void processButtonEvent(uint32_t data)
{
	switch (data)
	{
		case GPM_BUTTON_ONE_SHORT_PRESS:
            if(sgCurrentState == SYSTEM_STATE_IDLE)
            {
                if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
                {
                    msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_STREAMING);
                    sgCurrentState = SYSTEM_STATE_STREAMING;
                    drv_led_set(DRV_LED_PURPLE, DRV_LED_SOLID);
                    drv_piezo_playPattern(startRecordingTone, (sizeof(startRecordingTone) / sizeof(drv_piezo_noteElement_t)));                    
                }
                else
                {
                    msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_RECORDING);
                    sgCurrentState = SYSTEM_STATE_RECORDING;
                    drv_led_set(DRV_LED_RED, DRV_LED_SOLID);
                    drv_piezo_playPattern(startRecordingTone, (sizeof(startRecordingTone) / sizeof(drv_piezo_noteElement_t)));                 
                }

            }
            else if(sgCurrentState == SYSTEM_STATE_RECORDING)
            {
                msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
                sgCurrentState = SYSTEM_STATE_IDLE;
                drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);
                drv_piezo_playPattern(stopRecordingTone, (sizeof(stopRecordingTone) / sizeof(drv_piezo_noteElement_t)));
            }
            else if(sgCurrentState == SYSTEM_STATE_STREAMING)
            {
                msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);
                sgCurrentState = SYSTEM_STATE_IDLE;
                if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
                {
                    drv_led_set(DRV_LED_TURQUOISE, DRV_LED_SOLID);
                }
                else
                {
                    drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);    
                }                
                drv_piezo_playPattern(stopRecordingTone, (sizeof(stopRecordingTone) / sizeof(drv_piezo_noteElement_t)));                   
            }                                          
		break;
		case GPM_BUTTON_ONE_LONG_PRESS:
		break;
		case GPM_BUTTON_TWO_SHORT_PRESS:
			//drv_led_set(DRV_LED_RED, DRV_LED_SOLID);
			//drv_haptic_playPattern(hapticPatternArray, (sizeof(hapticPatternArray) / sizeof(drv_haptic_patternElement_t)));
			drv_piezo_playPattern(heddokoTone, (sizeof(heddokoTone) / sizeof(drv_piezo_noteElement_t)));
		break;
		case GPM_BUTTON_TWO_LONG_PRESS:
        //try to connect to the wifi network. 
        drv_piezo_playPattern(btnPress, (sizeof(btnPress) / sizeof(drv_piezo_noteElement_t)));                   
        
        if(sysNetworkState == NET_WIFI_STATE_CONNECTED)
        {
            net_disconnectFromNetwork(); 
        }
        else
        {
            net_connectToNetwork(&(currentSystemSettings.defaultWifiConfig));
        }
        
		break;
		case GPM_BOTH_BUTTON_LONG_PRESS:
        //restart the system. 
            subp_sendForcedRestartMessage();
		break;
		default:
		break;
	}
}
