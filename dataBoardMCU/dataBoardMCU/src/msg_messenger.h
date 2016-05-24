/*
 * msg_messenger.h
 *
 * Created: 5/24/2016 1:31:50 PM
 *  Author: sean
 */ 


#ifndef MSG_MESSENGER_H_
#define MSG_MESSENGER_H_
#include <asf.h>
typedef enum
{
	MSG_TYPE_ENTERING_NEW_STATE = 0,
	MSG_TYPE_READY,	
	MSG_TYPE_ERROR,
	MSG_TYPE_SDCARD_STATE,
	MSG_TYPE_SDCARD_SETTINGS,
	MSG_TYPE_WIFI_STATE,
	MSG_TYPE_WIFI_SETTINGS,
	MSG_TYPE_USB_CONNECTED,
	MSG_TYPE_CHARGER_EVENT,
	MSG_TYPE_COMMAND_PACKET_RECEIVED	
	
}msg_messageType_t;
typedef struct 
{
	msg_messageType_t type;
	modules_t source;
	void* parameters;	
}msg_message_t;

typedef struct  
{
	modules_t module;
	xQueueHandle queue;
	uint32_t messageMask;
}msg_messageBox_t;
/***********************************************************************************************
 * msg_registerForMessages(modules_t module, uint32_t messageMask,xQueueHandle messageQueue)
 * @brief This function is called by each of the modules on the start of their tasks. It 
 *	passes a mask to indicate which messages it has registered for.   
 * @param module: The module that's being registered, messageMask: mask of which messages the module
	is registering for, messageQueue: The intialized message queue handle 
 * @return STATUS_PASS or STATUS_FAIL
 ***********************************************************************************************/
status_t msg_registerForMessages(modules_t module, uint32_t messageMask,xQueueHandle messageQueue);
/***********************************************************************************************
 * msg_sendBroadcastMessage(msg_message_t* message)
 * @brief Sends a message to all modules that are registered for it.   
 * @param module: message: a pointer to the message that's being sent 
 * @return STATUS_PASS or STATUS_FAIL
 ***********************************************************************************************/
status_t msg_sendBroadcastMessage(msg_message_t* message);
/***********************************************************************************************
 * msg_sendMessage(modules_t module, msg_message_t* message)
 * @brief Sends a message to a specific module   
 * @param module: message: a pointer to the message that's being sent 
 * @return STATUS_PASS or STATUS_FAIL
 ***********************************************************************************************/
status_t msg_sendMessage(modules_t module, msg_message_t* message);

#endif /* MSG_MESSENGER_H_ */