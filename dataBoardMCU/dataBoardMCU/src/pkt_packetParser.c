/*
 * pkt_packetParser.c
 *
 * Created: 3/21/2016 9:52:32 AM
 *  Author: sean
 */ 

#include <asf.h>
#include <string.h>
#include "pkt_packetParser.h"
#include "drv_uart.h"
#include "imu.h"
#include "cmd_commandProcessor.h"
#include "sdc_sdCard.h"
#include "task_SensorHandler.h"

/*	Extern functions	*/
extern void changeSensorState(sensor_state_t sensorState);

/*	Extern variables	*/
extern drv_uart_config_t *consoleUart, *dataOutUart;

static bool enableRecording = FALSE;
bool enableStreaming = FALSE;
extern cmd_debugStructure_t debugStructure;
extern drv_uart_config_t uart0Config, uart1Config, usart1Config;
extern imuFrame_t imuFrameData;
extern sdc_file_t dataLogFile;

pkt_packetParserConfiguration_t *pktConfig;
uint8_t queuedPacket[pkt_MAX_QUEUED_PACKET_SIZE] = {0};
uint16_t queuedPacketIndex = 0;
uint32_t errorCount = 0;
bool packetState = FALSE;

uint8_t proto_queuedPacket[1000] = {0};
uint16_t proto_queuedPacketIndex = 0;
uint32_t proto_errorCount = 0;

void verifyPacket(rawPacket_t* packet)
{
	status_t status = STATUS_PASS;
	imuFrame_t* imuDataRx;
	imuDataRx = (imuFrame_t*) (&packet->payload[3]);
	if (imuDataRx->Acceleration_x != imuFrameData.Acceleration_x)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Acceleration_y != imuFrameData.Acceleration_y)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Acceleration_z != imuFrameData.Acceleration_z)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Magnetic_x != imuFrameData.Magnetic_x)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Magnetic_y != imuFrameData.Magnetic_y)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Magnetic_z != imuFrameData.Magnetic_z)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Rotation_x != imuFrameData.Rotation_x)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Rotation_y != imuFrameData.Rotation_y)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Rotation_z != imuFrameData.Rotation_z)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Quaternion_w != imuFrameData.Quaternion_w)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Quaternion_x != imuFrameData.Quaternion_x)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Quaternion_y != imuFrameData.Quaternion_y)
	{
		status |= STATUS_FAIL;
	}
	if (imuDataRx->Quaternion_z != imuFrameData.Quaternion_z)
	{
		status |= STATUS_FAIL;
	}
	if (status == STATUS_FAIL)
	{
		packetState = FALSE;
	}
	else
	{
		packetState = TRUE;
	}
}

void sendQueuedPacket()
{
	drv_uart_DMA_putData(pktConfig->uartModule, queuedPacket, queuedPacketIndex);
	drv_uart_DMA_wait_endTx(pktConfig->uartModule);
}

void sendByte(uint8_t byte)
{
	//send a byte function
	queuedPacket[queuedPacketIndex++] = byte;
	return;
}

void sendByteWithEscape(uint8_t byte)
{
	if(byte == RAW_PACKET_START_BYTE || byte == RAW_PACKET_ESCAPE_BYTE)
	{
		sendByte(RAW_PACKET_ESCAPE_BYTE);
		sendByte(byte + RAW_PACKET_ESCAPE_OFFSET);
	}
	else
	{
		sendByte(byte);
	}
}

void pkt_SendRawPacket(uint8_t* payload, uint16_t payloadSize)
{
	int i = 0;
	//TODO: set gpio for transmission mode on RS485
	queuedPacketIndex =0;
	//first send start byte
	sendByte(RAW_PACKET_START_BYTE);
	//send the payload size
	sendByteWithEscape((uint8_t)(payloadSize&0x00ff));
	sendByteWithEscape((uint8_t)((payloadSize>>8)&0x00ff));
	//indicate that it's a protobuf packet
	//sendByte(0x04);
	for(i=0;i<payloadSize;i++)
	{
		sendByteWithEscape(payload[i]);
	}
	if(pktConfig->transmitEnable != NULL)
	{
		(*(pktConfig->transmitEnable))();
	}
	
	sendQueuedPacket();
	
	//TODO: set gpio for receive mode on RS485
	if(pktConfig->transmitDisable != NULL)
	{
		(*(pktConfig->transmitDisable))();
	}
}

void sendCompletePacket()
{
	if (enableStreaming == TRUE)
	{
		drv_uart_DMA_putData(dataOutUart, proto_queuedPacket, proto_queuedPacketIndex);
		drv_uart_DMA_wait_endTx(dataOutUart);
		sdc_writeToFile(&dataLogFile, (void*)proto_queuedPacket, proto_queuedPacketIndex);
	}
	
	if (enableRecording == TRUE)
	{
		//sdc_writeToFile(&dataLogFile, (void*)proto_queuedPacket, proto_queuedPacketIndex);
	}
}

void sendByteProto(uint8_t byte)
{
	//send a byte function
	proto_queuedPacket[proto_queuedPacketIndex++] = byte;
	return;
}

void sendByteWithEscapeProto(uint8_t byte)
{
	if(byte == RAW_PACKET_START_BYTE || byte == RAW_PACKET_ESCAPE_BYTE)
	{
		sendByteProto(RAW_PACKET_ESCAPE_BYTE);
		sendByteProto(byte + RAW_PACKET_ESCAPE_OFFSET);
	}
	else
	{
		sendByteProto(byte);
	}
}

void pkt_SendRawPacketProto(uint8_t* payload, uint16_t payloadSize)
{
	int i = 0;
	//TODO: set gpio for transmission mode on RS485
	payloadSize += 1; //add one for indication that it's a protobuf file.
	proto_queuedPacketIndex =0;
	//first send start byte
	sendByteProto(RAW_PACKET_START_BYTE);
	//send the payload size
	sendByteWithEscapeProto((uint8_t)(payloadSize&0x00ff));
	sendByteWithEscapeProto((uint8_t)((payloadSize>>8)&0x00ff));
	//indicate that it's a protobuf packet
	sendByteProto(0x04);
	for(i=0;i<payloadSize-1;i++)
	{
	sendByteWithEscapeProto(payload[i]);
	}
	sendCompletePacket();
}

__attribute__((optimize("O0"))) status_t pkt_ProcessIncomingByte(uint8_t byte, rawPacket_t* packet)
{
	//if byte is start byte
	status_t status = STATUS_EAGAIN;
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(packet->bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			debugStructure.receiveErrorCount++;
			//status = STATUS_FAIL;
		}
		//reset the counts and everything for reception of the packet
		packet->bytesReceived = 0;
		packet->escapeFlag = false;
		packet->payloadSize = 0;
		return status;
	}
	//if byte is escape byte
	if(byte == RAW_PACKET_ESCAPE_BYTE)
	{
		//set escape flag, so the next byte is properly offset.
		packet->escapeFlag = true;
		return status;
	}
	//if escape byte flag is set
	if(packet->escapeFlag == true)
	{
		//un-escape the byte and process it as any other byte.
		byte = byte - RAW_PACKET_ESCAPE_OFFSET;
		//unset the flag
		packet->escapeFlag = false;
	}
	
	//if receive count is  0
	if(packet->bytesReceived == 0)
	{
		//this is the first byte of the payload size
		//copy byte to LSB of the payload size
		packet->payloadSize |= (uint16_t)byte;
		//increment received count
		packet->bytesReceived++;
	}
	else if(packet->bytesReceived == 1)
	{
		//this is the second byte of the payload size
		//copy byte to MSB of the payload size
		packet->payloadSize |= (uint16_t)(byte<<8);
		//increment received count
		packet->bytesReceived++;
	}
	else
	{	//copy byte to payload at point receivedBytes - 2
		packet->payload[packet->bytesReceived - 2] = byte;
		//check if we received the whole packet.
		if(packet->bytesReceived-1 == packet->payloadSize)
		{
			//set the status to pass, we have the packet!
			status = STATUS_PASS;
			//verifyPacket(packet);
			pktConfig->packetReceivedCallback(packet);
			//reset everything to zero
			packet->bytesReceived = 0;
		}
		else
		{
			packet->bytesReceived++;
		}
	}
	return status;	
}

status_t pkt_getRawPacket(rawPacket_t* packet, uint32_t maxTime, uint8_t minSize)
{
	status_t result = STATUS_EAGAIN;
	char val;
	int pointer = 0;
	uint32_t startTime = xTaskGetTickCount();
	uint16_t numBytesRead = 0;
	
	numBytesRead = drv_uart_DMA_readRxBytes(pktConfig->uartModule);
	
	if (minSize == 0)
	{
		minSize = numBytesRead; //IF size is not specified then process whatever is received
	}
	
	if(drv_uart_DMA_readRxBytes(pktConfig->uartModule) >= minSize)
	{
		while(result == STATUS_EAGAIN) //TODO add timeout
		{
			result = drv_uart_DMA_getChar(pktConfig->uartModule, &val);
			if(result != STATUS_EOF)
			{
				if(pointer < RAW_PACKET_MAX_SIZE)
				{
					result = pkt_ProcessIncomingByte(val, packet);
					pointer++;
				}
				else
				{
					//we overwrote the buffer
					result = STATUS_FAIL;
					break;
				}
			}
			else	//TODO: this section is obsolete, the function never returns STATUS_EOF
			{
				result = STATUS_EAGAIN; //check again
				//check if we've timed out yet...
				if(xTaskGetTickCount() > (startTime + maxTime))
				{
					//return fail, we've timed out.
					result = STATUS_FAIL;
					break;
				}
				vTaskDelay(1); //let the other processes do stuff
			}
			
		}
		//Fetching complete, check the integrity of data
		if (result != STATUS_PASS)
		{
			if (packetState == TRUE)
			{
				//puts("Good, \r");
			}
			else
			{
				//puts("Bad, \r");
			}
			//printf("Err Cnt: %d, , %d\r\n", debugStructure.receiveErrorCount, packet->bytesReceived);
		}
		else
		{
			changeSensorState(SENSOR_READY);
		}
	}
	//Incomplete packet: bytes received are less than minimum
	else
	{
		//printf("Sensor disconnected, size: %d\r\n", drv_uart_DMA_readRxBytes(pktConfig->uartModule));
		if (drv_uart_DMA_readRxBytes(pktConfig->uartModule) == 0)
		{
			changeSensorState(SENSOR_NOT_PRESENT);
		}
		else
		{
			changeSensorState(SENSOR_COMM_ERROR);
		}
		result = STATUS_FAIL;
	}
	return result;
}

void pkt_packetParserInit(pkt_packetParserConfiguration_t* config)
{
	pktConfig = config;
}

void enableSdWrite(bool state)
{
	enableRecording = state;
}