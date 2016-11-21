/*
 * cfg_configurationManager.c
 *
 * Created: 11/9/2016 1:12:40 PM
 *  Author: sean
 */ 

#include "cfg_configurationManager.h"
#include "dbg_debugManager.h"
#include "msg_messenger.h"
#include "pkt_packetParser.h"
#include "heddokoPacket.pb-c.h"
#include "net_wirelessNetwork.h"
#include "subp_subProcessor.h"


/* Global Variables */

/*	Static function forward declarations	*/

/*	Local variables	*/

/*	Extern functions	*/

/*	Extern variables	*/

/*	Function Definitions	*/

/*	Static function forward declarations	*/
static void processMessage(msg_message_t* message);
static void configSocketEventCallback(SOCKET socketId, net_socketStatus_t status);
static void configSocketReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength);
static void processProtoPacket( Heddoko__Packet* packet);
static void sendStatusPacket(net_socketConfig_t* socket);
static void sendMessageStatus(bool status);
static void sendProtoPacketToClient(Heddoko__Packet* packet, net_socketConfig_t* socket);
static status_t processRecordSettings(Heddoko__Packet* packet); 
static status_t processStartStream(Heddoko__Packet* packet); 
static status_t processStopStream(Heddoko__Packet* packet);
/*	Local variables	*/

xQueueHandle msg_queue_cfgManager = NULL;
cfg_moduleConfig_t* configSettings = NULL; 
static sys_manager_systemState_t currentState = SYSTEM_STATE_INIT; 
static subp_recordingConfig_t receivedRecordingConfig; 
static subp_streamConfig_t receivedStreamConfig; 
net_socketConfig_t configServer =
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
pkt_rawPacket_t receivedPacket;
static Heddoko__Packet statusPacket;
	
/*	Extern functions	*/

/*	Extern variables	*/

/*	Function definitions	*/
void cfg_configurationTask(void *pvParameters)
{
    msg_message_t receivedMessage;
    configSettings = (cfg_moduleConfig_t*)pvParameters; 
    msg_queue_cfgManager = xQueueCreate(10, sizeof(msg_message_t));
    if (msg_queue_cfgManager != NULL)
    {
        msg_registerForMessages(MODULE_CONFIG_MANAGER, 0xff, msg_queue_cfgManager);
    }
    //configure the listenning port
    configServer.endpoint.sin_port = _htons(configSettings->configPort); 
    //initialize the status packet
    heddoko__packet__init(&statusPacket);
    statusPacket.type = HEDDOKO__PACKET_TYPE__StatusResponse;
    statusPacket.serialnumber = configSettings->serialNumber;    
    while (1)
    {
        //receive from message queue and data queue, take necessary actions
        if (xQueueReceive(msg_queue_cfgManager, &receivedMessage, 20) == TRUE)
        {
            processMessage(&receivedMessage);
        }       
    }
}
static void configSocketEventCallback(SOCKET socketId, net_socketStatus_t status)
{
    switch (status)
    {
        case NET_SOCKET_STATUS_SERVER_OPEN:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Config Server Open!\r\n");
        break;
        case NET_SOCKET_STATUS_SERVER_OPEN_FAILED:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Config Server Open Failed!\r\n");
        //close the debug server socket
        net_closeSocket(&configServer);
        break;
        case NET_SOCKET_STATUS_CLIENT_CONNECTED:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Client Connected!\r\n");
        break;
        case NET_SOCKET_STATUS_CLIENT_DISCONNECTED:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Client Disconnected!\r\n");
        break;
    }
}
static void configSocketReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength)
{
    //process the protobuf packet. 
    int i = 0;
    status_t status = STATUS_FAIL;
    Heddoko__Packet* receivedProtoPacket; 	
    for(i=0;i<bufLength;i++)
    {
        if(pkt_processIncomingByte(&receivedPacket, buf[i]) == STATUS_PASS)
        {
            //the raw packet was successfully parsed. 
            status = STATUS_PASS;
            break; 
        }    
    }
    if(status == STATUS_PASS)
    {
        //check that the packet is a protobuf packet
        if(receivedPacket.payload[0] == 0x04)
        {
            receivedProtoPacket = heddoko__packet__unpack(NULL,receivedPacket.payloadSize-1,receivedPacket.payload+1);
            //check that the protopacket was deserialized properly
            if(receivedProtoPacket != NULL)
            {
                processProtoPacket(receivedProtoPacket);
                //clean the memory
                heddoko__packet__free_unpacked(receivedProtoPacket,NULL); 	
            }
            else
            {
                //failed to deserialize the packet, send a failed message response
                
            }
        }
    }    
}
static void processMessage(msg_message_t* message)
{
    switch(message->type)
    {
        case MSG_TYPE_ENTERING_NEW_STATE:
        {
            //update the status packet with the current state. 
            currentState = (sys_manager_systemState_t)message->data;
            statusPacket.has_brainpackstate = true; 
            if(currentState == SYSTEM_STATE_IDLE)
            {
                statusPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Idle;      
            }
            else if(currentState == SYSTEM_STATE_RECORDING)
            {
                statusPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Recording;      
            }
            else if(currentState == SYSTEM_STATE_STREAMING)
            {
                statusPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Streaming;      
            }
            else if(currentState == SYSTEM_STATE_ERROR)
            {
                statusPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Error;      
            }
            else //default to initializing
            {
                statusPacket.brainpackstate = HEDDOKO__BRAINPACK_STATE__Initializing;      
            }  
        }            
        break;
        case MSG_TYPE_ERROR:
        break;
        case MSG_TYPE_SDCARD_STATE:
        break;
		case MSG_TYPE_SUBP_STATUS:
        {
		    subp_status_t* subpReceivedStatus = (subp_status_t*)message->parameters;		
            //update the status packet
            statusPacket.has_chargestate = true;
            statusPacket.chargestate = (Heddoko__ChargeState)subpReceivedStatus->chargerState;
            statusPacket.has_batterylevel = true;
            statusPacket.batterylevel = subpReceivedStatus->chargeLevel; 
            statusPacket.has_sensormask = true; 
            statusPacket.sensormask = subpReceivedStatus->sensorMask; 
        }                        
		break;        
        case MSG_TYPE_WIFI_STATE:
 			if(message->data == NET_WIFI_STATE_CONNECTED)
 			{
     			if(net_createServerSocket(&configServer, 512) == STATUS_PASS)
     			{
         			dbg_printf(DBG_LOG_LEVEL_DEBUG,"Initializing config server socket\r\n");
     			}
     			else
     			{
         			dbg_printf(DBG_LOG_LEVEL_DEBUG,"Failed to initialize config server socket\r\n");
     			}
 			}
 			else if(message->data == NET_WIFI_STATE_DISCONNECTED)
 			{
     			net_closeSocket(&configServer);
 			}       
        
        break;
        default:
        break;
        
    }
}

static void processProtoPacket( Heddoko__Packet* packet)
{
    switch(packet->type)
    {
        case HEDDOKO__PACKET_TYPE__StatusRequest:
        {
            sendStatusPacket(&configServer);            
        }        
        break;
        case HEDDOKO__PACKET_TYPE__ConfigureRecordingSettings:
        {
           if(processRecordSettings(packet) == STATUS_PASS)
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
            if(processStartStream(packet) == STATUS_PASS)
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
            if(processStopStream(packet) == STATUS_PASS)
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



static void sendStatusPacket(net_socketConfig_t* socket)
{
	static char firmwareVersion[] = VERSION;

	//TODO Fill this information with the real stuff
	statusPacket.type = HEDDOKO__PACKET_TYPE__StatusResponse;
	statusPacket.firmwareversion = firmwareVersion;  
    statusPacket.serialnumber = configSettings->serialNumber;
    sendProtoPacketToClient(&statusPacket, socket);    
}

static void sendMessageStatus(bool status)
{
    Heddoko__Packet packet;
    heddoko__packet__init(&packet);
    packet.type = HEDDOKO__PACKET_TYPE__MessageStatus;
    packet.has_messagestatus = true; 
    packet.messagestatus = status; 
    sendProtoPacketToClient(&packet,&configServer);        
}    

#define MAX_ENCODED_PACKET_SIZE 255
static void sendProtoPacketToClient(Heddoko__Packet* packet, net_socketConfig_t* socket)
{
    static uint8_t serializedPacket[MAX_ENCODED_PACKET_SIZE];
    static uint8_t encodedPacket[MAX_ENCODED_PACKET_SIZE];
 	serializedPacket[0] = PACKET_TYPE_PROTO_BUF;
    size_t packetLength = heddoko__packet__pack(packet, serializedPacket+1); //increment packet pointer by one for packet type
 	packetLength += 1; //increment length to account for the packet type.
 	uint16_t encodedLength = 0;
 	//encode the serialized packet
 	pkt_serializeRawPacket(encodedPacket, MAX_ENCODED_PACKET_SIZE, &encodedLength,
 	serializedPacket, packetLength);
 	net_sendPacketToClientSock(socket,encodedPacket, encodedLength, false);   
}

static status_t processRecordSettings(Heddoko__Packet* packet)
{
    status_t status = STATUS_PASS;
    if(packet->has_recordingrate && (packet->recordingfilename != NULL) && packet->has_sensormask)
    {
        receivedRecordingConfig.rate = packet->recordingrate; 
        receivedRecordingConfig.sensorMask = packet->sensormask; 
        strncpy(receivedRecordingConfig.filename, packet->recordingfilename,SUBP_RECORDING_FILENAME_MAX_LENGTH); 
        msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_DEBUG, MSG_TYPE_RECORDING_CONFIG,&receivedRecordingConfig);
        msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_RECORDING_CONFIG,&receivedRecordingConfig);        
    }
    else
    {
        //the packet doesn't contain what we need, return fail
        status = STATUS_FAIL; 
    }
    return status; 
}


static status_t processStartStream(Heddoko__Packet* packet)
{
    status_t status = STATUS_FAIL;
    int ip[4] = {0,0,0,0};
    //to start a stream the brainpack must be in idle state
    if(currentState != SYSTEM_STATE_IDLE)
    {
        return STATUS_FAIL;    
    }        
    if(packet->has_recordingrate && (packet->recordingfilename != NULL) && packet->has_sensormask && (packet->endpoint != NULL))
    {
        receivedRecordingConfig.rate = packet->recordingrate;
        receivedRecordingConfig.sensorMask = packet->sensormask;
        strncpy(receivedRecordingConfig.filename, packet->recordingfilename,SUBP_RECORDING_FILENAME_MAX_LENGTH);
        if(packet->endpoint->address != NULL)
        {                
            //right now the command only accepts IPV4 addresses, in the future we will have to resolve hosts. 
            int ret = sscanf(packet->endpoint->address, "%d.%d.%d.%d",ip,ip+1,ip+2,ip+3);
            //the ret value should be 5 if the information was correctly parsed.
            if(ret == 4)
            {                    
                receivedStreamConfig.ipaddress.s_addr = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0] ;
                receivedStreamConfig.streamPort = (uint16_t)packet->endpoint->port; 
                status = STATUS_PASS;                
            }                    
        }
    }
    
    if(status == STATUS_PASS)
    {
         //send the recording configuration command
         msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_CONFIG_MANAGER, MSG_TYPE_RECORDING_CONFIG,&receivedRecordingConfig);
         msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_RECORDING_CONFIG,&receivedRecordingConfig);         
         //send the stream configuration command
         msg_sendMessage(MODULE_SUB_PROCESSOR, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_CONFIG,&receivedStreamConfig);
         msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_CONFIG,&receivedStreamConfig);          
         //send start stream request. 
         msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_REQUEST,1); //1= start stream      
    }        
     
    return status;
}

static status_t processStopStream(Heddoko__Packet* packet)
{
    status_t status = STATUS_PASS;
    if(currentState != SYSTEM_STATE_STREAMING)
    {
        return STATUS_FAIL;
    }         
    //send stop stream request.
    msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_CONFIG_MANAGER, MSG_TYPE_STREAM_REQUEST,0); //0 = stop stream
    return status;
}