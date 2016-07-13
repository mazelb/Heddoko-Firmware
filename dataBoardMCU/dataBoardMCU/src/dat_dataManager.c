/*
 * dat_dataManager.c
 *
 * Created: 2016-07-07 2:27:03 PM
 *  Author: Hriday Mehta
 */ 

#include "dat_dataManager.h"
#include "msg_messenger.h"
#include "drv_uart.h"

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void systemStateChangeAction(uint32_t broadcastData);			//Take necessary action to the system state change

/*	Local variables	*/
xQueueHandle msg_queue_dataHandler = NULL;
/*	Extern functions	*/

/*	Extern variables	*/

/*	Function definitions	*/
void dat_dataManagerTask(void *pvParameters)
{
	msg_message_t receivedMessage;
	
	msg_queue_dataHandler = xQueueCreate(10, sizeof(msg_message_t));
	if (msg_queue_dataHandler != NULL)
	{
		msg_registerForMessages(MODULE_DATA_MANAGER, 0xff, msg_queue_dataHandler);		// We will receive packets in the message queue itself.
	}
	
	while (1)
	{
		// receive from message queue and data queue, take necessary actions
		
		if (xQueueReceive(msg_queue_dataHandler, &receivedMessage, 1) == TRUE)
		{
			processMessage(receivedMessage);
		}

		
	}
}

static void processMessage(msg_message_t message)
{
	switch(message.type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
			systemStateChangeAction(message.broadcastData);
		break;
		case MSG_TYPE_READY:
		break;
		case MSG_TYPE_ERROR:
		break;
		case MSG_TYPE_SDCARD_STATE:
		break;
		case MSG_TYPE_SDCARD_SETTINGS:
		break;
		case MSG_TYPE_WIFI_STATE:
		break;
		case MSG_TYPE_WIFI_SETTINGS:
		break;
		case MSG_TYPE_USB_CONNECTED:
		break;
		case MSG_TYPE_CHARGER_EVENT:
		break;
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		break;
		default:
		break;
		
	}
}

static void systemStateChangeAction(uint32_t broadcastData)
{
	switch (broadcastData)
	{
		case SYSTEM_STATE_IDLE:
		break;
	}
}