/*
 * net_wirelessNetwork.h
 *
 * Created: 8/5/2016 2:44:21 PM
 *  Author: sean
 */ 


#ifndef NET_WIRELESSNETWORK_H_
#define NET_WIRELESSNETWORK_H_
#include "asf.h"
#include "msg_messenger.h"
#include "common.h"
#include "common/include/nm_common.h"
#include "socket/include/socket.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "driver/source/nmbus.h"
#include "driver/include/m2m_wifi.h"

#define MAX_NUMBER_OF_SOCKETS 5

typedef enum
{
    NET_WIFI_STATE_INIT,
    NET_WIFI_STATE_DISCONNECTED,
    NET_WIFI_STATE_CONNECTED,
    NET_WIFI_STATE_CONNECTING,
    NET_WIFI_STATE_ERROR
}net_wifiState_t;

typedef enum
{
    NET_SOCKET_STATUS_CLIENT_CONNECTED,
    NET_SOCKET_STATUS_CLIENT_DISCONNECTED,
    NET_SOCKET_STATUS_SERVER_OPEN,
    NET_SOCKET_STATUS_SERVER_OPEN_FAILED    
}net_socketStatus_t;



typedef struct
{
	char ssid[33]; //ssid
	char passphrase[65];
	tenuM2mSecType securityType;
	uint8_t channel; //by default will use 255, which is the search all channels option.
}net_wirelessConfig_t;

typedef void (*socketStatusCallback_t)(SOCKET socketId, net_socketStatus_t status);
typedef void (*socketDataReceivedCallback_t)(SOCKET socketId, uint8_t* buf, uint16_t bufLength);

typedef struct
{
	SOCKET socketId;
	struct sockaddr_in endpoint;	//end point for the socket
	modules_t sourceModule;			//the owning module
	uint8_t* buffer;
	size_t bufferLength; 	
  	SOCKET clientSocketId;
    socketStatusCallback_t socketStatusCallback;
    socketDataReceivedCallback_t socketDataReceivedCallback; 
}net_socketConfig_t;



void net_wirelessNetworkTask(void *pvParameters);
status_t net_connectToNetwork(net_wirelessConfig_t* wirelessConfig);
status_t net_disconnectFromNetwork();

status_t net_createUdpSocket(net_socketConfig_t* socket, size_t bufferSize);
status_t net_sendUdpPacket(net_socketConfig_t* socket, uint8_t* packetBuf, uint32_t packetBufLength);
status_t net_createServerSocket(net_socketConfig_t* sock, size_t receiveBufSize);
status_t net_sendPacketToClientSock(net_socketConfig_t* sock, uint8_t* packetBuf, uint32_t packetBufLength);
status_t net_closeSocket(net_socketConfig_t* socket); 




#endif /* NET_WIRELESSNETWORK_H_ */