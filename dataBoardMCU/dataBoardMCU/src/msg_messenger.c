/**
* @file msg_messenger.c
* @brief Interprocess messenger module, routes messages to each module. 
* @author Sean Cloghesy (sean@heddoko.com)
* @date May 2016
* Copyright Heddoko(TM) 2015, all rights reserved
*/
#include "msg_messenger.h"
#include "string.h"

//the message boxes, one for each module, each queue must be initialized to NULL.
msg_messageBox_t messageBoxes[MODULE_NUMBER_OF_MODULES] ={0};

status_t msg_registerForMessages(modules_t module, uint32_t messageMask, xQueueHandle messageQueue)
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
		if((messageBoxes[i].messageMask & (1<<message->msgType)) > 0)
		{
			if(messageBoxes[i].queue != NULL)
			{
				if(xQueueSendToBack( messageBoxes[i].queue,( void * ) message,10 ) != true)
				{
					vTaskDelay(1);
				}				
			}
		}
	}
	return status;
}
status_t msg_sendMessage(modules_t destModule, modules_t sourceModule, msg_messageType_t type, void* data)
{
	status_t status = STATUS_PASS;
	msg_message_t message;
	
	message.parameters = data;
	message.source = sourceModule;
	message.msgType = type;	
	message.data = NULL;
	if(messageBoxes[destModule].queue != NULL)
	{
		if(xQueueSendToBack( messageBoxes[destModule].queue,( void * ) &message,10 ) != true)
		{
			status = STATUS_FAIL;
			vTaskDelay(1);
		}
	}
	else
	{
		status = STATUS_FAIL;
	}
	return status;
}
status_t msg_sendMessageSimple(modules_t destModule, modules_t sourceModule, msg_messageType_t type, uint32_t data)
{
	status_t status = STATUS_PASS;
	msg_message_t message;
	
	message.parameters = NULL;
	message.source = sourceModule;
	message.msgType = type;
	message.data = data;
	if(messageBoxes[destModule].queue != NULL)
	{
		if(xQueueSendToBack( messageBoxes[destModule].queue,( void * ) &message,10 ) != true)
		{
			status = STATUS_FAIL;
			vTaskDelay(1);
		}
	}
	else
	{
		status = STATUS_FAIL;
	}
	return status;	
}

status_t msg_sendBroadcastMessageSimple(modules_t sourceModule, msg_messageType_t type, uint32_t data)
{
	status_t status = STATUS_PASS;
	msg_message_t message;
	
	message.parameters = NULL;
	message.source = sourceModule;
	message.msgType = type;
	message.data = data;
	int i =0;
	for(i=0;i<MODULE_NUMBER_OF_MODULES;i++)
	{
		//Check the mask for the module to see if the module accepts the message
		if((messageBoxes[i].messageMask & (1<<message.msgType)) > 0)
		{
			if(messageBoxes[i].queue != NULL)
			{
				if(xQueueSendToBack( messageBoxes[i].queue,( void * ) &message,10 ) != true)
				{
					vTaskDelay(1);
				}
			}
		}
	}
	return status;
}
