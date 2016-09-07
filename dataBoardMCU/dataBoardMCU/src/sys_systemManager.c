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

/* Global Variables */
xQueueHandle queue_systemManager = NULL;
sys_manager_systemState_t currentState = SYSTEM_STATE_INIT; 

/*	Local static functions	*/
static void sendStateChangeMessage(sys_manager_systemState_t state);
static void processEvent(msg_message_t* message);
/*	Extern functions	*/
/*	Extern variables	*/


//Delete me after testing complete
void playSound(float duration, float frequency);


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
	drv_led_set(DRV_LED_GREEN,DRV_LED_SOLID);
	drv_gpio_setPinState(DRV_GPIO_PIN_HAPTIC_OUT, DRV_GPIO_PIN_STATE_HIGH);
	drv_led_set(DRV_LED_RED,DRV_LED_SOLID);
	playSound(200,900);
	drv_led_set(DRV_LED_WHITE,DRV_LED_SOLID);
	playSound(200,2500);
	drv_led_set(DRV_LED_BLUE,DRV_LED_SOLID);
	playSound(200,4000);
	//drv_gpio_setPinState(DRV_GPIO_PIN_HAPTIC_OUT, DRV_GPIO_PIN_STATE_LOW);
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
			processEvent(&eventMessage);
		}
		wdt_restart(WDT);		
		vTaskDelay(100);
	}
}


static void processEvent(msg_message_t* message)
{
	switch(message->type)
	{
		case MSG_TYPE_SDCARD_STATE:
		break;
		case MSG_TYPE_WIFI_STATE:
		break;
		case MSG_TYPE_SUBP_POWER_DOWN_REQ:
		//right now just send back the power down ready message right away. 
		msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_SYSTEM_MANAGER, MSG_TYPE_SUBP_POWER_DOWN_READY,NULL);
		break;
	}
}

static void sendStateChangeMessage(sys_manager_systemState_t state)
{
	msg_message_t message = {.source = MODULE_SYSTEM_MANAGER, .data = state, .type = MSG_TYPE_ENTERING_NEW_STATE};
	msg_sendBroadcastMessage(&message);
}

void playSound(float duration, float frequency)
{

	long int i,cycles;
	long half_period;
	float wavelength;
	
	wavelength=(1/frequency)*1000000;
	cycles= (long)((duration*1000)/wavelength);
	half_period = (long)(wavelength/2);
	for (i=0;i<cycles;i++)
	{
		delay_us(half_period);
		drv_gpio_setPinState(DRV_GPIO_PIN_PIEZO_OUT, DRV_GPIO_PIN_STATE_HIGH);
		delay_us(half_period);
		drv_gpio_setPinState(DRV_GPIO_PIN_PIEZO_OUT, DRV_GPIO_PIN_STATE_LOW);
	}

	return;
}



