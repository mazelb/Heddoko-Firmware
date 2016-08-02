/*
 * brd_dataBoardManager.c
 *
 * Created: 8/1/2016 10:37:15 AM
 *  Author: Hriday Mehta
 */ 

#include <asf.h>
#include "common.h"
#include "brd_dataBoardManager.h"
#include "pkt_packetCommandsList.h"
#include "drv_gpio.h"
#include "drv_uart.h"

#define DATA_BOARD_TERMINAL_MSG_LENGTH	200
#define DATA_BOARD_TERMINAL_MSG_FREQ	2	// this controls the size of queue to data router

/*	Static functions forward declaration	*/
static void processPacket(drv_uart_rawPacket_t *packet);

/*	Extern variables	*/
extern xQueueHandle queue_sensorData;
extern bool enableStream;

/*	Local variables	*/
drv_uart_rawPacket_t dataBoardPacket;
xQueueHandle queue_dataBoard = NULL;		// queue to pass data to the data router
xSemaphoreHandle semaphore_dataBoardUart = NULL;

static drv_uart_config_t dataBoardPortConfig =		// TODO: undefine the UART configuration in brd_board.c
{
	.mem_index = -1,		// the driver will assign the mem_index
	.p_usart = UART1,
	.packetCallback = NULL,
	.uart_options = 
	{
		.baudrate = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits = CONF_STOPBITS
	},
	.uartMode = DRV_UART_MODE_PACKET_PARSER_DMA
};

void dat_dataBoardManager(void *pvParameters)
{
	UNUSED(pvParameters);
	drv_uart_rawPacket_t sensorPacket;
	
	// initialize the UART for data board.
	drv_uart_init(&dataBoardPortConfig);
	
	queue_dataBoard = xQueueCreate(DATA_BOARD_TERMINAL_MSG_FREQ, DATA_BOARD_TERMINAL_MSG_LENGTH);
	if (queue_dataBoard == NULL)
	{
		puts("Failed to create data board terminal message queue\r\n");
	}
	
	semaphore_dataBoardUart = xSemaphoreCreateMutex();
	
	while (1)
	{
		// check for incoming packets from data board
		if (drv_uart_getPacketTimed(&dataBoardPortConfig, &dataBoardPacket, 100) == STATUS_PASS)
		{
			// process the packet
			processPacket(&dataBoardPacket);
		}
		
		vTaskDelay(100);
	}
}

void processPacket(drv_uart_rawPacket_t *packet)
{
	if (packet->payloadSize < 2)
	{
		return;	// a packet should have minimum of two bytes
	}
	
	// TODO: does this data need to be unwrapped? It should be unwrapped in the UART driver itself
	if (packet->payload[0] == PACKET_TYPE_MASTER_CONTROL)
	{
		switch (packet->payload[1])
		{
			case PACKET_COMMAND_ID_SUBP_GET_STATUS:
				// return current status of Power board
				// use semaphore to access the uart
			break;
			case PACKET_COMMAND_ID_SUBP_CONFIG:
				// set the received sub processor configuration
			break;
			case PACKET_COMMAND_ID_SUBP_STREAMING:
				// enable / disable sensor streaming
				enableStream = packet->payload[2];
			break;
			case PACKET_COMMAND_ID_SUBP_OUTPUT_FRAME:
				// simply pass this data to daughter board and USB
				// should not access the USB and daughter board UART directly, pass it on a queue and handle the data in dataRouter
				xQueueSendToBack(queue_dataBoard, &packet->payload[2], 10);
			break;
			default:
			break;
		}
	}
}