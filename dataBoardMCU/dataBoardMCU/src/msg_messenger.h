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

/*	Message types	*/
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
	MSG_TYPE_COMMAND_PACKET_RECEIVED,			
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




status_t msg_registerForMessages(modules_t module, uint32_t messageMask,xQueueHandle messageQueue);
status_t msg_sendBroadcastMessage(msg_message_t* message);
status_t msg_sendMessage(modules_t destModule, modules_t sourceModule, msg_messageType_t type, void* data);
status_t msg_sendMessageSimple(modules_t destModule, modules_t sourceModule, msg_messageType_t type, uint32_t data);
status_t msg_sendBroadcastMessageSimple(modules_t sourceModule, msg_messageType_t type, uint32_t data);
#endif /* MSG_MESSENGER_H_ */
