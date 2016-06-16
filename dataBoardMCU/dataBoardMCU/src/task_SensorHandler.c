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

//extern functions
extern void reInitAllUarts();

//local functions
static void processEvent(msg_message_t message);	//process the received event
static void msgSensorModuleReady();
static void msgSensorModuleError(sensor_error_code_t errorCode);
void changeSensorState(sensor_state_t sensorState);

//extern variables
extern drv_uart_config_t usart1Config;
extern drv_uart_config_t uart0Config;
extern bool enableStreaming;

//local variables
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
	sendChangeBaud(2000000);
	delay_ms(2);	//wait for the data to be transmitted
	reInitAllUarts();
}

pkt_packetParserConfiguration_t packetParserConfig =
{
	.transmitDisable = disableRs485Transmit,
	.transmitEnable = enableRs485Transmit,
	.packetReceivedCallback = cmd_processPacket,
	.uartModule = &usart1Config
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
	
	queue_sensorHandler = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_sensorHandler != 0)
	{
		msg_registerForMessages(MODULE_SENSOR_HANDLER, 0xff, queue_sensorHandler);
	}
	//Enable Bus Matrix arbitration
	//for (int i=0; i<5; i++)
	//{
		//matrix_set_slave_arbitration_type(i, MATRIX_ARBT_ROUND_ROBIN);
		//matrix_set_slave_default_master_type(i, MATRIX_DEFMSTR_NO_DEFAULT_MASTER);
	//}
	
	//while(brainSettings.isLoaded != 1);
	
	/*	Packet Encoding check	*/
	pkt_packetParserInit(&packetParserConfig);
	protoPacketInit();
	//sendResetCommandFake();
	changeSensorBaud();
 	vTaskDelay(5);	//wait for the sensor module to reconfigure itself
 	sendEnableHPR(1);
	vTaskDelay(5);	//wait for the sensor module to reconfigure itself
	
	while(1)
	{
		if (enableSensor == true)
		{
			if (loopCount >= 9)
			{
				send_proto_packet = TRUE;
				sendUpdateCommand();	
				sendProtoPacket();
				clearProtoPacket();	//re-Init the packet
				vTaskDelay(2);
				loopCount = 0;
				if (xQueueReceive(queue_sensorHandler, &eventMessage, 1) == true)
				{
					processEvent(eventMessage);
				}
			}		
			drv_uart_DMA_reInit(packetParserConfig.uartModule);
			sendGetFrame(loopCount);
			//sendGetDebugStatus();
			delay_ms(1);
			pkt_getRawPacket(&sensorPacket, 100, 35);
			loopCount++;
		}
		else
		{
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
			if (sys_eventData->enableSensorStream == true)
			{
				if (sgSensorState == SENSOR_READY)
				{
					msgSensorModuleReady();
				}
				else
				{
					enableStreaming = true;
					enableSensor = true;
				}
			}
			else
			{
				enableStreaming = false;
				enableSensor = false;
				changeSensorState(SENSOR_STANDBY);
			}
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
	pkt_SendRawPacket(outputDataBuffer, 3);
}

void sendUpdateCommand()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE;
	pkt_SendRawPacket(outputDataBuffer, 2);
}

void sendSetupModeEnable()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_SETUP_MODE;
	outputDataBuffer[2] = 0x01;
	pkt_SendRawPacket(outputDataBuffer, 3);
}

void sendGetDebugStatus()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_STATUS;
	outputDataBuffer[2] = 0x00;
	pkt_SendRawPacket(outputDataBuffer, 3);
}

void sendUpdateCommandFake()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE_FAKE;
	pkt_SendRawPacket(outputDataBuffer, 2);
		
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
	pkt_SendRawPacket(outputDataBuffer, 2);
}

void sendEnableHPR(uint8_t enable)
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_ENABLE_HPR;
	outputDataBuffer[2] = enable;
	pkt_SendRawPacket(outputDataBuffer, 3);
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
	pkt_SendRawPacket(outputDataBuffer, 6);
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