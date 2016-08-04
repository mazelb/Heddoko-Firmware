/*
 * sen_sensorHandler.c
 *
 * Created: 7/28/2016 3:39:16 PM
 * Author: Hriday Mehta
 */ 

#include <asf.h>
#include "string.h"
#include "common.h"
#include "sen_sensorHandler.h"
#include "drv_uart.h"
#include "pkt_packetCommandsList.h"
#include "pkt_packetParser.h"

/*	Static function forward declarations	*/
static void sendPacket(uint8_t *data, uint8_t length);	// send raw packet over UART
static void disableRs485Transmit();	// toggle the GPIO to disable RS485 transmit
static void enableRs485Transmit();	// toggle the GPIO to enable Rs485 transmit
static void storePacket(pkt_rawPacket_t *packet);	// store the packet to local Full Frame structure
static void sendFullFrame();	// send the full frame of data from all sensor
static void clearFullFrame();	// clear the full frame

/*	Extern variables	*/
extern xSemaphoreHandle semaphore_dataBoardUart;

/*	Local variables	*/
uint8_t number_FramesReceived = 0;
pkt_rawPacket_t sensorFullFrame;
sensor_state_t sgSensorState;
pkt_rawPacket_t sensorPacket;	// packet to store sensor data
static uint8_t sensorLoopCount = 0;	// controls the active sensor to fetch from
bool enableStream = false;	// enables / disables sensor stream
xQueueHandle queue_sensorHandler = NULL;
uint32_t reqSensorMask = 0, errSensorMask = 0;
uint8_t dataRate = 0;

static drv_uart_config_t sensorPortConfig = 
{
	.mem_index = -1,		// the driver will assign the mem_index
	.p_usart = USART0,		// TODO: verify the actual physical port for sensor interface
	.uart_options = 
	{
		.baudrate = SENSOR_BUS_SPEED_LOW,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
};

/*	Function definitions	*/

/************************************************************************
 * sen_sensorHandler(void *pvParameters)
 * @brief Manages the sensors and handles their data
 * @param void *pvParameters
 * @return void                      
 ************************************************************************/
void sen_sensorHandler(void *pvParameters)
{
	uint8_t bufferOffset = 0;
	UNUSED(pvParameters);
	
	// initialize the UART driver
	drv_uart_init(&sensorPortConfig);
	
	// initialize the queue to pass data to task communicating to board
	queue_sensorHandler = xQueueCreate(10, sizeof(pkt_rawPacket_t));
	if (queue_sensorHandler == NULL)
	{
		puts("Failed to create sensor data queue\r\n");
	}
	
	while (1)
	{
		if (enableStream)	// fetch frames only if sensor data stream is enabled
		{
			if (sensorLoopCount < 1)
			{
				sendUpdateCommand();
				sendFullFrame();
				clearFullFrame();
				vTaskDelay(3);
				sensorLoopCount = 9;
			}
			// request packet
			sendGetFrame(sensorLoopCount - 1);	// actual sensor id count is 0 to 8
			vTaskDelay(1);
			
			// fetch the packet from buffer
			bufferOffset = ((sensorLoopCount-1) * 36) + 5;	// each sensor has 36 bytes + 7 bytes are for main header minus 2 bytes to remove current header
			//drv_uart_getPacketTimed(&sensorPortConfig, sensorFullFrame.payload[bufferOffset], 100);
			
			sensorLoopCount--;
		}
		else
		{
			// sit idle
			vTaskDelay(100);
		}
	}
}

static void sendPacket(uint8_t *data, uint8_t length)
{
	disableRs485Transmit();
	//drv_uart_sendPacket(&sensorPortConfig, );		// assuming that the uart driver will wrap the packet
	enableRs485Transmit();
}

static void disableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_LOW);
}

static void enableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_HIGH);
}

void sendGetFrame(int sensorId)
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_FRAME;
	outputDataBuffer[2] = sensorId;
	sendPacket(outputDataBuffer, 3);
}

void sendUpdateCommand()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE;
	sendPacket(outputDataBuffer, 2);
}

void sendSetupModeEnable()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_SETUP_MODE;
	outputDataBuffer[2] = 0x01;
	sendPacket(outputDataBuffer, 3);
}

void sendGetDebugStatus()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_STATUS;
	outputDataBuffer[2] = 0x00;
	sendPacket(outputDataBuffer, 3);
}

void sendResetCommandFake()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_RESET_FAKE;
	sendPacket(outputDataBuffer, 2);
}

void sendEnableHPR(uint8_t enable)
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_ENABLE_HPR;
	outputDataBuffer[2] = enable;
	sendPacket(outputDataBuffer, 3);
}

void sendChangeBaud(uint32_t baud)
{
	uint8_t outputDataBuffer[6] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_CHANGE_BAUD;
	//send out the baud rate in 8bits, LSB first (Little endian)
	outputDataBuffer[2] = (uint8_t)(baud & 0x000000ff);
	outputDataBuffer[3] = (uint8_t)((baud & 0x0000ff00) >> 8);
	outputDataBuffer[4] = (uint8_t)((baud & 0x00ff0000) >> 16);
	outputDataBuffer[5] = (uint8_t)((baud & 0xff000000) >> 24);
	sendPacket(outputDataBuffer, 6);
}

void sendChangePadding(bool paddingEnable, uint8_t paddingLength)
{
	uint8_t outputDataBuffer[4] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_CHANGE_PADDING;
	
	//if (sensorPortConfig.uartMode == DRV_UART_MODE_PACKET_PARSER_DMA)		//padding is only used in interrupt driven DMA mode
	//{
		outputDataBuffer[2] = paddingEnable;
		outputDataBuffer[3] = paddingLength;
	//}
	//else
	//{
		outputDataBuffer[2] = FALSE;
		outputDataBuffer[3] = NULL;
	//}
	
	sendPacket(outputDataBuffer, 4);
}

void changeSensorState(sensor_state_t sensorState)
{
	if (sensorState != sgSensorState)
	{
		sgSensorState = sensorState;
		// add further actions here in a switch statement
	}
}

static void storePacket(pkt_rawPacket_t *packet)
{
	uint8_t location = packet->payload[3] *36;		// 36 is the length of data from each sensor
	
	if (packet->payloadSize < 2)
	{
		return; // this is not a valid packet
	}
	
	if ((packet->payload[0] == PACKET_TYPE_IMU_SENSOR) && (packet->payload[0] == PACKET_COMMAND_ID_GET_FRAME_RESP))
	{
		// starting from the 3rd byte is sensor Id followed by sensor data, store it
		memcpy(&sensorFullFrame.payload[location + 7], packet->payload[3], 36);		// we need first 7 bytes for header
		sensorFullFrame.payloadSize += 36;
		number_FramesReceived++;
	}	
}

static void clearFullFrame()
{
	// clear the local structure holding the full frame of all sensors
	memset(&sensorFullFrame, 0x00, sizeof(sensorFullFrame));
	sensorFullFrame.payloadSize = 0;	// TODO: do i really need this or I wrote it out by mistake?
	number_FramesReceived = 0;
}

static void sendFullFrame()
{
	long sysTickCount = xTaskGetTickCount();
	// send all sensors data to the data board
	if (xSemaphoreTake(semaphore_dataBoardUart, 1) == TRUE)
	{
		sensorFullFrame.payload[0] = PACKET_TYPE_SUB_PROCESSOR;
		sensorFullFrame.payload[1] = PACKET_COMMAND_ID_SUBP_FULL_FRAME;
		sensorFullFrame.payload[2] = number_FramesReceived;
		// assign the sys tick, little endian
		sensorFullFrame.payload[3] = (sysTickCount >> 0) & 0xff;
		sensorFullFrame.payload[4] = (sysTickCount >> 8) & 0xff;
		sensorFullFrame.payload[5] = (sysTickCount >> 16) & 0xff;
		sensorFullFrame.payload[6] = (sysTickCount >> 24) & 0xff;
		// drv_uart_sendPacket();		// function call to send the packet.
		xSemaphoreGive(semaphore_dataBoardUart);
	}
}

sensor_state_t sen_getSensorState(void)
{
	return SENSOR_IDLE;
}

uint32_t sen_getDetectedSensors(void)
{
	return 0x000001ff;
}