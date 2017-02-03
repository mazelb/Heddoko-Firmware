/**
* @file cfg_configurationManager.c
* @brief This file contains all code dealing with the handling of configuration
* protocol buffer packets. This module runs a task that is directly responsible 
* for opening the configuration socket when the brainpack is connected to wifi. 
* The interface to and from the module is ideally through the intra-module messenger.
* @author Sean Cloghesy (sean@heddoko.com)
* @date November 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/

#include "cfg_configurationManager.h"
#include "dbg_debugManager.h"
#include "msg_messenger.h"
#include "pkt_packetParser.h"
#include "heddokoPacket.pb-c.h"
#include "net_wirelessNetwork.h"
#include "subp_subProcessor.h"

/* Local Defines */
#define MAX_ENCODED_PACKET_SIZE 255
/* Global Variables */

/*	Static function forward declarations	*/
static void processMessage(msg_message_t* message);
static void configSocketEventCallback(SOCKET socketId, net_socketStatus_t status);
static void configSocketReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength);
static void processProtoPacket( Heddoko__Packet* packet);
static void sendStatusPacket();
static void sendMessageStatus(bool status);
static void sendProtoPacketToClient(Heddoko__Packet* packet, net_socketConfig_t* socket);
static status_t processRecordSettings(Heddoko__Packet* packet); 
static status_t processStartStream(Heddoko__Packet* packet); 
static status_t processStopStream(Heddoko__Packet* packet);
/*	Local variables	*/
static xQueueHandle sgQueue_cfgManager = NULL;
static cfg_moduleConfig_t* sgpConfigSettings = NULL; 
static sys_manager_systemState_t sgCurrentState = SYSTEM_STATE_INIT; 
static subp_recordingConfig_t sgReceivedRecordingConfig; 
static subp_streamConfig_t sgReceivedStreamConfig; 
static net_socketConfig_t sgConfigServerSocket =
{
    .endpoint.sin_addr = 0, //irrelevant for the server
    .endpoint.sin_family = AF_INET,
    .endpoint.sin_port = _htons(6665),
    .sourceModule = MODULE_CONFIG_MANAGER,
    .socketStatusCallback = configSocketEventCallback,
    .socketDataReceivedCallback = configSocketReceivedDataCallback,
    .socketId = -1,
    .clientSocketId = -1    
};
static pkt_rawPacket_t sgReceivedPacket;
static Heddoko__Packet sgStatusProtoPacket;
	
/*	Extern functions	*/

/*	Extern variables	*/

/*	Function definitions	*/

/**
* @brief The configuration manager task. Is executed by the free rtos
* main task and loops while the brainpack is functional.
* Processes messages from the other free rtos tasks. 
* @param pvParameters, pointer to the initialization variables for the task
* @return void
*/
void cfg_configurationTask(void *pvParameters)
{
    msg_message_t vReceivedMessage;
    //cast the module configuration to the initial values. 
    sgpConfigSettings = (cfg_moduleConfig_t*)pvParameters; 
    sgQueue_cfgManager = xQueueCreate(10, sizeof(msg_message_t));
    if (sgQueue_cfgManager != NULL)
    {
        msg_registerForMessages(MODULE_CONFIG_MANAGER, 0xff, sgQueue_cfgManager);
    }
    //configure the listening port
    sgConfigServerSocket.endpoint.sin_port = _htons(sgpConfigSettings->configPort); 
    //initialize the status packet
    heddoko__packet__init(&sgStatusProtoPacket);
    sgStatusProtoPacket.type = HEDDOKO__PACKET_TYPE__StatusResponse;
    sgStatusProtoPacket.serialnumber = sgpConfigSettings->serialNumber;    
    while (1)
    {
        //receive from message queue, take necessary actions
        if (xQueueReceive(sgQueue_cfgManager, &vReceivedMessage, 20) == TRUE)
        {
            processMessage(&vReceivedMessage);
        }       
    }
}
/**
* @brief Configuration socket event callback. Is called from the wifi callback when a 
* socket connection event occurs.
* @param vSocketId, the socket ID pertaining to the event
* @param vStatus, enumerated status code pertaining to the event
* @return void
*/
static void configSocketEventCallback(SOCKET vSocketId, net_socketStatus_t vStatus)
{
    switch (vStatus)
    {
        case NET_SOCKET_STATUS_SERVER_OPEN:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Config Server Open!\r\n");
        break;
        case NET_SOCKET_STATUS_SERVER_OPEN_FAILED:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Config Server Open Failed!\r\n");
        //close the config server socket
        net_closeSocket(&sgConfigServerSocket);
        break;
        case NET_SOCKET_STATUS_CLIENT_CONNECTED:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Client Connected!\r\n");
        break;
        case NET_SOCKET_STATUS_CLIENT_DISCONNECTED:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Client Disconnected!\r\n");
        break;
    }
}
/**
* @brief Configuration socket received data callback. Is called from the wifi callback when
* data is received on the client socket.
* @param vSocketId, the socket ID where the data came from. 
* @param vpReceivedData, pointer to the received data
* @param vReceivedLength, length of the received data
* @return void
*/
static void configSocketReceivedDataCallback(SOCKET vSocketId, uint8_t* vpReceivedData, uint16_t vReceivedLength)
{
    //process the protobuf packet. 
    int i = 0;
    status_t vStatus = STATUS_FAIL;
    Heddoko__Packet* vpReceivedProtoPacket; 	
    for(i=0;i<vReceivedLength;i++)
    {
        //TODO: This is not thread safe... Fix this. 
        if(pkt_processIncomingByte(&sgReceivedPacket, vpReceivedData[i]) == STATUS_PASS)
        {
            //the raw packet was successfully parsed. 
            vStatus = STATUS_PASS;
            break; 
        }    
    }
    if(vStatus == STATUS_PASS)
    {
        //check that the packet is a protobuf packet
        if(sgReceivedPacket.payload[0] == 0x04)
        {
            vpReceivedProtoPacket = heddoko__packet__unpack(NULL,sgReceivedPacket.payloadSize-1,sgReceivedPacket.payload+1);
            //check that the protopacket was deserialized properly
            if(vpReceivedProtoPacket != NULL)
            {
                processProtoPacket(vpReceivedProtoPacket);
                //clean the memory
                heddoko__packet__free_unpacked(vpReceivedProtoPacket,NULL); 	
            }
            else
            {
                //failed to deserialize the packet, send a failed message response
                
            }
        }
    }    
}
/**
* @brief Processes messages received from other modules. Is called from config task.
* @param vMessage, message that was received
* @return void
*/
static void processMessage(msg_message_t* vpMessage)
{
    switch(vpMessage->msgType)
    {
        case MSG_TYPE_ENTERING_NEW_STATE:
        {
            //update the status packet with the current state. 
            sgCurrentState = (sys_manager_systemState_t)vpMessage->data;
            sgStatusProtoPacket.has_brainpackstate = true; 
            if(sgCurrentState == SYSTEM_STATE_IDLE)
            {
                sgStatusProtoPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Idle;      
            }
            else if(sgCurrentState == SYSTEM_STATE_RECORDING)
            {
                sgStatusProtoPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Recording;      
            }
            else if(sgCurrentState == SYSTEM_STATE_STREAMING)
            {
                sgStatusProtoPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Streaming;      
            }
            else if(sgCurrentState == SYSTEM_STATE_ERROR)
            {
                sgStatusProtoPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Error;      
            }
            else //default to initializing
            {
                sgStatusProtoPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Initializing;      
            }  
        }            
        break;
        case MSG_TYPE_ERROR:
        break;
        case MSG_TYPE_SDCARD_STATE:
        break;
		case MSG_TYPE_SUBP_STATUS:
        {
		    subp_status_t* subpReceivedStatus = (subp_status_t*)vpMessage->parameters;		
            //update the status packet
            sgStatusProtoPacket.has_chargestate = true;
            sgStatusProtoPacket.chargestate = (Heddoko__ChargeState)subpReceivedStatus->chargerState;
            sgStatusProtoPacket.has_batterylevel = true;
            sgStatusProtoPacket.batterylevel = subpReceivedStatus->chargeLevel; 
            sgStatusProtoPacket.has_sensormask = true; 
            sgStatusProtoPacket.sensormask = subpReceivedStatus->sensorMask; 
        }                        
		break;        
        case MSG_TYPE_WIFI_STATE:
 			if(vpMessage->data == NET_WIFI_STATE_CONNECTED)
 			{
     			if(net_createServerSocket(&sgConfigServerSocket, 512) == STATUS_PASS)
     			{
         			dbg_printf(DBG_LOG_LEVEL_DEBUG,"Initializing config server socket\r\n");
     			}
     			else
     			{
         			dbg_printf(DBG_LOG_LEVEL_DEBUG,"Failed to initialize config server socket\r\n");
     			}
 			}
 			else if(vpMessage->data == NET_WIFI_STATE_DISCONNECTED)
 			{
     			net_closeSocket(&sgConfigServerSocket);
 			}       
        
        break;
        default:
        break;
        
    }
}
/**
* @brief Processes a received protobuf packet. 
* @param vProtoPacket, pointer to protocol buffer packet that was received. 
* @return void
*/
static void processProtoPacket( Heddoko__Packet* vpProtoPacket)
{
    switch(vpProtoPacket->type)
    {
        case HEDDOKO__PACKET_TYPE__StatusRequest:
        {
            sendStatusPacket();            
        }        
        break;
        case HEDDOKO__PACKET_TYPE__ConfigureRecordingSettings:
        {
           if(processRecordSettings(vpProtoPacket) == STATUS_PASS)
           {
               //message was processed successfully, send a message status of pass. 
               sendMessageStatus(true);
           }
           else
           {
               sendMessageStatus(false);
           }
        }        
        break; 
        case HEDDOKO__PACKET_TYPE__StartDataStream:
        {
            if(processStartStream(vpProtoPacket) == STATUS_PASS)
            {
                //message was processed successfully, send a message status of pass.
                sendMessageStatus(true);
            }
            else
            {
                sendMessageStatus(false);
            }
        }
        break;
        case HEDDOKO__PACKET_TYPE__StopDataStream:
        {
            if(processStopStream(vpProtoPacket) == STATUS_PASS)
            {
                //message was processed successfully, send a message status of pass.
                sendMessageStatus(true);
            }
            else
            {
                sendMessageStatus(false);
            }
        }
        break;        
        default:
        {
            // we aren't handling this message, send a failed status response. 
            sendMessageStatus(false);
        }
        break;
    }
}

/**
* @brief Sends the completed status packet to the connected client
* @return void
*/
static void sendStatusPacket()
{
	static char firmwareVersion[] = VERSION;

	//TODO Fill this information with the real stuff
	sgStatusProtoPacket.type = HEDDOKO__PACKET_TYPE__StatusResponse;
	sgStatusProtoPacket.firmwareversion = firmwareVersion;  
    sgStatusProtoPacket.serialnumber = sgpConfigSettings->serialNumber;
    sendProtoPacketToClient(&sgStatusProtoPacket, &sgConfigServerSocket);    
}
/**
* @brief Sends the completed message status packet to the connected client
* @param vStatus,boolean representing the pass fail status of the received packet. 
* @return void
*/
static void sendMessageStatus(bool vStatus)
{
    Heddoko__Packet packet;
    heddoko__packet__init(&packet);
    packet.type = HEDDOKO__PACKET_TYPE__MessageStatus;
    packet.has_messagestatus = true; 
    packet.messagestatus = vStatus; 
    sendProtoPacketToClient(&packet,&sgConfigServerSocket);        
}    

/**
* @brief Sends a protobuf packet to a connected client
* @param vpPacket,pointer to protocol buffer packet
* @param vpSocket,pointer to configuration socket structure
* @return void
*/
static void sendProtoPacketToClient(Heddoko__Packet* vpPacket, net_socketConfig_t* vpSocket)
{
    static uint8_t vaSerializedPacket[MAX_ENCODED_PACKET_SIZE];
    static uint8_t vaEncodedPacket[MAX_ENCODED_PACKET_SIZE];
 	vaSerializedPacket[0] = PACKET_TYPE_PROTO_BUF;
    size_t vPacketLength = heddoko__packet__pack(vpPacket, vaSerializedPacket+1); //increment packet pointer by one for packet type
 	vPacketLength += 1; //increment length to account for the packet type.
 	uint16_t vEncodedLength = 0;
 	//encode the serialized packet
 	pkt_serializeRawPacket(vaEncodedPacket, MAX_ENCODED_PACKET_SIZE, &vEncodedLength,
 	vaSerializedPacket, vPacketLength);
 	net_sendPacketToClientSock(vpSocket,vaEncodedPacket, vEncodedLength, false);   
}
/**
* @brief Processes a received recording settings protobuf packet
* @param vpPacket,pointer to protocol buffer packet
* @return STATUS_PASS if the packet was processed, STATUS_FAIL if there was something 
* missing from it. 
*/
static status_t processRecordSettings(Heddoko__Packet* vpPacket)
{
    status_t vStatus = STATUS_PASS;
    if(vpPacket->has_recordingrate && (vpPacket->recordingfilename != NULL) && vpPacket->has_sensormask)
    {
        sgReceivedRecordingConfig.rate = vpPacket->recordingrate; 
        sgReceivedRecordingConfig.sensorMask = vpPacket->sensormask; 
        strncpy(sgReceivedRecordingConfig.filename, vpPacket->recordingfilename,SUBP_RECORDING_FILENAME_MAX_LENGTH); 
        msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_DEBUG, MSG_TYPE_RECORDING_CONFIG,&sgReceivedRecordingConfig);
        msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_RECORDING_CONFIG,&sgReceivedRecordingConfig);        
    }
    else
    {
        //the packet doesn't contain what we need, return fail
        vStatus = STATUS_FAIL; 
    }
    return vStatus; 
}

/**
* @brief Processes a received start stream packet
* @param vpPacket,pointer to protocol buffer packet
* @return STATUS_PASS if the packet was processed, STATUS_FAIL if there was something
* missing from it, or the brainpack was in an incorrect state. 
*/
static status_t processStartStream(Heddoko__Packet* vpPacket)
{
    status_t vStatus = STATUS_FAIL;
    int vaIP[4] = {0,0,0,0};
    //to start a stream the brainpack must be in idle state
    if(sgCurrentState != SYSTEM_STATE_IDLE)
    {
        return STATUS_FAIL;    
    }        
    if(vpPacket->has_recordingrate && (vpPacket->recordingfilename != NULL) && vpPacket->has_sensormask && (vpPacket->endpoint != NULL))
    {
        sgReceivedRecordingConfig.rate = vpPacket->recordingrate;
        sgReceivedRecordingConfig.sensorMask = vpPacket->sensormask;
        strncpy(sgReceivedRecordingConfig.filename, vpPacket->recordingfilename,SUBP_RECORDING_FILENAME_MAX_LENGTH);
        if(vpPacket->endpoint->address != NULL)
        {                
            //right now the command only accepts IPV4 addresses, in the future we will have to resolve hosts. 
            int ret = sscanf(vpPacket->endpoint->address, "%d.%d.%d.%d",vaIP,vaIP+1,vaIP+2,vaIP+3);
            //the ret value should be 5 if the information was correctly parsed.
            if(ret == 4)
            {                    
                sgReceivedStreamConfig.ipaddress.s_addr = (vaIP[3] << 24) | (vaIP[2] << 16) | (vaIP[1] << 8) | vaIP[0] ;
                sgReceivedStreamConfig.streamPort = (uint16_t)vpPacket->endpoint->port; 
                vStatus = STATUS_PASS;                
            }                    
        }
    }
    
    if(vStatus == STATUS_PASS)
    {
         //send the recording configuration command
         msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_CONFIG_MANAGER, MSG_TYPE_RECORDING_CONFIG,&sgReceivedRecordingConfig);
         msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_RECORDING_CONFIG,&sgReceivedRecordingConfig);         
         //send the stream configuration command
         msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_CONFIG,&sgReceivedStreamConfig);
         msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_CONFIG,&sgReceivedStreamConfig);          
         //send start stream request. 
         msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_REQUEST,1); //1= start stream      
    }        
     
    return vStatus;
}
/**
* @brief Processes a received stop stream packet
* @param vpPacket,pointer to protocol buffer packet
* @return STATUS_PASS if the packet was processed, STATUS_FAIL if there was something
* missing from it, or the brainpack was in an incorrect state.
*/
static status_t processStopStream(Heddoko__Packet* vpPacket)
{
    status_t status = STATUS_PASS;
    if(sgCurrentState != SYSTEM_STATE_STREAMING)
    {
        return STATUS_FAIL;
    }         
    //send stop stream request.
    msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_REQUEST,0); //0 = stop stream
    return status;
}