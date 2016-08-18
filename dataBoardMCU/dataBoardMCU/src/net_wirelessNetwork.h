/*
 * net_wirelessNetwork.h
 *
 * Created: 8/5/2016 2:44:21 PM
 *  Author: sean
 */ 


#ifndef NET_WIRELESSNETWORK_H_
#define NET_WIRELESSNETWORK_H_

void net_wirelessNetworkTask(void *pvParameters);
status_t net_sendPacket(uint8_t* packetBuf, uint32_t packetBufLength);

#endif /* NET_WIRELESSNETWORK_H_ */