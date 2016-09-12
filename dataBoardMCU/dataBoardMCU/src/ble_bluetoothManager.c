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

/*	Local defines	*/
#define BLE_MAX_BP_STATUS_DATA_LENGTH	5

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void processRawPacket(pkt_rawPacket_t* packet);
static void sendBpStatusData();

/*	Local variables	*/
xQueueHandle queue_ble = NULL;
bool newBpStateDataAvailble = false;
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
static struct  
{
	uint8_t batteryLevel;
	uint8_t chargeState;
	uint8_t currentState;
	uint8_t wiFiConnectionState;
	uint8_t sdCardState;
}bpStatus;

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
		msg_registerForMessages(MODULE_SUB_PROCESSOR, 0xff, queue_ble);
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
		vTaskDelay(3);		// carefully assign the delay as one packet can be as fast as 1.85ms
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

			default:
			break;
		}
	}
	dbg_printString(DBG_LOG_LEVEL_DEBUG,"Received a packet!!!!");
}

static void processMessage(msg_message_t message)
{
	switch(message.type)
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
		default:
		break;
		
	}
}

static void sendBpStatusData()
{
	uint8_t outputData[BLE_MAX_BP_STATUS_DATA_LENGTH + 2] = {0};	// frame has a two byte header
	if (newBpStateDataAvailble)
	{
		newBpStateDataAvailble = false;
		
		outputData[0] = PACKET_TYPE_MASTER_CONTROL;
		outputData[1] = PACKET_COMMAND_ID_BP_STATUS;
		memcpy(&outputData[2], &bpStatus, BLE_MAX_BP_STATUS_DATA_LENGTH);
		
		pkt_sendRawPacket(&usart1Config, outputData, (BLE_MAX_BP_STATUS_DATA_LENGTH + 2));
	}
}