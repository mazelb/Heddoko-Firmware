/*
 * ble_bluetoothManager.c
 *
 * Created: 8/17/2016 4:40:43 PM
 *  Author: sean
 */ 
#include "ble_bluetoothManager.h"
#include "common.h"
#include "drv_gpio.h"
#include "msg_messenger.h"
#include "pkt_packetParser.h"
#include "dbg_debugManager.h"
#include "pkt_packetCommandsList.h"
#include "subp_subProcessor.h"

/*	Local defines	*/
#define BLE_BP_STATUS_DATA_SIZE	5
#define BLE_MAX_RAW_DATA_SIZE	20
#define SSID_DATA_SIZE			32
#define PASSPHRASE_DATA_SIZE	64
#define BLE_WIFI_DATA_SIZE		(SSID_DATA_SIZE + PASSPHRASE_DATA_SIZE + 1)	// one byte for security type enum
#define PACKET_HEADER_SIZE		2

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void processRawPacket(pkt_rawPacket_t* packet);
static void sendBpStatusData();

/*	Local variables	*/
xQueueHandle queue_ble = NULL;
bool newBpStateDataAvailble = false;
subp_status_t *subProcessorStatusData;
uint8_t vRawData[BLE_MAX_RAW_DATA_SIZE];
drv_uart_config_t usart1Config =
{
	.p_usart = USART1,
	.mem_index = 0,
	.uart_options =
	{
		.baudrate   = 115200,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
	.mode = DRV_UART_MODE_INTERRUPT
};
struct bpStatus_t
{
	uint8_t batteryLevel;
	uint8_t chargeState;
	uint8_t currentState;
	uint8_t wiFiConnectionState;
	uint8_t sdCardState;
}bpStatus;	// status to be passed to BLE module.

struct wifi_data_t
{
    uint8_t ssid[SSID_DATA_SIZE];
    uint8_t passphrase[PASSPHRASE_DATA_SIZE];
    uint8_t securityType;
}wifi_data;	// local config to store wifi data for BLE module.


/*		Function definitions	*/
void ble_bluetoothManagerTask(void *pvParameters)
{
	msg_message_t receivedMessage;
	pkt_rawPacket_t rawPacket =
	{
		.bytesReceived = 0,
		.escapeFlag = 0,
		.payloadSize = 0
	};
	
	queue_ble = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_ble != 0)
	{
		msg_registerForMessages(MODULE_BLE, 0xff, queue_ble);
	}
	//initialize the UART packet receiver
	if(drv_uart_init(&usart1Config) != STATUS_PASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"failed to open USART1 for ble\r\n");
	}
	drv_gpio_setPinState(DRV_GPIO_PIN_BLE_RST, DRV_GPIO_PIN_STATE_HIGH);
	
	//send the get time command to the power board
	
	//start the main thread where we listen for packets and messages
	while (1)
	{
		if (xQueueReceive(queue_ble, &receivedMessage, 1) == true)
		{
			processMessage(receivedMessage);
		}
		if(pkt_getPacketTimed(&usart1Config,&rawPacket,10) == STATUS_PASS)
		{
			//we have a full packet
			processRawPacket(&rawPacket);
		}
		
		if (newBpStateDataAvailble)
		{
			sendBpStatusData();
			newBpStateDataAvailble = false;
		}
		
		vTaskDelay(10);
	}
}

static void processRawPacket(pkt_rawPacket_t* packet)
{

	//check which type of packet it is.
	//All the packets should be of this type... we're not on a 485 bus.
	int i =0;
	int result = 0;
	if(packet->payload[0] == PACKET_TYPE_BLE_MODULE)
	{
		switch(packet->payload[1])
		{
			case PACKET_COMMAND_ID_GPS_DATA_RESP:
				
			break;
			
			case PACKET_COMMAND_ID_ALL_WIFI_DATA_RESP:
				memcpy(wifi_data.ssid, (uint8_t *) &packet->payload[2], SSID_DATA_SIZE);    
				memcpy(wifi_data.passphrase, (uint8_t *) &packet->payload[34], PASSPHRASE_DATA_SIZE);
				wifi_data.securityType = packet->payload[98];
			break;
			
			case PACKET_COMMAND_ID_SEND_RAW_DATA_TO_MASTER:	// received as notification every time new data is written
			break;
			
			case PACKET_COMMAND_ID_GET_RAW_DATA_RESP:	// received when the master polls for data
			break;
			
			default:
			break;
		}
	}
	dbg_printf(DBG_LOG_LEVEL_DEBUG,"Received a packet!!!!");
}

static void processMessage(msg_message_t message)
{
	switch(message.msgType)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		{
			if (bpStatus.currentState != message.data)
			{
				bpStatus.currentState = message.data;
				newBpStateDataAvailble = true;
			}
		}
		break;
		case MSG_TYPE_ERROR:
			
		break;
		case MSG_TYPE_SDCARD_STATE:
		{
			if (bpStatus.sdCardState != message.data)
			{
				bpStatus.sdCardState = message.data;
				newBpStateDataAvailble = true;
			}
		}
		break;
		case MSG_TYPE_WIFI_STATE:
		{
			if (bpStatus.wiFiConnectionState != message.data)
			{
				bpStatus.wiFiConnectionState = message.data;
				newBpStateDataAvailble = true;
			}
		}
		break;
		case MSG_TYPE_SUBP_STATUS:
		{
			subProcessorStatusData = message.parameters;
			if ((bpStatus.batteryLevel != subProcessorStatusData->chargeLevel) || (bpStatus.chargeState != subProcessorStatusData->chargerState))
			{
				bpStatus.batteryLevel =	subProcessorStatusData->chargeLevel;
				bpStatus.chargeState = subProcessorStatusData->chargerState;
				newBpStateDataAvailble = true;
			}
		}
		break;
		default:
		break;
		
	}
}

static void sendBpStatusData()
{
	uint8_t outputData[BLE_BP_STATUS_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header
		
	outputData[0] = PACKET_TYPE_MASTER_CONTROL;
	outputData[1] = PACKET_COMMAND_ID_BP_STATUS;
	memcpy(&outputData[2], &bpStatus, BLE_BP_STATUS_DATA_SIZE);
		
	pkt_sendRawPacket(&usart1Config, outputData, (BLE_BP_STATUS_DATA_SIZE + PACKET_HEADER_SIZE));
}

void ble_startFastAdv()
{
	uint8_t outputData[2] = {PACKET_TYPE_MASTER_CONTROL, PACKET_COMMAND_ID_START_FAST_ADV};
	pkt_sendRawPacket(&usart1Config, outputData, sizeof(outputData));
}

void ble_sendWiFiConfig(net_wirelessConfig_t *wiFiConfig)
{
	uint8_t outputData[BLE_WIFI_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header
	
	outputData[0] = PACKET_TYPE_MASTER_CONTROL;
	outputData[1] = PACKET_COMMAND_ID_DEFAULT_WIFI_DATA;
	
	memset(&wifi_data.ssid, NULL, SSID_DATA_SIZE);
	strncpy(&wifi_data.ssid, &wiFiConfig->ssid, SSID_DATA_SIZE);
	memset(&wifi_data.passphrase, NULL, PASSPHRASE_DATA_SIZE);
	strncpy(&wifi_data.passphrase, &wiFiConfig->ssid, PASSPHRASE_DATA_SIZE);
	wifi_data.securityType = wiFiConfig->securityType;
	
	memcpy(&outputData[2], (uint8_t *) &wifi_data, BLE_WIFI_DATA_SIZE);
	pkt_sendRawPacket(&usart1Config, outputData, (BLE_WIFI_DATA_SIZE + PACKET_HEADER_SIZE));
}

void ble_sendRawData(uint8_t *data, uint8_t size)
{
	uint8_t outputData[BLE_MAX_RAW_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header
	
	outputData[0] = PACKET_TYPE_MASTER_CONTROL;
	outputData[1] = PACKET_COMMAND_ID_SEND_RAW_DATA_TO_BLE;
	memcpy(&outputData[2], data, (size > BLE_MAX_RAW_DATA_SIZE ? BLE_MAX_RAW_DATA_SIZE : size));
	
	pkt_sendRawPacket(&usart1Config, outputData, (BLE_MAX_RAW_DATA_SIZE + PACKET_HEADER_SIZE));
}

void ble_wifiDataReq()
{
	uint8_t outputData[2] = {PACKET_TYPE_MASTER_CONTROL, PACKET_COMMAND_ID_ALL_WIFI_DATA_REQ};
	pkt_sendRawPacket(&usart1Config, outputData, sizeof(outputData));
}

void ble_rawDataReq()
{
	uint8_t outputData[2] = {PACKET_TYPE_MASTER_CONTROL, PACKET_COMMAND_ID_GET_RAW_DATA_REQ};
	pkt_sendRawPacket(&usart1Config, outputData, sizeof(outputData));
}