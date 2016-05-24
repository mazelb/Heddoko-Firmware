/*
 * msg_messenger.c
 *
 * Created: 5/24/2016 1:31:30 PM
 *  Author: sean
 */ 

#include "msg_messenger.h"

//the message boxes, one for each module, each queue must be initialized to NULL.
msg_messageBox_t messageBoxes[] =
{
	{MODULE_SYSTEM_MANAGER,NULL,NULL},
	{MODULE_SDCARD,NULL,NULL},
	{MODULE_SENSOR_HANDLER,NULL,NULL},
	{MODULE_WIFI,NULL,NULL},
	{MODULE_COMMAND,NULL,NULL},
	{MODULE_DEBUG,NULL,NULL}
};


status_t msg_registerForMessages(modules_t module, uint32_t messageMask,xQueueHandle messageQueue)
{
	status_t status = STATUS_PASS;
	messageBoxes[module].queue = messageQueue;
	messageBoxes[module].messageMask = messageMask;
	return status;
}
status_t msg_sendBroadcastMessage(msg_message_t* message)
{
	status_t status = STATUS_PASS;
	int i =0;
	for(i=0;i<MODULE_NUMBER_OF_MODULES;i++)
	{
		//Check the mask for the module to see if the module accepts the message
		if((messageBoxes[i].messageMask & (1<<message->type)) > 0)
		{
			if(messageBoxes[i].queue != NULL)
			{
				if(xQueueSendToBack( messageBoxes[i].queue,( void * ) &message,10 ) != TRUE)
				{
					vTaskDelay(1);
				}				
			}
		}
	}
	return status;
}
status_t msg_sendMessage(modules_t module, msg_message_t* message)
{
	status_t status = STATUS_PASS;
	if(messageBoxes[module].queue != NULL)
	{
		if(xQueueSendToBack( messageBoxes[module].queue,( void * ) &message,10 ) != TRUE)
		{
			vTaskDelay(1);
		}
	}
	return status;
}