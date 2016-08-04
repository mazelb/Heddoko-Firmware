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
#include "subp_subProcessor.h"
#include "dat_dataManager.h"
#include "dbg_debugManager.h"

/* Global Variables */
xQueueHandle queue_systemManager = NULL;
sys_manager_systemState_t currentState = SYSTEM_STATE_INIT; 
/*	Local static functions	*/
static void sendStateChangeMessage(sys_manager_systemState_t state);
/*	Extern functions	*/
/*	Extern variables	*/





void sys_systemManagerTask(void* pvParameters)
{

	uint16_t count = 0;
	status_t status = STATUS_PASS;
	msg_message_t eventMessage;
	//initialize the GPIO here... possibly split it up later
	drv_gpio_initializeAll();
	
	queue_systemManager = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_systemManager != 0)
	{
		msg_registerForMessages(MODULE_SYSTEM_MANAGER, 0xff, queue_systemManager);
	}
	//start the other tasks
	if (xTaskCreate(dbg_debugTask, "DBG", TASK_DEBUG_MANAGER_STACK_SIZE, NULL, TASK_DEBUG_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sub process handler task\r\n");
	}	
	if (xTaskCreate(subp_subProcessorTask, "SP", TASK_SUB_PROCESS_MANAGER_STACK_SIZE, NULL, TASK_SUB_PROCESS_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sub process handler task\r\n");
	}
	if (xTaskCreate(sdc_sdCardTask, "SDC", TASK_SD_CARD_STACK_SIZE, NULL, TASK_SD_CARD_PRIORITY, NULL) != pdPASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create sd card task\r\n");
	}
	vTaskDelay(200); 
	sendStateChangeMessage(SYSTEM_STATE_INIT); 
	
	//vTaskDelay(5000); 
	//sendStateChangeMessage(SYSTEM_STATE_RECORDING);
	
	while (1)
	{		
		if(xQueueReceive(queue_systemManager, &(eventMessage), 1) == true)
		{
			
		}
		wdt_restart(WDT);		
		vTaskDelay(100);
	}
}




static void sendStateChangeMessage(sys_manager_systemState_t state)
{
	msg_message_t message = {.source = MODULE_SYSTEM_MANAGER, .data = state, .type = MSG_TYPE_ENTERING_NEW_STATE};
	msg_sendBroadcastMessage(&message);
}



