/*
 * task_SensorHandler.c
 *
 * Created: 2016-04-18 12:44:13 PM
 *  Author: Hriday Mehta
 */ 

#include "task_SensorHandler.h"
#include "pkt_packetParser.h"
#include "cmd_commandProcessor.h"
#include "drv_uart.h"
#include "imu.h"
#include "msg_messenger.h"
#include "sys_systemManager.h"

//#define SENSOR_PORT		USART1_IDX		//assign a default comm bus to the module

/*	Extern functions	*/
extern void configureBusSpeed(drv_uart_config_t* uartConfig, uint32_t newBaud);

/*	Extern variables	*/
extern system_port_config_t sys_comPorts;
extern drv_uart_config_t usart1Config;
extern drv_uart_config_t uart0Config;
extern bool enableStreaming;
extern struct
{
	rawPacket_t *p_packet;
	packetCallback_t callBackFunction;
}localPackets[4];

/*	Local functions	*/
static void processEvent(msg_message_t message);	//process the received event
static void msgSensorModuleReady();
static void msgSensorModuleError(sensor_error_code_t errorCode);
void changeSensorState(sensor_state_t sensorState);
static void systemStateChangeAction(msg_sys_manager_t* sys_eventData);

/*	Local variables	*/
//uint8_t sensor_comPort = SENSOR_PORT;
sensor_state_t sgSensorState;
xQueueHandle queue_sensorHandler = NULL;
bool send_proto_packet = FALSE;
static bool enableSensor = FALSE;
uint8_t loopCount = 9;

imuFrame_t imuFrameData =
{
	.Quaternion_x = 0.1,
	.Quaternion_y = 0.2,
	.Quaternion_z = 0.3,
	.Quaternion_w = 0.4,
	.Magnetic_x = 1,
	.Magnetic_y = 2,
	.Magnetic_z = 3,
	.Acceleration_x = 4,
	.Acceleration_y = 5,
	.Acceleration_z = 6,
	.Rotation_x = 7,
	.Rotation_y = 8,
	.Rotation_z = 9
};

void enableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_HIGH);
}
void disableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_LOW);
}
void changeSensorBaud()
{
	sendChangeBaud(SENSOR_BUS_SPEED_HIGH);
	delay_ms(2);	//wait for the data to be transmitted
	configureBusSpeed(sys_comPorts.sensor_port, SENSOR_BUS_SPEED_HIGH);
	//reInitAllUarts();
}

//pkt_packetParserConfiguration_t packetParserConfig =
//{
	//.transmitDisable = disableRs485Transmit,
	//.transmitEnable = enableRs485Transmit,
	//.packetReceivedCallback = cmd_processPacket,
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
	.mem_index = 3,
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
		.transmitDisable = disableRs485Transmit,
		.transmitEnable = enableRs485Transmit,
		.packetReceivedCallback = cmd_processPacket,
		.packet =
		{
			.payload = {NULL},
			.payloadSize = NULL,
			.bytesReceived = NULL,
			.escapeFlag = NULL
		}
	}
};

void task_SensorHandler(void *pvParameters)
{
	UNUSED(pvParameters);
	status_t status = STATUS_FAIL;
	
	uint8_t count = 0;
	rawPacket_t sensorPacket;
	msg_message_t eventMessage;
	
	sensorPacket.bytesReceived = 0;
	sensorPacket.escapeFlag = 0;
	sensorPacket.payloadSize = 0;
	memcpy(sensorPacket.payload, NULL, RAW_PACKET_MAX_SIZE);
	sgSensorState = SENSOR_NOT_PRESENT;
	disableRs485Transmit();
	
	// initialize the com port
	drv_uart_init(sys_comPorts.dataOutUart);
	drv_uart_init(sys_comPorts.sensor_port);
	
	queue_sensorHandler = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_sensorHandler != 0)
	{
		msg_registerForMessages(MODULE_SENSOR_HANDLER, 0xff, queue_sensorHandler);
	}
	
	/*	Packet Encoding check	*/
	pkt_packetParserInit(&usart1Config.pktConfig, sys_comPorts.sensor_port->mem_index);
	if (sys_comPorts.sensor_port->enable_dma_interrupt == TRUE)
	{
		//pkt_registerPacketAndCallback(&usart1Config.pktConfig, &sensorPacket, cmd_processPacket);
	}
	protoPacketInit();
	
	#ifdef ENABLE_SENSOR_PACKET_TEST
	sendResetCommandFake();
	#endif
	//changeSensorBaud();
	//vTaskDelay(5);	//wait for the sensor module to reconfigure itself
 	//sendEnableHPR(1);
	//vTaskDelay(5);	//wait for the sensor module to reconfigure itself
	
	while(1)
	{
		if (enableSensor == true)
		{
			if (loopCount >= 9)
			{
				send_proto_packet = TRUE;
				#ifdef ENABLE_SENSOR_PACKET_TEST
				sendUpdateCommandFake();
				#else
				sendUpdateCommand();	
				#endif
				sendProtoPacket();
				clearProtoPacket();	//re-Init the protoBuff packet
				vTaskDelay(3);
				loopCount = 0;
				if (xQueueReceive(queue_sensorHandler, &eventMessage, 1) == true)	//Process message queues here at every 10-20ms 
				{
					processEvent(eventMessage);
				}
			}
			
			if (sys_comPorts.sensor_port->enable_dma_interrupt != TRUE)
			{
				drv_uart_DMA_reInit(&usart1Config, UART_DMA_BUFFER_A);
			}
			sendGetFrame(0);
			//sendGetDebugStatus();
			vTaskDelay(1);
			if (sys_comPorts.sensor_port->enable_dma_interrupt != TRUE)
			{
				pkt_getRawPacket(100, 35, sys_comPorts.sensor_port->mem_index);
			}
			
			
			
			//TODO: add a 1ms delay only when the uart speed is less than 2Mbps.
			loopCount++;
		}
		else
		{
			//Just process message queues if the sensor is not enabled
			if (xQueueReceive(queue_sensorHandler, &eventMessage, 1) == true)
			{
				processEvent(eventMessage);
			}
			vTaskDelay(100);
		}
	}
}

static void processEvent(msg_message_t message)
{
	msg_sys_manager_t* sys_eventData;
	sys_eventData = (msg_sys_manager_t *) message.parameters;
	
	switch (message.type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
			//if (sys_eventData->enableSensorStream == true)
			//{
				//if (sgSensorState == SENSOR_READY)
				//{
					//msgSensorModuleReady();
				//}
				//else
				//{
					//enableStreaming = true;
					//enableSensor = true;
					//drv_uart_DMA_initRx(&usart1Config);
					//sendChangePadding(TRUE, PACKET_PADDING_LENGTH);
					//msgSensorModuleReady();	//for testing, remove it when done
				//}
			//}
			//else
			//{
				//enableStreaming = false;
				//enableSensor = false;
				//drv_uart_deInitRx(&usart1Config);
				//changeSensorState(SENSOR_STANDBY);
				//printf("Total bytes received error: %d\r\n", localPackets[3].p_packet->bytesReceived);
			//}
			systemStateChangeAction(message.broadcastData);
		break;
		case MSG_TYPE_SDCARD_STATE:
		case MSG_TYPE_READY:
		case MSG_TYPE_ERROR:
		case MSG_TYPE_SDCARD_SETTINGS:
		case MSG_TYPE_WIFI_STATE:
		case MSG_TYPE_WIFI_SETTINGS:
		case MSG_TYPE_USB_CONNECTED:
		case MSG_TYPE_CHARGER_EVENT:
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		default:
		break;
	}
	
	free(message.parameters);
}

void sendGetFrame(int sensorId)
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_FRAME;
	outputDataBuffer[2] = sensorId;
	pkt_SendRawPacket(outputDataBuffer, 3, sys_comPorts.sensor_port->mem_index);
}

void sendUpdateCommand()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE;
	pkt_SendRawPacket(outputDataBuffer, 2, sys_comPorts.sensor_port->mem_index);
}

void sendSetupModeEnable()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_SETUP_MODE;
	outputDataBuffer[2] = 0x01;
	pkt_SendRawPacket(outputDataBuffer, 3, sys_comPorts.sensor_port->mem_index);
}

void sendGetDebugStatus()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_STATUS;
	outputDataBuffer[2] = 0x00;
	pkt_SendRawPacket(outputDataBuffer, 3, sys_comPorts.sensor_port->mem_index);
}

void sendUpdateCommandFake()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE_FAKE;
	pkt_SendRawPacket(outputDataBuffer, 2, sys_comPorts.sensor_port->mem_index);
		
	imuFrameData.Acceleration_x++;
	imuFrameData.Acceleration_y++;
	imuFrameData.Acceleration_z++;
	imuFrameData.Magnetic_x++;
	imuFrameData.Magnetic_y++;
	imuFrameData.Magnetic_z++;
	imuFrameData.Rotation_x++;
	imuFrameData.Rotation_y++;
	imuFrameData.Rotation_z++;
	imuFrameData.Quaternion_w++;
	imuFrameData.Quaternion_x++;
	imuFrameData.Quaternion_y++;
	imuFrameData.Quaternion_z++;
}

void sendResetCommandFake()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_RESET_FAKE;
	pkt_SendRawPacket(outputDataBuffer, 2, sys_comPorts.sensor_port->mem_index);
}

void sendEnableHPR(uint8_t enable)
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_ENABLE_HPR;
	outputDataBuffer[2] = enable;
	pkt_SendRawPacket(outputDataBuffer, 3, sys_comPorts.sensor_port->mem_index);
}

void sendChangeBaud(uint32_t baud)
{
	uint8_t outputDataBuffer[6] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_CHANGE_BAUD;
	//send out the baud rate in 8bits, LSB first
	outputDataBuffer[2] = (uint8_t)(baud & 0x000000ff);
	outputDataBuffer[3] = (uint8_t)((baud & 0x0000ff00) >> 8);
	outputDataBuffer[4] = (uint8_t)((baud & 0x00ff0000) >> 16);
	outputDataBuffer[5] = (uint8_t)((baud & 0xff000000) >> 24);
	pkt_SendRawPacket(outputDataBuffer, 6, sys_comPorts.sensor_port->mem_index);
}

void sendChangePadding(bool paddingEnable, uint8_t paddingLength)
{
	uint8_t outputDataBuffer[4] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_CHANGE_PADDING;
	
	if (sys_comPorts.sensor_port->enable_dma_interrupt == TRUE)		//padding is only used in interrupt driven DMA mode
	{
		outputDataBuffer[2] = paddingEnable;
		outputDataBuffer[3] = paddingLength;
	}
	else
	{
		outputDataBuffer[2] = FALSE;
		outputDataBuffer[3] = NULL;
	}
	
	pkt_SendRawPacket(outputDataBuffer, 4, sys_comPorts.sensor_port->mem_index);
}

static void msgSensorModuleReady()
{
	status_t status = STATUS_PASS;
	uint8_t eventData[2] = {0};
	msg_sensor_state_t* messageData;
	
	messageData = malloc (sizeof(msg_sensor_state_t));
	messageData->sensorInitialized = true;
	messageData->errorCode = 0;
	msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SENSOR_HANDLER, MSG_TYPE_READY, (void*)messageData);
}

static void msgSensorModuleError(sensor_error_code_t errorCode)
{
	status_t status = STATUS_PASS;
	uint8_t eventData[2] = {0};
	msg_sensor_state_t* messageData;
	
	messageData = malloc (sizeof(msg_sensor_state_t));
	messageData->sensorInitialized = false;
	messageData->errorCode = errorCode;
	msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_SENSOR_HANDLER, MSG_TYPE_ERROR, (void*)messageData);
}

void changeSensorState(sensor_state_t sensorState)
{
	if (sensorState != sgSensorState)
	{
		switch (sensorState)
		{
			case SENSOR_READY:
				sgSensorState = SENSOR_READY;
				msgSensorModuleReady();
			break;
			case SENSOR_STANDBY:
				sgSensorState = SENSOR_STANDBY;
				//msgSensorModuleReady();
			break;
			case SENSOR_NOT_PRESENT:
				sgSensorState = SENSOR_NOT_PRESENT;
				msgSensorModuleError(SENSOR_DISCONNECTED);
			break;
			case SENSOR_COMM_ERROR:
				sgSensorState = SENSOR_COMM_ERROR;
				msgSensorModuleError(SENSOR_PACKET_INCOMPLETE);
			break;
			default:
			break;
		}
	}
}

static void systemStateChangeAction(msg_sys_manager_t* sys_eventData)
{
	//Entering new state messages are always broadcast messages
	switch (sys_eventData->systemState)
	{
		case SYSTEM_STATE_SLEEP:
			enableStreaming = false;
			enableSensor = false;
			drv_uart_deInitRx(sys_comPorts.sensor_port);
			changeSensorState(SENSOR_STANDBY);
			printf("Total bytes received error: %d\r\n", localPackets[3].p_packet->bytesReceived);
		break;
		
		case SYSTEM_STATE_INIT:
			if (sgSensorState == SENSOR_READY)
			{
				msgSensorModuleReady();
			}
			else
			{
				drv_uart_DMA_initRx(sys_comPorts.sensor_port);
				sendChangePadding(TRUE, PACKET_PADDING_LENGTH);
				changeSensorState(SENSOR_READY);
			}
		break;
		
		case SYSTEM_STATE_IDLE:
			enableStreaming = false;
			enableSensor = false;
			changeSensorState(SENSOR_STANDBY);
		break;
		
		case SYSTEM_STATE_ERROR:
			enableStreaming = false;
			enableSensor = false;
			changeSensorState(SENSOR_STANDBY);
		break;
		
		case SYSTEM_STATE_RECORDING:
			//if (sgSensorState == SENSOR_READY)
			//{
				enableStreaming = true;
				enableSensor = true;
			//}
		break;
		
		default:
		break;
	}
}