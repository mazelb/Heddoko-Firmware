/**
* @file msg_messenger.c
* @brief Interprocess messenger module, routes messages to each module.
* @author Sean Cloghesy (sean@heddoko.com)
* @date May 2016
* Copyright Heddoko(TM) 2015, all rights reserved
*/

#ifndef MSG_MESSENGER_H_
#define MSG_MESSENGER_H_

#include <asf.h>
#include "common.h"
#include "sys_systemManager.h"

typedef enum
{
	MSG_TYPE_ENTERING_NEW_STATE = 0, //sent before a change state event
	MSG_TYPE_REQUEST_STATE,			 //sent when a module wishes to change the state of the brainpack
	MSG_TYPE_READY,					 //sent after a manager deems itself ready for the state change
	MSG_TYPE_STATE,					 //sent when the actual change takes place. 
	MSG_TYPE_ERROR,
	MSG_TYPE_SDCARD_STATE,
	MSG_TYPE_WIFI_STATE,	//sent from the net manager when the wifi connection state changes 
	MSG_TYPE_USB_CONNECTED,
	MSG_TYPE_CHARGER_EVENT,
	MSG_TYPE_COMMAND_PACKET_RECEIVED,			//TODO: should we have a separate type for the commands to the power board
	MSG_TYPE_SUBP_POWER_DOWN_REQ,
	MSG_TYPE_SUBP_POWER_DOWN_READY,
	MSG_TYPE_SUBP_STATUS,
	MSG_TYPE_GPM_BUTTON_EVENT, 
    MSG_TYPE_STREAM_CONFIG, 
    MSG_TYPE_RECORDING_CONFIG,
    MSG_TYPE_WIFI_CONFIG,
    MSG_TYPE_WIFI_CONTROL, //sent to connect and disconnect from a configured wifi network. 
    MSG_TYPE_SET_SERIAL,
    MSG_TYPE_SAVE_SETTINGS,
    MSG_TYPE_STREAM_REQUEST,
    MSG_TYPE_TFTP_INITIATE_TRANSFER,
    MSG_TYPE_TFTP_PACKET_RECEIVED,
    MSG_TYPE_TFTP_TRANSFER_RESULT,
    MSG_TYPE_TFTP_PACKET_TIMEOUT, 
    MSG_TYPE_DEBUG_BLE, 
    MSG_TYPE_TOGGLE_RECORDING, 
    MSG_TYPE_FW_UPDATE_RESTART_REQUEST,
    MSG_TYPE_FW_UPDATE_START_PB_UPDATE
    
}msg_messageType_t;

typedef struct 
{
	msg_messageType_t msgType;
	modules_t source;
	void* parameters;
	uint32_t data;	
}msg_message_t;

typedef struct  
{
	modules_t module;
	xQueueHandle queue;
	uint32_t messageMask;
}msg_messageBox_t;

/*	Message types	*/

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
 * msg_sendMessage(modules_t destModule, modules_t sourceModule, msg_messageType_t type, void* data)
 * @brief Sends a message to a specific module
 * @param destmodule: the module where the message is being directed
 * @param sourceModule: the module where the message is from
 * @param type: the message type (from enumeration)
 * @param data: a void pointer to the payload of the message
 * @return STATUS_PASS or STATUS_FAIL
 ***********************************************************************************************/
status_t msg_sendMessage(modules_t destModule, modules_t sourceModule, msg_messageType_t type, void* data);
/***********************************************************************************************
 * msg_sendMessageSimple(modules_t destModule, modules_t sourceModule, msg_messageType_t type, uint32_t data)
 * @brief Sends a message to a specific module   
 * @param destmodule: the module where the message is being directed
 * @param sourceModule: the module where the message is from
 * @param type: the message type (from enumeration)
 * @param data: a 32 bit integer representing the payload of the message. (casted enum)
 * @return STATUS_PASS or STATUS_FAIL
 ***********************************************************************************************/
status_t msg_sendMessageSimple(modules_t destModule, modules_t sourceModule, msg_messageType_t type, uint32_t data);
/***********************************************************************************************
 * msg_sendBroadcastMessageSimple(modules_t sourceModule, msg_messageType_t type, uint32_t data)
 * @brief Sends a simple message to all modules 
 * @param sourceModule: the module where the message is from
 * @param type: the message type (from enumeration)
 * @param data: a 32 bit integer representing the payload of the message. (casted enum)
 * @return STATUS_PASS or STATUS_FAIL
 ***********************************************************************************************/
status_t msg_sendBroadcastMessageSimple(modules_t sourceModule, msg_messageType_t type, uint32_t data);
#endif /* MSG_MESSENGER_H_ */
