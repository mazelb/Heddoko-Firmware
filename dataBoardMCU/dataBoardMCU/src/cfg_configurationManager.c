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
/*	Local variables	*/

xQueueHandle msg_queue_cfgManager = NULL;
cfg_moduleConfig_t* configSettings = NULL; 
static sys_manager_systemState_t currentState = SYSTEM_STATE_INIT; 
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
