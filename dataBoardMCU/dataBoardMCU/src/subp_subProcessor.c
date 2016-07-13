/*
 * subp_subProcessor.c
 *
 * Created: 6/20/2016 10:33:32 AM
 *  Author: sean
 * 
 */ 

#include "subp_subProcessor.h"
#include "common.h"
#include "msg_messenger.h"
#include "drv_uart.h"
#include "task_SensorHandler.h"
#include "cmd_commandProcessor.h"

/*	Extern functions	*/

/*	Extern variables	*/
extern system_status_t systemStatus;

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void pkt_callBack(rawPacket_t* packet);
static void systemStateChangeAction(msg_sys_manager_t* sys_eventData);			//Take necessary action to the system state change
static void sendPacket(rawPacket_t *p_packet);

/*	Local variables	*/
static xQueueHandle msg_queue_subp = NULL;			// queue to receive interprocessor messages
static xQueueHandle queue_localPkt = NULL;			// queues to store the packets from the packet callback
static rawPacket_t localPacket = {NULL};			// structure to hold the local packets

drv_uart_config_t usart1Config =
{
	.p_usart = USART1,
	.mem_index = USART1_IDX,
	.init_as_DMA = TRUE,
	.enable_dma_interrupt = FALSE,
	.dma_bufferDepth = FIFO_BUFFER_SIZE,
	.uart_options =
	{
		.baudrate   = SENSOR_BUS_SPEED_LOW,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
	.pktConfig = 
	{
		.transmitDisable = NULL,		// no direct interfacing with the sensor
		.transmitEnable = NULL,
		.packetReceivedCallback = pkt_callBack,
		.packet =
		{
			.payload = {NULL},
			.payloadSize = NULL,
			.bytesReceived = NULL,
			.escapeFlag = NULL
		}
	}
};

/*	Function Definitions	*/
void subp_subProcessorTask(void *pvParameters)
{
	msg_message_t receivedMessage;
	
	msg_queue_subp = xQueueCreate(10, sizeof(msg_message_t));
	if (msg_queue_subp != 0)
	{
		msg_registerForMessages(MODULE_SUB_PROCESSOR, 0xff, msg_queue_subp);
	}
	
	queue_localPkt = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(rawPacket_t));
	if (queue_localPkt == NULL)
	{
		puts("Failed to create the queue to fetch packets\r");
	}
	//setup the uart used for communication. 
	drv_uart_init(&usart1Config);
	
	//send the get time command to the power board
	
	//start the main thread where we listen for packets and messages	
	while (1)
	{
		if (xQueueReceive(msg_queue_subp, &receivedMessage, 1) == true)
		{
			processMessage(receivedMessage);
		}

		// categorize the enqueued packets and send them to respective modules
		if (xQueueReceive(queue_localPkt, &localPacket, 1) == TRUE)
		{
			sendPacket(&localPacket);
		}
					
		vTaskDelay(1);		// carefully assign the delay as one packet can be as fast as 1.85ms
	}
}

static void processMessage(msg_message_t message)
{
	switch(message.type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		break;
		case MSG_TYPE_READY:
		break;
		case MSG_TYPE_ERROR:
		break;
		case MSG_TYPE_SDCARD_STATE:
		break;
		case MSG_TYPE_SDCARD_SETTINGS:
		break;
		case MSG_TYPE_WIFI_STATE:
		break;
		case MSG_TYPE_WIFI_SETTINGS:
		break;
		case MSG_TYPE_USB_CONNECTED:
		break;
		case MSG_TYPE_CHARGER_EVENT:
		break;
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		break;
		default:
		break;
		 
	}
}

static void pkt_callBack(rawPacket_t* packet)
{
	//make sure the packet has enough bytes in it
	if(packet->payloadSize < 2)
	{
		//if there's less than two bytes... its not a valid packet
		return;
	}
	
	// don't scrutinize packet, just send it to the local queue.
	if (xQueueSendToBack(queue_localPkt, packet, 1) != TRUE)
	{
		// The packet will be lost.
		puts("Failed to enqueue the packet\r");
	}
}

static void sendPacket(rawPacket_t *p_packet)
{
	
	if (p_packet->payload[0] == PACKET_TYPE_IMU_SENSOR)
	{
		if (p_packet->payload[1] == PACKET_COMMAND_ID_GET_FRAME_RESP)
		{
			//this is a data packet, send it to data handler queue
			
			//msg_sendMessage(MODULE_DATA_MANAGER, MODULE_SUB_PROCESSOR, MSG_TYPE_COMMAND_PACKET_RECEIVED, );
		}
	}
	else if (p_packet->payload[0] == PACKET_TYPE_POWER_BOARD)
	{
		// this is a power board message, route it to the System manager
		
	}
}