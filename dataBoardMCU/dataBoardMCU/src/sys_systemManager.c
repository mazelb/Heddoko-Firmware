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
#include "dat_dataManager.h"
#include "dbg_debugManager.h"
#include "net_wirelessNetwork.h"
#include "ble_bluetoothManager.h"
#include "drv_haptic.h"
#include "drv_piezo.h"
#include "gpm_gpioManager.h"

/* Global Variables */
xQueueHandle queue_systemManager = NULL;
sys_manager_systemState_t currentState = SYSTEM_STATE_INIT; 

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
	{4000, 300},
	//{000, 250}
};
drv_piezo_noteElement_t startRecordingTone[] =
{
    {2500, 150},
    //{000, 250},
    {3000, 150},
    //{000, 250},
    {3500, 150},
    //{000, 250}
};
drv_piezo_noteElement_t stopRecordingTone[] =
{
    {3500, 150},
    //{000, 250},
    {3000, 150},
    //{000, 250},
    {2500, 150},
    //{000, 250}
};
drv_piezo_noteElement_t errorTone[] =
{
    {800, 300},
    //{000, 250},
    {800, 300},
    //{000, 250},
};


drv_haptic_config_t hapticConfig =
{
	.hapticGpio = DRV_GPIO_PIN_HAPTIC_OUT,
	.onState = DRV_GPIO_PIN_STATE_LOW,
	.offState = DRV_GPIO_PIN_STATE_HIGH
};
drv_haptic_patternElement_t hapticPatternArray[] =
{
	{100, 1}
};

/*	Local static functions	*/
static void sendStateChangeMessage(sys_manager_systemState_t state);
static void processMessage(msg_message_t message);
static void processButtonEvent(uint32_t data);
/*	Extern functions	*/
/*	Extern variables	*/


//Delete me after testing complete

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
	drv_led_init(&ledConfiguration);
	drv_led_set(DRV_LED_BLUE,DRV_LED_FLASH);
	
	drv_piezo_init(&piezoConfig);
	//drv_piezo_playPattern(noteElementsArray, (sizeof(noteElementsArray) / sizeof(drv_piezo_noteElement_t)));
	
	drv_haptic_init(&hapticConfig);
	drv_haptic_playPattern(hapticPatternArray, (sizeof(hapticPatternArray) / sizeof(drv_haptic_patternElement_t)));
	
	vTaskDelay(200);
	queue_systemManager = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_systemManager != 0)
	{
		msg_registerForMessages(MODULE_SYSTEM_MANAGER, 0xff, queue_systemManager);
	}
	//start the other tasks
	if (xTaskCreate(dbg_debugTask, "dbg", TASK_DEBUG_MANAGER_STACK_SIZE, NULL, TASK_DEBUG_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sub process handler task\r\n");
	}	
	if(xTaskCreate(gpm_gpioManagerTask, "gpm", (3000/sizeof(portSTACK_TYPE)), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
    	dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create gpm task\r\n");
	} 
	if (xTaskCreate(subp_subProcessorTask, "subp", TASK_SUB_PROCESS_MANAGER_STACK_SIZE, NULL, TASK_SUB_PROCESS_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sub process handler task\r\n");
	}
	if (xTaskCreate(sdc_sdCardTask, "sdc", TASK_SD_CARD_STACK_SIZE, NULL, TASK_SD_CARD_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sd card task\r\n");
	}
	if(xTaskCreate(net_wirelessNetworkTask, "wif", (4000/sizeof(portSTACK_TYPE)), NULL, tskIDLE_PRIORITY+4, NULL) != pdPASS)
	{
    	dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create wireless task\r\n");
	}
	if(xTaskCreate(ble_bluetoothManagerTask, "ble", (4000/sizeof(portSTACK_TYPE)), NULL, tskIDLE_PRIORITY+3, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create ble task\r\n");
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
    switch(message.type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		break;
		case MSG_TYPE_ERROR:
     
		break;
		case MSG_TYPE_SDCARD_STATE:
            if(message.data == SD_CARD_MOUNTED)
            {
                if(currentState == SYSTEM_STATE_INIT || currentState == SYSTEM_STATE_ERROR)
                {
                    currentState = SYSTEM_STATE_IDLE;
                    msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE); 
                    drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID); 
                }   
            }
            else
            {
                currentState = SYSTEM_STATE_ERROR;
                drv_piezo_playPattern(errorTone, (sizeof(errorTone) / sizeof(drv_piezo_noteElement_t)));             
                msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_ERROR);
                drv_led_set(DRV_LED_YELLOW, DRV_LED_FLASH);   
            }                                
		break;
		case MSG_TYPE_WIFI_STATE:
        
		break;
		case MSG_TYPE_GPM_BUTTON_EVENT:
		{
			processButtonEvent(message.data);
			
		}
		break;
		case MSG_TYPE_SUBP_POWER_DOWN_REQ:
		//right now just send back the power down ready message right away. 
		msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_SYSTEM_MANAGER, MSG_TYPE_SUBP_POWER_DOWN_READY,NULL);
		break;
		case MSG_TYPE_SUBP_STATUS:
		    subpReceivedStatus = (subp_status_t*)message.parameters;		
		    if(currentState == SYSTEM_STATE_RECORDING)
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
                        currentState = SYSTEM_STATE_IDLE; // go to error state. 
                        drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);  
                    }
                }   
            }                
		break;        
		default:
		break;
		
	}
}

static void sendStateChangeMessage(sys_manager_systemState_t state)
{
	msg_message_t message = {.source = MODULE_SYSTEM_MANAGER, .data = state, .type = MSG_TYPE_ENTERING_NEW_STATE};
	msg_sendBroadcastMessage(&message);
}

static void processButtonEvent(uint32_t data)
{
	switch (data)
	{
		case GPM_BUTTON_ONE_SHORT_PRESS:
                          
		break;
		case GPM_BUTTON_ONE_LONG_PRESS:
		break;
		case GPM_BUTTON_TWO_SHORT_PRESS:
            if(currentState == SYSTEM_STATE_IDLE)
            {
                msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_RECORDING);        
                currentState = SYSTEM_STATE_RECORDING;                
                drv_led_set(DRV_LED_RED, DRV_LED_SOLID); 
                drv_piezo_playPattern(startRecordingTone, (sizeof(startRecordingTone) / sizeof(drv_piezo_noteElement_t)));
            }
            else if(currentState == SYSTEM_STATE_RECORDING)
            {
                msg_sendBroadcastMessageSimple(MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, SYSTEM_STATE_IDLE);        
                currentState = SYSTEM_STATE_IDLE;                
                drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);      
                drv_piezo_playPattern(stopRecordingTone, (sizeof(stopRecordingTone) / sizeof(drv_piezo_noteElement_t)));             
            }              
		break;
		case GPM_BUTTON_TWO_LONG_PRESS:
        
		break;
		case GPM_BOTH_BUTTON_LONG_PRESS:
        //restart the system. 
		break;
		default:
		break;
	}
}
