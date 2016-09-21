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

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void processRawPacket(pkt_rawPacket_t* packet);

/*	Local variables	*/
xQueueHandle queue_ble = NULL;
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
	//initialize the uart packet receiver
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
		break;
		case MSG_TYPE_ERROR:
		break;
		case MSG_TYPE_SDCARD_STATE:
		break;
		case MSG_TYPE_WIFI_STATE:
		break;
		default:
		break;
		
	}
}
