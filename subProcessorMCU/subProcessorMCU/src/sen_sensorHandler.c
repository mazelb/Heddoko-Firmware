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
#include "dat_dataRouter.h"
#include "pkt_packetParser.h"
#include "brd_board.h"
#include "arm_math.h"

#define SEN_DEFAULT_DATA_FRAME_LENGTH	37
#define SEN_MIN_DATA_PERIOD	12
#define SEN_MAX_NUM_SENSORS 9

/*	Static function forward declarations	*/
static void sendPacket(uint8_t *data, uint8_t length);	// send raw packet over UART
static void disableRs485Transmit();	// toggle the GPIO to disable RS485 transmit
static void enableRs485Transmit();	// toggle the GPIO to enable Rs485 transmit
static void storePacket(pkt_rawPacket_t *packet);	// store the packet to local Full Frame structure
static void sendFullFrame();	// send the full frame of data from all sensor
static void clearFullFrame();	// clear the full frame
static void sendGetFrame(int sensorId);
static void sendSetupModeEnable();
static void sendUpdateCommand();
static void sendGetDebugStatus();
static void sendUpdateCommandFake();
static void sendResetCommandFake();
static void sendEnableHPR(uint8_t enable);
static void sendChangeBaud(uint32_t baud);
static void sendChangePadding(bool paddingEnable, uint8_t paddingLength);
static status_t isExpectedSensorId(uint8_t byte, uint8_t expectedId);
static status_t isSensorRequested(uint8_t sensorId);
static void sendCommand();
static void changeSensorState(sensor_state_t sensorState);
static status_t verifyFakePacket(pkt_rawPacketNew_t packet);

/*	Extern variables	*/
extern xSemaphoreHandle semaphore_dataBoardUart;
extern drv_uart_config_t uart0Config;
extern dat_dataRouterConfig_t dataRouterConfiguration;

/*	Local variables	*/
uint8_t number_FramesReceived = 0;
uint8_t sensorFullFrame[SEN_MAX_NUM_SENSORS * SEN_DEFAULT_DATA_FRAME_LENGTH] = {0}; // includes the header from individual frames, the final frame is 322 bytes.
uint32_t *p_sensorFullFramePayload = NULL;
sensor_state_t sgSensorState;	// global sensor state
pkt_rawPacket_t sensorPacket;	// packet to store sensor data
static uint8_t sensorLoopCount = 0;	// cycles from 1 to 9 which correspond to the sensor ID 0 to 8. 
bool enableStream = false;	// enables / disables sensor stream
xQueueHandle queue_sensorHandler = NULL;
uint32_t reqSensorMask = 0, detectedSensorMask = 0;
uint8_t dataRate = 0;
static drv_uart_config_t *sensorPortConfig;

#ifdef ENABLE_SENSORS_DEBUG_MODE
/*	IMU packet	*/
#pragma pack(push, 1)
typedef struct
{
	float32_t Quaternion_x;
	float32_t Quaternion_y;
	float32_t Quaternion_z;
	float32_t Quaternion_w;
	uint16_t Magnetic_x;
	uint16_t Magnetic_y;
	uint16_t Magnetic_z;
	uint16_t Acceleration_x;
	uint16_t Acceleration_y;
	uint16_t Acceleration_z;
	uint16_t Rotation_x;
	uint16_t Rotation_y;
	uint16_t Rotation_z;
}imuFrame_t;
#pragma	pack(pop)
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
#endif

/*	Function definitions	*/

/************************************************************************
 * sen_sensorHandler(void *pvParameters)
 * @brief Manages the sensors and handles their data
 * @param void *pvParameters
 * @return void                      
 ************************************************************************/
void sen_sensorHandlerTask(void *pvParameters)
{
	UNUSED(pvParameters);
	
	uint16_t bufferOffset = 0;
	bool firstFrame = TRUE;		// check if it is the very first frame
	pkt_rawPacketNew_t tempPacket;
	
	// initialize the UART driver
	sensorPortConfig = &uart0Config;
	
	// initialize the queue to pass data to task communicating to board
	queue_sensorHandler = xQueueCreate(10, sizeof(pkt_rawPacket_t));
	if (queue_sensorHandler == NULL)
	{
		puts("Failed to create sensor data queue\r\n");
	}
	
	p_sensorFullFramePayload = sensorFullFrame;
	
	sendChangeBaud(SENSOR_BUS_SPEED_HIGH);
	
	while (1)
	{
		if (enableStream)																// fetch frames only if sensor data stream is enabled
		{
			if (sensorLoopCount < 1)													// if all the sensors data are fetched then pass update command
			{
				#ifndef ENABLE_SENSORS_DEBUG_MODE
				sendUpdateCommand();
				#endif
				if (!firstFrame)
				{	
					#ifdef ENABLE_SENSORS_DEBUG_MODE
					sendUpdateCommandFake();
					#endif
					sendFullFrame();
				}
				else
				{
					// reconfigure the baud rate (in case they were disconnected, default speed is LOW)
					sendChangeBaud(SENSOR_BUS_SPEED_LOW);
					sendChangeBaud(SENSOR_BUS_SPEED_HIGH);
					// change the sensor state to streaming
					changeSensorState(SENSOR_STREAMING);
					#ifdef ENABLE_SENSORS_DEBUG_MODE
					sendResetCommandFake();	// send the reset fake command to reset the counts to default values.
					#endif
				}
				clearFullFrame();
				vTaskDelay(1);
				if (dataRate > SEN_MIN_DATA_PERIOD)
				{
					vTaskDelay(dataRate - SEN_MIN_DATA_PERIOD);
				}
				sensorLoopCount = 9;													// reset the loop count to get frames from all sensors
				firstFrame = FALSE;
			}
			// request packet
			if (isSensorRequested(sensorLoopCount-1) == STATUS_PASS)		// NOTE: this will fill the rest of the frame with nulls
			{
				sendGetFrame(sensorLoopCount-1);											// actual sensor id count is 0 to 8
				vTaskDelay(1);
			
				// fetch the packet from buffer
				bufferOffset = ((sensorLoopCount-1) * 35) + 5;								// each sensor has 35 bytes + 7 bytes are for main header minus 2 bytes to remove current header
				tempPacket.p_payload = &sensorFullFrame[bufferOffset]; 				
				pkt_getPacketTimedNew(sensorPortConfig, &tempPacket, 1);					// NOTE: needs a pointer to the packet and not the packet.payload
			
				// perform a short check to verify the integrity of the packet
				if (((tempPacket.payloadSize != SEN_DEFAULT_DATA_FRAME_LENGTH) || 
					(isExpectedSensorId(sensorFullFrame[bufferOffset + 2], sensorLoopCount - 1)) != STATUS_PASS))	// check for the size of packet and valid sensor ID
				{
					// this is a corrupt frame, should discard it
					//dat_sendDebugMsgToDataBoard("Corrupt sensor packet\r\n");
					detectedSensorMask &= ~(0x01 << (sensorLoopCount - 1));		// clear the detected sensor mask
					if (reqSensorMask != detectedSensorMask)					// if not all the requested sensors are present then move to Error state
					{
						changeSensorState(SENSOR_ERROR);
					}
				}
				else
				{
					#ifdef ENABLE_SENSORS_DEBUG_MODE
					verifyFakePacket(tempPacket);	// verify the integrity of fake packet
					#endif
					detectedSensorMask |= (0x01 << (sensorLoopCount - 1));		// set the detected sensor mask
					if (reqSensorMask == detectedSensorMask)					// if all the sensors are present then change the state to streaming
					{
						changeSensorState(SENSOR_STREAMING);
					}
					number_FramesReceived++;	// set the count for the number of frames received
				}
			}
			sensorLoopCount--;
		}
		else
		{
			changeSensorState(SENSOR_IDLE);
			sensorLoopCount = 0;	// make sure that if the next time the stream starts, buffer is always cleared
			detectedSensorMask = 0;
			firstFrame = TRUE;
			// sit idle
			vTaskDelay(100);
		}
	}
}

static void sendPacket(uint8_t *data, uint8_t length)
{
	// calculate the delay from the length of data and baud rate
	uint32_t delay = ((length + 3)*10*1000000) / sensorPortConfig->uart_options.baudrate;	// 3 bytes is header for the wrapped frame, 10 is number of bits per byte and 1M is to convert to uS.
	enableRs485Transmit();
	pkt_sendRawPacket(sensorPortConfig, data, length);
	delay_us(delay);	// time taken for the frame to be transmitted
	disableRs485Transmit();
}

static void disableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_LOW);
}

static void enableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_HIGH);
}

static void sendGetFrame(int sensorId)
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_FRAME;
	outputDataBuffer[2] = sensorId;
	sendPacket(outputDataBuffer, 3);
}

static void sendUpdateCommand()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE;
	sendPacket(outputDataBuffer, 2);
}

static void sendSetupModeEnable()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_SETUP_MODE;
	outputDataBuffer[2] = 0x01;
	sendPacket(outputDataBuffer, 3);
}

static void sendGetDebugStatus()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_STATUS;
	outputDataBuffer[2] = 0x00;
	sendPacket(outputDataBuffer, 3);
}

static void sendEnableHPR(uint8_t enable)
{
	// enable Head, Pitch and Roll.
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_ENABLE_HPR;
	outputDataBuffer[2] = enable;
	sendPacket(outputDataBuffer, 3);
}

static void sendChangeBaud(uint32_t baud)
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
	vTaskDelay(10);	// give time to send out the command
	// change local baud rate
	changeUartBaud(baud);
}

static void sendChangePadding(bool paddingEnable, uint8_t paddingLength)
{
	uint8_t outputDataBuffer[4] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_CHANGE_PADDING;
	
	if (sensorPortConfig->mode == DRV_UART_MODE_DMA)		//padding is only used in interrupt driven DMA mode
	{
		outputDataBuffer[2] = paddingEnable;
		outputDataBuffer[3] = paddingLength;
	}
	else
	{
		outputDataBuffer[2] = FALSE;
		outputDataBuffer[3] = NULL;
	}
	
	sendPacket(outputDataBuffer, 4);
}

static void changeSensorState(sensor_state_t sensorState)
{
	if (sensorState != sgSensorState)
	{
		sgSensorState = sensorState;
		// If required add further actions here in a switch statement
	}
}

static void storePacket(pkt_rawPacket_t *packet)	// this function is obsolete as pointer manipulation is used to store data
{
	if (packet->payload[3] > 8)
	{
		return;	// the sensorId is not valid
	}
	uint8_t location = packet->payload[3] *35;		// 36 is the length of data from each sensor
	
	if (packet->payloadSize < 2)
	{
		return; // this is not a valid packet
	}
	
	if ((packet->payload[0] == PACKET_TYPE_IMU_SENSOR) && (packet->payload[0] == PACKET_COMMAND_ID_GET_FRAME_RESP))
	{
		// starting from the 3rd byte is sensor Id followed by sensor data, store it
		memcpy(&sensorFullFrame[location + 7], packet->payload[3], 36);		// we need first 7 bytes for header
		number_FramesReceived++;
	}	
}

static void clearFullFrame()
{
	// clear the local structure holding the full frame of all sensors
	memset(&sensorFullFrame, 0x00, sizeof(sensorFullFrame));
	number_FramesReceived = 0;
}

static void sendFullFrame()
{
	uint32_t frameSize = 0;
	long sysTickCount = xTaskGetTickCount();
	// send all sensors data to the data board
	if (xSemaphoreTake(semaphore_dataBoardUart, 1) == TRUE)
	{
		sensorFullFrame[0] = PACKET_TYPE_SUB_PROCESSOR;
		sensorFullFrame[1] = PACKET_COMMAND_ID_SUBP_FULL_FRAME;
		sensorFullFrame[2] = number_FramesReceived;
		// assign the sys tick, little endian
		sensorFullFrame[3] = (sysTickCount >> 0) & 0xff;
		sensorFullFrame[4] = (sysTickCount >> 8) & 0xff;
		sensorFullFrame[5] = (sysTickCount >> 16) & 0xff;
		sensorFullFrame[6] = (sysTickCount >> 24) & 0xff;
		// calculate the frame size, keep 7 bytes for the main frame header
		frameSize = (number_FramesReceived * 35) + 7;
		// send the packet out.
		pkt_sendRawPacket(dataRouterConfiguration.dataBoardUart, sensorFullFrame, frameSize);	// TODO: reduce the packet size according to the number of sensors present
		xSemaphoreGive(semaphore_dataBoardUart);
	}
}

sensor_state_t sen_getSensorState(void)
{
	return sgSensorState;
}

uint32_t sen_getDetectedSensors(void)
{
	return detectedSensorMask;
}

static status_t isExpectedSensorId(uint8_t byte, uint8_t expectedId)
{
	if (0 <= byte <= 8)
	{
		if (byte == expectedId)
		{
			return STATUS_PASS;
		}
	}
	return STATUS_FAIL;
}

static status_t isSensorRequested(uint8_t sensorId)
{
	if (0 <= sensorId <= 8)
	{
		if (((reqSensorMask >> sensorId) & 0x01)!= 0)
		{
			return STATUS_PASS;
		}
	}
	return STATUS_FAIL;
}

void sen_enableSensorStream(bool enable)
{
	if (reqSensorMask != 0)
	{
		enableStream = enable;	// only enable the stream if any sensors are requested
	}
}

/*	Debug commands	*/
#ifdef ENABLE_SENSORS_DEBUG_MODE
static status_t verifyFakePacket(pkt_rawPacketNew_t packet)
{
	// compare the received data with the local debug structure, both should be the same
	status_t status = STATUS_PASS;
	imuFrame_t *rxFrame = (imuFrame_t*) (packet.p_payload+3);	// first three bytes are header in the packet
	if (rxFrame->Quaternion_w != (imuFrameData.Quaternion_w))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Quaternion_x != (imuFrameData.Quaternion_x))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Quaternion_y != (imuFrameData.Quaternion_y))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Quaternion_z != (imuFrameData.Quaternion_z))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Rotation_x != (imuFrameData.Rotation_x))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Rotation_y != (imuFrameData.Rotation_y))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Rotation_z != (imuFrameData.Rotation_z))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Acceleration_x != (imuFrameData.Acceleration_x))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Acceleration_y != (imuFrameData.Acceleration_y))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Acceleration_z != (imuFrameData.Acceleration_z))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Magnetic_x != (imuFrameData.Magnetic_x))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Magnetic_y != (imuFrameData.Magnetic_y))
	{
		status |= STATUS_FAIL;
	}
	if (rxFrame->Magnetic_z != (imuFrameData.Magnetic_z))
	{
		status |= STATUS_FAIL;
	}
	return status;
}

static void sendResetCommandFake()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_RESET_FAKE;
	sendPacket(outputDataBuffer, 2);
	// reset the local structure
	imuFrameData.Quaternion_x = 0.1;
	imuFrameData.Quaternion_y = 0.2;
	imuFrameData.Quaternion_z = 0.3;
	imuFrameData.Quaternion_w = 0.4;
	imuFrameData.Magnetic_x = 1;
	imuFrameData.Magnetic_y = 2;
	imuFrameData.Magnetic_z = 3;
	imuFrameData.Acceleration_x = 4;
	imuFrameData.Acceleration_y = 5;
	imuFrameData.Acceleration_z = 6;
	imuFrameData.Rotation_x = 7;
	imuFrameData.Rotation_y = 8;
	imuFrameData.Rotation_z = 9;
}

void sendUpdateCommandFake()
{
	uint8_t outputDataBuffer[3] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE_FAKE;
	sendPacket(outputDataBuffer, 2);
	// update the local structure.
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
#endif