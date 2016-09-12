/*
 * net_wirelessNetwork.c
 *
 * Created: 8/5/2016 2:44:03 PM
 *  Author: sean
 */ 
#include <asf.h>
#include <string.h>

#include "net_wirelessNetwork.h"
#include "dbg_debugManager.h"
#include "heddokoPacket.pb-c.h"
#include "pkt_packetParser.h"

/* Global Variables */

/*	Static function forward declarations	*/
static void wifi_cb(uint8 msg_type, void *msg_data);
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg);
static void processEvent(msg_message_t* message);
static status_t registerSocket(net_socketConfig_t* socket);
static status_t deRegisterSocket(net_socketConfig_t* socket);
static status_t sendAdvertisingPacket(net_socketConfig_t* advertisingSocket);
/*	Local variables	*/
xSemaphoreHandle semaphore_wifiAccess = NULL;
xQueueHandle queue_netManager = NULL;
net_socketConfig_t* registeredSockets[MAX_NUMBER_OF_SOCKETS] = {0};
//advertising socket lives here. 
net_socketConfig_t advertisingSocket =
{
	.endpoint.sin_addr = 0xFFFFFFFF, //broadcast Address
	.endpoint.sin_family = AF_INET,
	.endpoint.sin_port = _htons(6668),
	.sourceModule = MODULE_WIFI
};
	
/*	Extern functions	*/

/*	Extern variables	*/

/*	Function Definitions	*/




static uint8_t wifi_connected = M2M_WIFI_DISCONNECTED;
static uint8_t tcp_connected = 0;
/** TCP server socket. */
static SOCKET tcp_server_socket = -1;

/** TCP client socket. */
static SOCKET tcp_client_socket = -1;
//UDP socket
static SOCKET udp_socket = -1;
//static function forward declarations

uint8_t receiveBuffer[512] = {0};
	
net_wifiState_t currentWifiState = NET_WIFI_STATE_DISCONNECTED; 	
	
struct sockaddr_in udpAddr = 
{
	.sin_family = AF_INET,
	.sin_port = _htons(6667),
	.sin_addr.s_addr = 0xFFFFFFFF//0x6402A8C0 //192.168.2.100		
};	
void net_wirelessNetworkTask(void *pvParameters)
{
	tstrWifiInitParam param;
	int8_t ret;
	msg_message_t eventMessage;
	currentWifiState = NET_WIFI_STATE_INIT;
	semaphore_wifiAccess = xSemaphoreCreateMutex();
	queue_netManager = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_netManager != 0)
	{
		msg_registerForMessages(MODULE_WIFI, 0xff, queue_netManager);
	}	
	uint32_t advertisingPacketLastSentTime = xTaskGetTickCount(); 
	/* Initialize WINC IOs. */
	nm_bsp_init();
	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

	/* Initialize Wi-Fi driver with data and status callbacks. */
	param.pfAppWifiCb = wifi_cb;
	ret = m2m_wifi_init(&param);
	
	
	
	//tstrM2MAPConfig strM2MAPConfig;
	//memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
	//strcpy((char *)&strM2MAPConfig.au8SSID, "Virus");
	//strM2MAPConfig.u8ListenChannel = 6;
	//strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
	//strM2MAPConfig.au8DHCPServerIP[0] = 0xC0; /* 192 */
	//strM2MAPConfig.au8DHCPServerIP[1] = 0xA8; /* 168 */
	//strM2MAPConfig.au8DHCPServerIP[2] = 0x02; /* 2 */
	//strM2MAPConfig.au8DHCPServerIP[3] = 0x01; /* 1 */
	//m2m_wifi_enable_ap(&strM2MAPConfig);	
	/* Initialize socket interface. */
	socketInit();
	registerSocketCallback(socket_cb, NULL);
	currentWifiState = NET_WIFI_STATE_DISCONNECTED;
	
	while(1)
	{
		//check the command queue
		
		//if (wifi_connected == M2M_WIFI_CONNECTED && tcp_server_socket < 0)
		//{
			//struct sockaddr_in addr;
			//if(xSemaphoreTake(semaphore_wifiAccess, 100) == true)
			//{
				///* Create TCP server socket. */
				//if ((tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) //SOCK_STREAM
				//{
					//dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create TCP server socket!\r\n");
					//continue;
				//}
				///* Initialize socket address structure and bind service. */
				//addr.sin_family = AF_INET;
				//addr.sin_port = _htons(6666);
				//addr.sin_addr.s_addr = 0;
				//bind(tcp_server_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
				///* Create socket for Tx UDP */
				//if (udp_socket < 0) 
				//{
					//uint32 u32EnableCallbacks = 0;
					//if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
					//{
						//dbg_printString(DBG_LOG_LEVEL_DEBUG,"Failed to create TX UDP client socket error!\r\n");
						//continue;
					//}
					//setsockopt(udp_socket, SOL_SOCKET, SO_SET_UDP_SEND_CALLBACK, &u32EnableCallbacks , 0);
				//}				
				//xSemaphoreGive(semaphore_wifiAccess);						
			//}			
		//}			
		if(xSemaphoreTake(semaphore_wifiAccess, 100) == true)
		{
			//handle the wifi process
			m2m_wifi_handle_events(NULL);
			xSemaphoreGive(semaphore_wifiAccess);
		}
		if(xQueueReceive(queue_netManager, &(eventMessage), 1) == true)
		{
			processEvent(&eventMessage);
		}
		
		if(currentWifiState == NET_WIFI_STATE_CONNECTED)
		{
			if(xTaskGetTickCount() > (advertisingPacketLastSentTime +5000))
			{
				advertisingPacketLastSentTime = xTaskGetTickCount();
				sendAdvertisingPacket(&advertisingSocket);
			}
		}
		
		vTaskDelay(1); 
	}
}

status_t net_sendPacket(uint8_t* packetBuf, uint32_t packetBufLength)
{
	
	if(tcp_connected == 1 && tcp_client_socket > -1)
	{	
		if(xSemaphoreTake(semaphore_wifiAccess, 100) == true)
		{
			send(tcp_client_socket, packetBuf, packetBufLength, 0);
			xSemaphoreGive(semaphore_wifiAccess);					
		}
	}
	else if(udp_socket > -1)
	{
		if(xSemaphoreTake(semaphore_wifiAccess, 100) == true)
		{
			int ret = sendto(udp_socket, packetBuf, packetBufLength, 0, (struct sockaddr *)&udpAddr, sizeof(udpAddr));
			if (ret != M2M_SUCCESS) 
			{
				dbg_printString(DBG_LOG_LEVEL_ERROR,"main: failed to send packet!\r\n");
			}		
			xSemaphoreGive(semaphore_wifiAccess);			
		}		
	}
	
}

status_t net_connectToNetwork(net_wirelessConfig_t* wirelessConfig)
{
	status_t status = STATUS_PASS;
	int8_t retVal = 0;
	if(currentWifiState != NET_WIFI_STATE_DISCONNECTED)
	{
		//The wifi module must be disconnected before another connection is made
		return STATUS_FAIL;
	}
	//set the state to connecting
	currentWifiState = NET_WIFI_STATE_CONNECTING;
	if(xSemaphoreTake(semaphore_wifiAccess, 100) == true)
	{
		retVal = m2m_wifi_connect(wirelessConfig->ssid, strlen(wirelessConfig->ssid), wirelessConfig->securityType,
			wirelessConfig->passphrase, wirelessConfig->channel);
		if(retVal != M2M_SUCCESS)
		{
			dgb_printf(DBG_LOG_LEVEL_ERROR, "Failed to execute wifi connect with error\r\n");
			currentWifiState = NET_WIFI_STATE_DISCONNECTED;
			status = STATUS_FAIL;
		}
		xSemaphoreGive(semaphore_wifiAccess);
	}
	else
	{
		status = STATUS_FAIL;
		currentWifiState = NET_WIFI_STATE_DISCONNECTED;		
	}

	return status;
}
status_t net_disconnectFromNetwork()
{
	status_t status = STATUS_PASS;
	int8_t retVal = 0;
	if(currentWifiState == NET_WIFI_STATE_CONNECTED || currentWifiState == NET_WIFI_STATE_CONNECTING)
	{
		retVal = m2m_wifi_disconnect();
		if(retVal != M2M_SUCCESS)
		{
			status = STATUS_FAIL;
		}
	}
	return status;
}

status_t net_createUdpSocket(net_socketConfig_t* sock, size_t bufferSize)
{
	status_t status = STATUS_PASS;	
	uint32_t enableCallBacks = 0;
	//make sure we're connected to the wifi network
	if(currentWifiState != NET_WIFI_STATE_CONNECTED)
	{
		return STATUS_FAIL;
	}
	if(xSemaphoreTake(semaphore_wifiAccess, 50) == true)
	{	
		//register the socket with the handler
		//this must be done first to make sure there's room in the array.
		status = registerSocket(sock);
		if(status == STATUS_PASS)
		{
			//create the socket
			if ((sock->socketId = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"Failed to create UDP client socket error!\r\n");
				status = STATUS_FAIL;
			}
			else
			{
				//set the socket options, we don't want call backs on the UDP streaming socket. 
				setsockopt(sock->socketId, SOL_SOCKET, SO_SET_UDP_SEND_CALLBACK, &enableCallBacks , 0);
				//allocate a buffer for the socket. 
				sock->buffer = malloc(bufferSize); 
				sock->bufferLength = bufferSize;
				if(sock->buffer == NULL)
				{
					status = STATUS_FAIL;
				}
			}
			if(status == STATUS_FAIL)
			{
				//remove the socket from the handler list.
				deRegisterSocket(sock);				
			}		
		}
		xSemaphoreGive(semaphore_wifiAccess);
	}
	return status;
}
status_t net_sendUdpPacket(net_socketConfig_t* sock, uint8_t* packetBuf, uint32_t packetBufLength)
{
	status_t status = STATUS_FAIL;	
	
	if(currentWifiState == NET_WIFI_STATE_CONNECTED && sock->socketId > -1)
	{
		//TODO: add validation of socket endpoint
		//don't wait very long for the wifi access, only 5ms for now. 		
		if(xSemaphoreTake(semaphore_wifiAccess, 5) == true)
		{
			int ret = sendto(sock->socketId, packetBuf, packetBufLength, 0, (struct sockaddr *)&sock->endpoint, sizeof(struct sockaddr));
			if (ret == M2M_SUCCESS)
			{
				status = STATUS_PASS;
			}
			else
			{
				dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to send packet!\r\n");
			}
			xSemaphoreGive(semaphore_wifiAccess);
		}
	}
	return status;	
}
status_t net_closeSocket(net_socketConfig_t* sock)
{
	status_t status = STATUS_PASS;	
	if(sock->socketId < 0)
	{
		return STATUS_FAIL;
	}
	if(xSemaphoreTake(semaphore_wifiAccess, 50) == true)
	{
		deRegisterSocket(sock);
		close(sock->socketId);	
		if(sock->buffer != NULL)
		{
			free(sock->buffer);
		}
		sock->socketId = -1; 	
		xSemaphoreGive(semaphore_wifiAccess);	
	}
	return status;	
}

static void processEvent(msg_message_t* message)
{
	switch(message->type)
	{
		case MSG_TYPE_WIFI_STATE:
			if(message->data == NET_WIFI_STATE_CONNECTED)
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG, "Creating advertising socket\r\n");
				net_createUdpSocket(&advertisingSocket, 255); 
			}
			else if(message->data == NET_WIFI_STATE_DISCONNECTED)
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG, "Closing advertising socket\r\n");				
				net_closeSocket(&advertisingSocket);
			}			
		break;
		default:
		break;
	}
}

static status_t registerSocket(net_socketConfig_t* newSocket)
{
	status_t status = STATUS_FAIL;
	int i = 0;
	//go through the registered socket list and look for an empty location.
	for(i=0;i<MAX_NUMBER_OF_SOCKETS;i++)
	{
		if(registeredSockets[i] == NULL)
		{
			registeredSockets[i] = newSocket;
			status = STATUS_PASS;
			//get out of the loop, we found a place for the socket.
			break;
		}
	}
	return status;
}

static status_t deRegisterSocket(net_socketConfig_t* socket)
{
	status_t status = STATUS_FAIL;
	int i = 0;
	//find the socket in the list and set it to NULL to clear it.  
	for(i=0;i<MAX_NUMBER_OF_SOCKETS;i++)
	{		
		if(registeredSockets[i] == socket)
		{
			registeredSockets[i] = NULL;
			status = STATUS_PASS;
		}		
	}
	return status;
}

static status_t sendAdvertisingPacket(net_socketConfig_t* advertisingSocket)
{
	char firmwareVersion[] = "V2.0"; 
	char serialNumber[] = "BP00001";
	uint8_t serializedPacket[255]; 	
	Heddoko__Packet advertisingProtoPacket;
	heddoko__packet__init(&advertisingProtoPacket);
	//TODO Fill this information with the real stuff that is sent on boot. 
	advertisingProtoPacket.type = HEDDOKO__PACKET_TYPE__AdvertisingPacket;
	advertisingProtoPacket.firmwareversion = firmwareVersion; 
	advertisingProtoPacket.serialnumber = serialNumber;
	advertisingProtoPacket.has_configurationport = true;
	advertisingProtoPacket.configurationport = 6666; 
	serializedPacket[0] = PACKET_TYPE_PROTO_BUF;
	size_t packetLength = heddoko__packet__pack(&advertisingProtoPacket, serializedPacket+1); //increment packet pointer by one for packet type
	packetLength += 1; //increment length to account for the packet type.
	uint16_t encodedLength = 0;
	//encode the serialized packet
	pkt_serializeRawPacket(advertisingSocket->buffer, advertisingSocket->bufferLength, &encodedLength, 
		serializedPacket, packetLength);
	net_sendUdpPacket(advertisingSocket,advertisingSocket->buffer, encodedLength); 	
}


//static function declarations

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] msg_type Type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
 *  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
 *  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
 *  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
 *  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
 *  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
 * \param[in] msg_data A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type. Existing types are:
 *  - tstrM2mWifiStateChanged
 *  - tstrM2MWPSInfo
 *  - tstrM2MP2pResp
 *  - tstrM2MAPResp
 *  - tstrM2mScanDone
 *  - tstrM2mWifiscanResult
 */
static void wifi_cb(uint8 msg_type, void *msg_data)
{
	switch (msg_type) {
	case M2M_WIFI_RESP_CON_STATE_CHANGED:
	{
		tstrM2mWifiStateChanged *msg_wifi_state = (tstrM2mWifiStateChanged *)msg_data;
		if (msg_wifi_state->u8CurrState == M2M_WIFI_CONNECTED) 
		{
			/* If Wi-Fi is connected. */
			dbg_printString(DBG_LOG_LEVEL_DEBUG,"Wi-Fi connected.\r\n");
			currentWifiState = NET_WIFI_STATE_CONNECTED;
		}
		 else if (msg_wifi_state->u8CurrState == M2M_WIFI_DISCONNECTED) 
		 {
			/* If Wi-Fi is disconnected. */
			dbg_printString(DBG_LOG_LEVEL_DEBUG,"Wi-Fi disconnected!\r\n");
			currentWifiState = NET_WIFI_STATE_DISCONNECTED;
			//m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID),
					//MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
		}
		msg_sendBroadcastMessageSimple(MODULE_WIFI, MSG_TYPE_WIFI_STATE, currentWifiState);
		
	}
	break;

	case M2M_WIFI_REQ_DHCP_CONF:
	{
		uint8 *pu8IPAddress = (uint8 *)msg_data;
		wifi_connected = M2M_WIFI_CONNECTED;
		dgb_printf(DBG_LOG_LEVEL_DEBUG,"Wi-Fi IP is %u.%u.%u.%u\r\n", pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
	}
	break;

	default:
		break;
	}
}


/**
 * \brief Callback to get the Socket event.
 *
 * \param[in] sock Socket descriptor.
 * \param[in] u8Msg Type of Socket notification. Possible types are:
 *  - [SOCKET_MSG_CONNECT](@ref SOCKET_MSG_CONNECT)
 *  - [SOCKET_MSG_BIND](@ref SOCKET_MSG_BIND)
 *  - [SOCKET_MSG_LISTEN](@ref SOCKET_MSG_LISTEN)
 *  - [SOCKET_MSG_ACCEPT](@ref SOCKET_MSG_ACCEPT)
 *  - [SOCKET_MSG_RECV](@ref SOCKET_MSG_RECV)
 *  - [SOCKET_MSG_SEND](@ref SOCKET_MSG_SEND)
 *  - [SOCKET_MSG_SENDTO](@ref SOCKET_MSG_SENDTO)
 *  - [SOCKET_MSG_RECVFROM](@ref SOCKET_MSG_RECVFROM)
 * \param[in] pvMsg A structure contains notification informations.
 */
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	
	int i =0; 
	net_socketConfig_t* socketConfig = NULL; 
	//go through all socket configurations and find which one is used for this socket.
	for(int i = 0; i < MAX_NUMBER_OF_SOCKETS; i++)
	{
		if(registeredSockets[i]->socketId == sock)
		{
			socketConfig = registeredSockets[i];	
		}		
	}

	switch (u8Msg) 
	{
		/* Socket bind. */
		case SOCKET_MSG_BIND:
		{
			tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;
			if (pstrBind && pstrBind->status == 0) 
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: bind success.\r\n");
				listen(tcp_server_socket, 0);
			} 
			else 
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: bind error!\r\n");
			}
		}
		break;

		/* Socket listen. */
		case SOCKET_MSG_LISTEN:
		{
			tstrSocketListenMsg *pstrListen = (tstrSocketListenMsg *)pvMsg;
			if (pstrListen && pstrListen->status == 0) 
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: listen success.\r\n");
				accept(sock, NULL, NULL);
			}
			else 
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: listen error!\r\n");
			}
		}
		break;

		/* Connect accept. */
		case SOCKET_MSG_ACCEPT:
		{
			tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;
			if (pstrAccept) 
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: accept success.\r\n");
				accept(tcp_server_socket, NULL, NULL);
				tcp_client_socket = pstrAccept->sock;
				tcp_connected = 1;
				recv(tcp_client_socket, receiveBuffer, sizeof(receiveBuffer), 0);
			} 
			else 
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: accept error!\r\n");
				close(tcp_server_socket);
				tcp_server_socket = -1;
				tcp_connected = 0;
			}
		}
		break;

		/* Socket connected. */
		case SOCKET_MSG_CONNECT:
		{
			tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
			if (pstrConnect && pstrConnect->s8Error >= 0) {
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: connect success.\r\n");
				tcp_connected = 1;
				recv(tcp_client_socket, receiveBuffer, sizeof(receiveBuffer), 0);
			} else {
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: connect error!\r\n");
				tcp_connected = 0;
			}
		}
		break;

		/* Message send. */
		case SOCKET_MSG_SEND:
		{
			recv(tcp_client_socket, receiveBuffer, sizeof(receiveBuffer), 0);
		}
		break;
		/* Message send to */
		case SOCKET_MSG_SENDTO:
		{
			dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: sendto success!\r\n");
		}
		break;
		/* Message receive. */
		case SOCKET_MSG_RECV:
		{
			tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
			if (pstrRecv && pstrRecv->s16BufferSize > 0) 
			{
				//if (!strncmp((char *)pstrRecv->pu8Buffer, REMOTE_CMD_INDICATOR, INDICATOR_STRING_LEN)) {
					//parse_command((char *)(pstrRecv->pu8Buffer + INDICATOR_STRING_LEN), 1);
				//} else {
					//PRINT_REMOTE_MSG(pstrRecv->pu8Buffer);
				//}
				//loop back the data!
				//send(tcp_client_socket, pstrRecv->pu8Buffer, pstrRecv->s16BufferSize, 0);
			} 
			else 
			{
				dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: recv error!\r\n");
				if (tcp_client_socket >= 0) 
				{
					dbg_printString(DBG_LOG_LEVEL_DEBUG,"Close client socket to disconnect.\r\n");
					close(tcp_client_socket);
					tcp_client_socket = -1;		
				}
				tcp_connected = 0;
				break;
			}
			recv(tcp_client_socket, receiveBuffer, sizeof(receiveBuffer), 0);
		}
		break;
		

		default:
			break;
	}
	
}