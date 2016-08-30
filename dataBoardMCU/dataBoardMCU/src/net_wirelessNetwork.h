/*
 * net_wirelessNetwork.h
 *
 * Created: 8/5/2016 2:44:21 PM
 *  Author: sean
 */ 


#ifndef NET_WIRELESSNETWORK_H_
#define NET_WIRELESSNETWORK_H_
#include "asf.h"
#include "common/include/nm_common.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "driver/source/nmbus.h"
#include "driver/include/m2m_wifi.h"
typedef struct
{
	char ssid[33]; //ssid
	char passphrase[65];
	tenuM2mSecType securityType;
	uint8_t channel; //by default will use 255, which is the search all channels option.
}net_wirelessConfig_t;

typedef enum
{
	NET_WIFI_STATE_INIT,
	NET_WIFI_STATE_DISCONNECTED,
	NET_WIFI_STATE_CONNECTED,
	NET_WIFI_STATE_CONNECTING,
	NET_WIFI_STATE_ERROR
}net_wifiState_t;

typedef struct
{
	SOCKET socketId;
	struct sockaddr_in endpoint;
	
	
}net_socketConfig_t;

void net_wirelessNetworkTask(void *pvParameters);
status_t net_sendPacket(uint8_t* packetBuf, uint32_t packetBufLength);

status_t net_connectToNetwork(net_wirelessConfig_t* wirelessConfig);
status_t net_disconnectFromNetwork();


#endif /* NET_WIRELESSNETWORK_H_ */