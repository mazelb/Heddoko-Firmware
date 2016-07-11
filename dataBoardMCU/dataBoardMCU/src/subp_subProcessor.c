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

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void pkt_callBack(rawPacket_t* packet);

/*	Local variables	*/
xQueueHandle queue_subp = NULL;
xQueueHandle queue_pkt = NULL;		// queues to store the packets
static rawPacket_t localPacket = {NULL};
//static system_states_t currentState = SYSTEM_STATE_INITIALIZATION;

//pkt_packetParserConfiguration_t packetParserConfig =
//{
	//.transmitDisable = NULL,		// no direct interfacing with the sensor
	//.transmitEnable = NULL,			
	//.packetReceivedCallback = pkt_callBack,		// call back function from the interrupt handlers
	//.packet =
	//{
		//.payload = {NULL},
		//.payloadSize = NULL,
		//.bytesReceived = NULL,
		//.escapeFlag = NULL
	//}
//};

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

/*	Extern functions	*/

/*	Extern variables	*/
extern system_status_t systemStatus;
extern xQueueHandle pkt_dataHandler;

/*	Function Definitions	*/
void subp_subProcessorTask(void *pvParameters)
{
	msg_message_t receivedMessage;
	
	queue_subp = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_subp != 0)
	{
		msg_registerForMessages(MODULE_SUB_PROCESSOR, 0xff, queue_subp);
	}
	
	queue_pkt = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(rawPacket_t));
	if (queue_pkt == NULL)
	{
		puts("Failed to create the queue to fetch packets\r");
	}
	//setup the uart used for communication. 
	drv_uart_init(&usart1Config);
	
	//send the get time command to the power board
	
	//start the main thread where we listen for packets and messages	
	while (1)
	{
		if (xQueueReceive(queue_subp, &receivedMessage, 1) == true)
		{
			processMessage(receivedMessage);
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
	
	//scrutinize packets here and direct them to particular module
	if (packet->payload[0] == PACKET_TYPE_IMU_SENSOR)
	{
		// Do the processing according to the types of messages and enqueue them
		if (packet->payload[1] == PACKET_COMMAND_ID_GET_FRAME_RESP)
		{
			if (xQueueSendToBack(pkt_dataHandler, packet, 1) != TRUE)		// send the packet to the data handler
			{
				puts("Failed to queue the packet\r");
			}
		}
	}
}