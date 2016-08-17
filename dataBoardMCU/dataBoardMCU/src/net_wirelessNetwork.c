/*
 * net_wirelessNetwork.c
 *
 * Created: 8/5/2016 2:44:03 PM
 *  Author: sean
 */ 
#include <asf.h>
#include <string.h>
#include "common.h"
#include "common/include/nm_common.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "driver/source/nmbus.h"
#include "driver/include/m2m_wifi.h"
#include "net_wirelessNetwork.h"
#include "dbg_debugManager.h"
#include "socket/include/socket.h"
/* Global Variables */

/*	Static function forward declarations	*/

/*	Local variables	*/

/*	Extern functions	*/

/*	Extern variables	*/

/*	Function Definitions	*/



xSemaphoreHandle semaphore_wifiAccess = NULL;
static uint8_t wifi_connected = M2M_WIFI_DISCONNECTED;
static uint8_t tcp_connected = 0;
/** TCP server socket. */
static SOCKET tcp_server_socket = -1;

/** TCP client socket. */
static SOCKET tcp_client_socket = -1;
//UDP socket
static SOCKET udp_socket = -1;
//static function forward declarations
static void wifi_cb(uint8 msg_type, void *msg_data);
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg);
uint8_t receiveBuffer[512] = {0};
	
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
	semaphore_wifiAccess = xSemaphoreCreateMutex();
	/* Initialize WINC IOs. */
	nm_bsp_init();
	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

	/* Initialize Wi-Fi driver with data and status callbacks. */
	param.pfAppWifiCb = wifi_cb;
	ret = m2m_wifi_init(&param);
	
	tstrM2MAPConfig strM2MAPConfig;
	memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
	strcpy((char *)&strM2MAPConfig.au8SSID, "FuckYou");
	strM2MAPConfig.u8ListenChannel = 6;
	strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
	strM2MAPConfig.au8DHCPServerIP[0] = 0xC0; /* 192 */
	strM2MAPConfig.au8DHCPServerIP[1] = 0xA8; /* 168 */
	strM2MAPConfig.au8DHCPServerIP[2] = 0x02; /* 2 */
	strM2MAPConfig.au8DHCPServerIP[3] = 0x01; /* 1 */
	m2m_wifi_enable_ap(&strM2MAPConfig);	
	/* Initialize socket interface. */
	socketInit();
	registerSocketCallback(socket_cb, NULL);
	
	while(1)
	{
		//check the command queue
		
		if (wifi_connected == M2M_WIFI_CONNECTED && tcp_server_socket < 0)
		{
			struct sockaddr_in addr;
			if(xSemaphoreTake(semaphore_wifiAccess, 100) == true)
			{
				/* Create TCP server socket. */
				if ((tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) //SOCK_STREAM
				{
					dbg_printString(DBG_LOG_LEVEL_ERROR,"Failed to create TCP server socket!\r\n");
					continue;
				}
				/* Initialize socket address structure and bind service. */
				addr.sin_family = AF_INET;
				addr.sin_port = _htons(6666);
				addr.sin_addr.s_addr = 0;
				bind(tcp_server_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
				/* Create socket for Tx UDP */
				if (udp_socket < 0) 
				{
					uint32 u32EnableCallbacks = 0;
					if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
					{
						dbg_printString(DBG_LOG_LEVEL_DEBUG,"Failed to create TX UDP client socket error!\r\n");
						continue;
					}
					setsockopt(udp_socket, SOL_SOCKET, SO_SET_UDP_SEND_CALLBACK, &u32EnableCallbacks , 0);
				}				
				xSemaphoreGive(semaphore_wifiAccess);						
			}			
		}			
		if(xSemaphoreTake(semaphore_wifiAccess, 100) == true)
		{
			//handle the wifi process
			m2m_wifi_handle_events(NULL);
			xSemaphoreGive(semaphore_wifiAccess);
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
	else if(wifi_connected == 1 && udp_socket > -1)
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
			dbg_printString(DBG_LOG_LEVEL_ERROR,"Wi-Fi connected.\r\n");
			m2m_wifi_request_dhcp_client();
		}
		 else if (msg_wifi_state->u8CurrState == M2M_WIFI_DISCONNECTED) 
		 {
			/* If Wi-Fi is disconnected. */
			dbg_printString(DBG_LOG_LEVEL_ERROR,"Wi-Fi disconnected!\r\n");
			wifi_connected = M2M_WIFI_DISCONNECTED;
			
			//m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID),
					//MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
		}
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
	
	if(sock == udp_socket)
	{
		if (u8Msg == SOCKET_MSG_SENDTO) 
		{
			dbg_printString(DBG_LOG_LEVEL_DEBUG,"socket_cb: sendto success!\r\n");
			//recvfrom(rx_socket, gau8SocketTestBuffer, MAIN_WIFI_M2M_BUFFER_SIZE, 0);
		}		
	}
	else
	{
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
				accept(tcp_server_socket, NULL, NULL);
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
			} else {
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
}