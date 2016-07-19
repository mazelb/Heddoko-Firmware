/*
 * sys_systemManager.c
 *
 * Created: 5/24/2016 3:37:09 PM
 *  Author: sean
 */ 
/*
 * @brief: A free running task which initializes the peripherals on boot-up,
 *			listens to the other modules and sends commands, starts other task threads.
 */

#include <asf.h>
#include <stdarg.h>
#include "sys_systemManager.h"
#include "common.h"
#include "string.h"
#include "msg_messenger.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "task_SensorHandler.h"
#include "subp_subProcessor.h"
#include "dat_dataManager.h"

/*	Local static functions	*/

/*	Extern functions	*/
/*	Extern variables	*/





void sys_systemManagerTask(void* pvParameters)
{

	uint16_t count = 0;
	status_t status = STATUS_PASS;
	msg_message_t eventMessage;
	
	
	//queue_systemManager = xQueueCreate(10, sizeof(msg_message_t));
	//if (queue_systemManager != 0)
	//{
		//msg_registerForMessages(MODULE_SYSTEM_MANAGER, 0xff, queue_systemManager);
	//}

	
	while (1)
	{		

		
		//if(xQueueReceive(queue_systemManager, &(eventMessage), 1) == true)
		//{
			//
		//}
		
		//checkGpioInt();
		vTaskDelay(100);
	}
}




