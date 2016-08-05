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
#include "mgr_managerTask.h"
#include "sts_statusHeartbeat.h"
#include "pkt_packetParser.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "LTC2941-1.h"

#define DATA_BOARD_TERMINAL_MSG_LENGTH	200
#define DATA_BOARD_TERMINAL_MSG_FREQ	2	// this controls the size of queue to data router

/*	Static functions forward declaration	*/
static void processPacket(pkt_rawPacket_t *packet);

/*	Extern variables	*/
extern xQueueHandle queue_sensorHandler;
extern bool enableStream;
extern uint32_t reqSensorMask;
extern uint8_t dataRate;

/*	Local variables	*/
pkt_rawPacket_t dataBoardPacket;
xQueueHandle queue_dataBoard = NULL;		// queue to pass data to the data router
xSemaphoreHandle semaphore_dataBoardUart = NULL;

static drv_uart_config_t dataBoardPortConfig =		// TODO: undefine the UART configuration in brd_board.c
{
	.mem_index = -1,		// the driver will assign the mem_index
	.p_usart = UART1,
	.uart_options = 
	{
		.baudrate = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits = CONF_STOPBITS
	},
	.mode = DRV_UART_MODE_INTERRUPT
};

void dat_dataBoardManager(void *pvParameters)
{
	UNUSED(pvParameters);
	pkt_rawPacket_t sensorPacket;
	uint32_t buffer;
	subp_status_t systemStatus;
	uint8_t buff[10] = {0};
	
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
		if (pkt_getPacketTimed(&dataBoardPortConfig, &dataBoardPacket, 100) == STATUS_PASS)
		{
			// process the packet
			processPacket(&dataBoardPacket);
		}
		
		if (xQueueReceive(queue_dataBoard, &buffer, 100) == TRUE)
		{
			sts_getSystemStatus(&systemStatus);		
		}
		
		vTaskDelay(100);
	}
}

void processPacket(pkt_rawPacket_t *packet)
{
	uint16_t chargeLevel;
	subp_status_t systemStatus;
	
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
				// use semaphore to access the UART
				sts_getSystemStatus(&systemStatus);
				
			break;
			case PACKET_COMMAND_ID_SUBP_CONFIG:
				// set the received sub processor configuration
				dataRate = packet->payload[3];
				reqSensorMask = packet->payload[4] | (packet->payload[5] << 8);	// we ignore the rest of the packet as the total number of sensors is only 9
			break;
			case PACKET_COMMAND_ID_SUBP_STREAMING:
				// enable / disable sensor streaming
				enableStream = packet->payload[2];
			break;
			case PACKET_COMMAND_ID_SUBP_OUTPUT_DATA:
				// simply pass this data to daughter board and USB
				// should not access the USB and daughter board UART directly, pass it on a queue and handle the data in dataRouter
				xQueueSendToBack(queue_dataBoard, &packet->payload[2], 10);
			break;
			default:
			break;
		}
	}
}