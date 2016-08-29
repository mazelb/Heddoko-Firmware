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

/*	Local defines	*/
#define SEN_DEFAULT_DATA_FRAME_LENGTH		37	// the size of data frame from the sensor
#define SEN_DEFAULT_DATA_SIZE				35	// the size of actual unwrapped data
#define SEN_SENSOR_FRAME_HEADER_SIZE		2	// first two byte are header in the frame received from the sensor
#define SEN_SENSOR_FRAME_SENID_OFFSET		SEN_SENSOR_FRAME_HEADER_SIZE	// the sensor id is offset by the amount of bytes of header
#define SEN_LOOP_PERIOD						12	// this is the amount of time(ms) taken by the loop to execute
#define SEN_MAX_NUM_SENSORS					9	// the maximum number of sensor used in the product
#define SEN_MAX_SENSOR_ID					8	// sensorId range from 0 to 8
#define SEN_MIN_SENSOR_ID					0
#define SEN_SENSOR_FULL_FRAME_HEADER_SIZE	7	// the sensor full frame has a header of 7 bytes

/*	Static function forward declarations	*/
static void sendPacket(uint8_t *data, uint8_t length);	// send raw packet over UART
static void disableRs485Transmit();						// toggle the GPIO to disable RS485 transmit
static void enableRs485Transmit();						// toggle the GPIO to enable Rs485 transmit
static void storePacket(pkt_rawPacket_t *packet);		// store the packet to local Full Frame structure
static void sendFullFrame();							// send the full frame of data from all sensor
static void clearFullFrame();							// clear the full frame
static status_t isExpectedSensorId(uint8_t byte, uint8_t expectedId);
static status_t isSensorRequested(uint8_t sensorId);
static void sendCommand(sensor_commands_t commandId, uint8_t sensorId, uint32_t data);
static void changeSensorState(sensor_state_t sensorState);
static status_t verifyFakePacket(pkt_rawPacketVarSize_t packet);

/*	OBSOLETE FUNCTIONS	- SHALL BE REMOVED AFTER TESTING AND APPROVAL
static void storePacket(pkt_rawPacket_t *packet);	// store the packet to local Full Frame structure
static void sendGetFrame(int sensorId);
static void sendSetupModeEnable();
static void sendUpdateCommand();
static void sendGetDebugStatus();
static void sendUpdateCommandFake();
static void sendResetCommandFake();
static void sendEnableHPR(uint8_t enable);
static void sendChangeBaud(uint32_t baud);
static void sendChangePadding(bool paddingEnable, uint8_t paddingLength);
*/

/*	Extern variables	*/
extern xSemaphoreHandle semaphore_dataBoardUart;
extern drv_uart_config_t uart0Config;
extern dat_dataRouterConfig_t dataRouterConfiguration;

/*	Local variables	*/
bool enableStream = false;				// enables / disables sensor stream
uint8_t number_FramesReceived = 0;		// monitors the total number of valid frames received in one cycle
sensor_state_t sgSensorState;			// global sensor state
static uint8_t sensorLoopCount = 0;		// cycles from 1 to 9 which correspond to the sensor ID 0 to 8.
uint8_t dataRate = 0, numReqSensors = 0;
static drv_uart_config_t *sensorPortConfig;
uint32_t reqSensorMask = 0, detectedSensorMask = 0;
uint8_t sensorFullFrame[SEN_MAX_NUM_SENSORS * SEN_DEFAULT_DATA_FRAME_LENGTH] = {0}; // with a little overhead, the final frame is 322 bytes.

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
	uint8_t sensorID = 0;
	bool firstFrame = TRUE;		// check if it is the very first frame
	pkt_rawPacketVarSize_t tempPacket;	// temporary holder for individual sensor packets
	
	// initialize the UART driver
	sensorPortConfig = &uart0Config;
		
	sendCommand(COMMAND_ID_CHANGE_BAUD, NULL, SENSOR_BUS_SPEED_HIGH);	// change the baud rate of sensors and designated UART.
	
	while (1)
	{
		if (enableStream)																// fetch frames only if sensor data stream is enabled
		{
			if (sensorLoopCount < 1)													// if all the sensors data are fetched then pass update command
			{
				#ifndef ENABLE_SENSORS_DEBUG_MODE
				sendCommand(COMMAND_ID_UPDATE, NULL, NULL);
				#endif
				
				if (!firstFrame)
				{	
					#ifdef ENABLE_SENSORS_DEBUG_MODE
					sendCommand(COMMAND_ID_UPDATE_FAKE, NULL, NULL);
					#endif
					sendFullFrame();
				}
				else
				{
					// change the sensor state to streaming
					changeSensorState(SENSOR_STREAMING);
					#ifdef ENABLE_SENSORS_DEBUG_MODE
					sendCommand(COMMAND_ID_RESET_FAKE, NULL, NULL);	// send the reset fake command to reset the counts to default values.
					#endif
				}
				
				clearFullFrame();
				
				// control the data rate
				vTaskDelay(2);
				if (dataRate > SEN_LOOP_PERIOD)
				{
					vTaskDelay(dataRate - SEN_LOOP_PERIOD);
				}
				else
				{
					// if the number of requested sensors is less then the time taken to execute the loop plus the delay is not enough
					// it needs more time to update the data in the sensors.				 
					if (numReqSensors < 7)
					{
						vTaskDelay(2);
					}
				}
				
				// reset the sensor loop count and first frame bool
				sensorLoopCount = 9;	// reset the loop count to get frames from all sensors
				firstFrame = FALSE;
			}
			
			sensorID = sensorLoopCount - 1;	// sensorID is from 0 to 8.
			// request packet
			if (isSensorRequested(sensorID) == STATUS_PASS)		// only fetch the frame if the sensor is requested
			{
				sendCommand(COMMAND_ID_GET_FRAME, sensorID, NULL);											// actual sensor id count is 0 to 8
				vTaskDelay(1);
			
				// fetch the packet from buffer
				
				/* we start filling the buffer from its tail. The location of the data for sensorId #8 is the tail,
				   Number of frames received predicts how much data has been written, then,
				   each sensor has 35 bytes + 7 bytes are for main header minus 2 bytes to remove current header */
				
				bufferOffset = ((SEN_MAX_SENSOR_ID - number_FramesReceived) * SEN_DEFAULT_DATA_SIZE) + SEN_SENSOR_FULL_FRAME_HEADER_SIZE - SEN_SENSOR_FRAME_HEADER_SIZE;
				tempPacket.p_payload = sensorFullFrame + bufferOffset; 						// set the pointer to the set offset in the full frame array
				pkt_getPacketTimedNew(sensorPortConfig, &tempPacket, 1);					// NOTE: needs a pointer to the packet and not the packet.payload
			
				// perform a short check to verify the integrity of the packet
				if (((tempPacket.payloadSize != SEN_DEFAULT_DATA_FRAME_LENGTH) || 
					(isExpectedSensorId(sensorFullFrame[bufferOffset + SEN_SENSOR_FRAME_SENID_OFFSET], sensorID)) != STATUS_PASS))	// check for the size of packet and a valid sensor ID
				{
					// this is a corrupt frame, should discard it
					detectedSensorMask &= ~(0x01 << (sensorID));		// clear the detected sensor mask
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
					
					detectedSensorMask |= (0x01 << (sensorID));		// set the detected sensor mask
					if (reqSensorMask == detectedSensorMask)					// if all the sensors are present then change the state to streaming
					{
						changeSensorState(SENSOR_STREAMING);
					}
					tempPacket.payloadSize = 0;	// reset the payload size as it will be reused for next packet
					number_FramesReceived++;	// set the count for the number of frames received
				}
			}
			
			else	// if the sensor is not requested then just clear it's detected bit
			{
				// clear the bit in the detected mask
				detectedSensorMask &= ~(0x01 << (sensorID));
			}
			sensorLoopCount--;
		}
		
		else	// if the sensor stream is not enabled then enter idle loop
		{
			changeSensorState(SENSOR_IDLE);
			sensorLoopCount = 0;	// make sure that if the next time the stream starts, buffer is always cleared
			detectedSensorMask = 0;
			firstFrame = TRUE;
			// sit idle
			vTaskDelay(100);
		}
		/*	End of 'enableStream' if-else loop	*/
	}
	/*End of While(1) loop	*/
}

/************************************************************************
 * sendPacket(uint8_t *data, uint8_t length)
 * @brief Sends the requested data to the sensors. Calculates the time
 *        taken by the data to be sent dynamically which is used to provide
 *		  delay between enabling and disabling the RS485 transceiver
 * @param uint8_t *data - pointer to the data, uint8_t length - data length
 * @return void                      
 ************************************************************************/
static void sendPacket(uint8_t *data, uint8_t length)
{
	// dynamically calculate the delay from length of the data and UART baud rate
	uint32_t delay = ((length + 3)*10*1000000) / sensorPortConfig->uart_options.baudrate;	// 3 bytes is header for the wrapped frame, 10 is number of bits per byte and 1M is to convert to uS.
	enableRs485Transmit();
	pkt_sendRawPacket(sensorPortConfig, data, length);
	delay_us(delay);	// time taken for the frame to be transmitted
	disableRs485Transmit();
}

/************************************************************************
 * disableRs485Transmit()
 * @brief Puts the transceiver in receive mode
 * @param void
 * @return void                      
 ************************************************************************/
static void disableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_LOW);
}

/************************************************************************
 * enableRs485Transmit()
 * @brief Puts the transceiver in transmit mode
 * @param void
 * @return void                      
 ************************************************************************/
static void enableRs485Transmit()
{
	drv_gpio_setPinState(GPIO_RS485_DATA_DIRECTION_RE,  DRV_GPIO_PIN_STATE_HIGH);
}

/************************************************************************
 * sendCommand(sensor_commands_t commandId, uint8_t sensorId, uint32_t data)
 * @brief Sends the requested command to the sensor module
 * @param sensor_commands_t commandId, uint8_t sensorId, uint32_t data - data to be passed
 * @return void                      
 ************************************************************************/
static void __attribute__((Optimize("O0"))) sendCommand(sensor_commands_t commandId, uint8_t sensorId, uint32_t data)
{
	uint8_t outputDataBuffer[6] = {0};
	outputDataBuffer[0] = PACKET_TYPE_MASTER_CONTROL;
	
	switch (commandId)
	{
		case COMMAND_ID_GET_FRAME:
		{
			outputDataBuffer[1] = PACKET_COMMAND_ID_GET_FRAME;
			outputDataBuffer[2] = sensorId;
			sendPacket(outputDataBuffer, 3);
		}
		break;
		case COMMAND_ID_UPDATE:
		{
			outputDataBuffer[1] = PACKET_COMMAND_ID_UPDATE;
			sendPacket(outputDataBuffer, 2);
		}
		break;
		case COMMAND_ID_SETUP_MODE:
		{
			outputDataBuffer[1] = PACKET_COMMAND_ID_SETUP_MODE;
			outputDataBuffer[2] = 0x01;
			sendPacket(outputDataBuffer, 3);
		}
		break;
		case COMMAND_ID_GET_STATUS:
		{
			outputDataBuffer[1] = PACKET_COMMAND_ID_GET_STATUS;
			outputDataBuffer[2] = 0x00;
			sendPacket(outputDataBuffer, 3);
		}
		break;
		case COMMAND_ID_ENABLE_HPR:
		{
			outputDataBuffer[1] = PACKET_COMMAND_ID_ENABLE_HPR;
			outputDataBuffer[2] = (uint8_t)data;
			sendPacket(outputDataBuffer, 3);
		}
		break;
		case COMMAND_ID_CHANGE_BAUD:
		{
			outputDataBuffer[1] = PACKET_COMMAND_ID_CHANGE_BAUD;
			//send out the baud rate in 8bits, LSB first (Little endian)
			outputDataBuffer[2] = (uint8_t)(data & 0x000000ff);
			outputDataBuffer[3] = (uint8_t)((data & 0x0000ff00) >> 8);
			outputDataBuffer[4] = (uint8_t)((data & 0x00ff0000) >> 16);
			outputDataBuffer[5] = (uint8_t)((data & 0xff000000) >> 24);
			sendPacket(outputDataBuffer, 6);
			vTaskDelay(10);	// give time to send out the command and the sensor to implement it
			// change local baud rate
			changeUartBaud(data);
		}
		break;
		case COMMAND_ID_CHANGE_PADDING:
		{
			outputDataBuffer[1] = PACKET_COMMAND_ID_CHANGE_PADDING;
			
			if (sensorPortConfig->mode == DRV_UART_MODE_DMA)		//padding is only used in interrupt driven DMA mode
			{
				outputDataBuffer[2] = (uint8_t)(data & 0x000000ff);			// padding enable
				outputDataBuffer[3] = (uint8_t)((data & 0x0000ff00) >> 8);	// padding length
			}
			else
			{
				outputDataBuffer[2] = FALSE;
				outputDataBuffer[3] = NULL;
			}
			
			sendPacket(outputDataBuffer, 4);
		}
		break;
		case COMMAND_ID_SET_RATES:
		break;
		case COMMAND_ID_SET_IMU_ID:
		break;
		#ifdef ENABLE_SENSORS_DEBUG_MODE
		case COMMAND_ID_RESET_FAKE:
		{
			sendResetCommandFake();
		}
		break;
		case COMMAND_ID_UPDATE_FAKE:
		{
			sendUpdateCommandFake();
		}
		#endif
		break;
		default:
		break;
	}
}

/*		OBSOLETE FUNCTIONS	
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

static void storePacket(pkt_rawPacket_t *packet)	// this function is obsolete as pointer manipulation is used to store data
{
	if (packet->payload[3] > 8)
	{
		return;	// the sensorId is not valid
	}
	uint8_t location = packet->payload[3] *35;		// 35 is the length of data from each sensor
	
	if (packet->payloadSize < 2)
	{
		return; // this is not a valid packet
	}
	
	if ((packet->payload[0] == PACKET_TYPE_IMU_SENSOR) && (packet->payload[0] == PACKET_COMMAND_ID_GET_FRAME_RESP))
	{
		// starting from the 3rd byte is sensor Id followed by sensor data, store it
		memcpy(&sensorFullFrame[location + 7], packet->payload[3], 35);		// we need first 7 bytes for header
		number_FramesReceived++;
	}
}
*/

/************************************************************************
 * changeSensorState(sensor_state_t sensorState)
 * @brief Change the global sensor state to the requested one
 * @param sensor_state_t sensorState - new sensor state
 * @return void                      
 ************************************************************************/
static void changeSensorState(sensor_state_t sensorState)
{
	if (sensorState != sgSensorState)
	{
		sgSensorState = sensorState;
		// If required add further actions here in a switch statement
	}
}

/************************************************************************
 * clearFullFrame()
 * @brief Clear the sensorFullFrame buffer to reuse it for new data
 * @param void
 * @return void                      
 ************************************************************************/
static void clearFullFrame()
{
	// clear the local structure holding the full frame of all sensors
	memset(&sensorFullFrame, 0x00, sizeof(sensorFullFrame));
	number_FramesReceived = 0;
}

/************************************************************************
 * sendFullFrame()
 * @brief Send all the data received and stored in sensorFullFrame to the 
 *		  data board.
 * @param void
 * @return void                      
 ************************************************************************/
static void sendFullFrame()
{
	uint32_t frameSize = 0;
	long sysTickCount = xTaskGetTickCount();
	uint32_t buffOffset = ((SEN_MAX_NUM_SENSORS - number_FramesReceived) * SEN_DEFAULT_DATA_SIZE);
	// send all sensors data to the data board
	if (xSemaphoreTake(semaphore_dataBoardUart, 1) == TRUE)
	{
		sensorFullFrame[buffOffset] = PACKET_TYPE_SUB_PROCESSOR;		// TODO: make this shit neat
		sensorFullFrame[buffOffset + 1] = PACKET_COMMAND_ID_SUBP_FULL_FRAME;
		sensorFullFrame[buffOffset + 2] = number_FramesReceived;
		// assign the sys tick, little endian
		sensorFullFrame[buffOffset + 3] = (sysTickCount >> 0) & 0xff;
		sensorFullFrame[buffOffset + 4] = (sysTickCount >> 8) & 0xff;
		sensorFullFrame[buffOffset + 5] = (sysTickCount >> 16) & 0xff;
		sensorFullFrame[buffOffset + 6] = (sysTickCount >> 24) & 0xff;
		// calculate the frame size, keep 7 bytes for the main frame header
		frameSize = (number_FramesReceived * SEN_DEFAULT_DATA_SIZE) + SEN_SENSOR_FULL_FRAME_HEADER_SIZE;
		// send the packet out.
		pkt_sendRawPacket(dataRouterConfiguration.dataBoardUart, &sensorFullFrame[buffOffset], frameSize);	// TODO: reduce the packet size according to the number of sensors present
		xSemaphoreGive(semaphore_dataBoardUart);
	}
}

/************************************************************************
 * sen_getSensorState()
 * @brief Returns the current sensor state.
 * @param void
 * @return sensor_state_t current sensor state                      
 ************************************************************************/
sensor_state_t sen_getSensorState(void)
{
	return sgSensorState;
}

/************************************************************************
 * sen_getDetectedSensors()
 * @brief Returns the detected sensor mask.
 * @param void
 * @return uint32_t current detected sensor mask                      
 ************************************************************************/
uint32_t sen_getDetectedSensors(void)
{
	return detectedSensorMask;
}

/************************************************************************
 * isExpectedSensorId(uint8_t byte, uint8_t expectedId)
 * @brief check if the byte is expected sensor id
 * @param void
 * @return status_t STATUS_PASS, STATUS_FAIL                      
 ************************************************************************/
static status_t isExpectedSensorId(uint8_t byte, uint8_t expectedId)
{
	if ((byte >= SEN_MIN_SENSOR_ID) && (byte <= SEN_MAX_SENSOR_ID))
	{
		if (byte == expectedId)
		{
			return STATUS_PASS;
		}
	}
	return STATUS_FAIL;
}

/************************************************************************
 * isSensorRequested(uint8_t sensorId)
 * @brief check if the sensorId is requested
 * @param void
 * @return status_t STATUS_PASS, STATUS_FAIL                      
 ************************************************************************/
static status_t isSensorRequested(uint8_t sensorId)
{
	if ((sensorId >= SEN_MIN_SENSOR_ID) && (sensorId <= SEN_MAX_SENSOR_ID))
	{
		if (((reqSensorMask >> sensorId) & 0x01)!= 0)
		{
			return STATUS_PASS;
		}
	}
	return STATUS_FAIL;
}

/************************************************************************
 * sen_enableSensorStream(bool enable)
 * @brief enable or disable the sensor stream
 * @param bool enable - 1 to enable and 0 to disable
 * @return void                     
 ************************************************************************/
// enable the sensor stream
void sen_enableSensorStream(bool enable)
{
	if (reqSensorMask != 0)
	{
		enableStream = enable;	// only enable the stream if any sensors are requested
		if (enableStream)
		{
			// reconfigure the baud rate (in case sensors were disconnected, default speed is LOW)
			changeUartBaud(SENSOR_BUS_SPEED_LOW);	// NOTE: these functions do not work with optimization.
			sendCommand(COMMAND_ID_CHANGE_BAUD, NULL, SENSOR_BUS_SPEED_HIGH);
		}
	}
}

/************************************************************************
 * sen_setConfig(uint8_t *data)
 * @brief Set the received sensor configuration
 * @param uint8_t *data: pointer to the data
 * @return void                     
 ************************************************************************/
void sen_setConfig(uint8_t *data)
{
	dataRate = data[0];
	reqSensorMask = data[1] | (data[2] << 8);	// we ignore the rest of the packet
	numReqSensors = 0;	// reset the number of requested sensor count
	// count the number of requested sensors
	for (int i = 0 ; i < SEN_MAX_NUM_SENSORS; i++)
	{
		if ((reqSensorMask >> i) & 0x01)
		{
			numReqSensors++;	// depicts the total number of sensors requested
		}
	}
}

/************************************************************************
 * sen_preSleepProcess()
 * @brief Disable the sensor stream and clear the reqSensor mask
 * @param uint8_t *data: pointer to the data
 * @return void                     
 ************************************************************************/
void sen_preSleepProcess()
{
	enableStream = false;
	reqSensorMask = 0;
}

/*	Debug commands	*/
#ifdef ENABLE_SENSORS_DEBUG_MODE
/************************************************************************
 * verifyFakePacket(pkt_rawPacketNew_t packet)
 * @brief Verify the integrity of the received debug packet
 * @param pkt_rawPacketNew_t packet: packet received from the sensor
 * @return status_t STATUS_PASS, STATUS_FAIL                     
 ************************************************************************/
static status_t verifyFakePacket(pkt_rawPacketVarSize_t packet)
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

/************************************************************************
 * sendResetCommandFake()
 * @brief Send the command to reset the debug counts on the sensor modules
 *        Also reset the local debug counts.
 * @param void
 * @return void                     
 ************************************************************************/
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

/************************************************************************
 * sendUpdateCommandFake()
 * @brief Send the command to update the debug counts on the sensor modules
 *        Also update the local debug counts.
 * @param void
 * @return void                     
 ************************************************************************/
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