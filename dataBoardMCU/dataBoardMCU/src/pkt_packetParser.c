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
#include "sys_systemManager.h"

/*	Static function forward declarations	*/
//static __attribute__((optimize("O0"))) status_t pkt_ProcessIncomingByteFromUart(uint8_t byte, drv_uart_config_t* p_comPortConfig);
static __attribute__((optimize("O0"))) status_t pkt_ProcessIncomingByte(uint8_t byte, uint8_t comPortIndex);

/*	Extern functions	*/
extern void changeSensorState(sensor_state_t sensorState);

/*	Extern variables	*/
extern system_port_config_t sys_comPorts;
extern drv_uart_config_t *p_uartConfig[MAX_NUM_OF_UARTS];
extern cmd_debugStructure_t debugStructure;
extern imuFrame_t imuFrameData;
extern sdc_file_t dataLogFile;
	
/*	Local variables	*/
static bool enableRecording = FALSE;
bool enableStreaming = FALSE;
struct  
{
	rawPacket_t *p_packet;
	packetCallback_t callBackFunction;
}localPackets[MAX_NUM_OF_UARTS];		//array of pointers to external packets and respective call back functions. used in DMA mode

pkt_packetParserConfiguration_t *pktConfig[MAX_NUM_OF_UARTS];
uint8_t queuedPacket[pkt_MAX_QUEUED_PACKET_SIZE] = {0};
uint16_t queuedPacketIndex = 0;
uint32_t errorCount = 0;
bool packetState = FALSE;
bool bufferChanged[MAX_NUM_OF_UARTS] = {FALSE};
bool previousBuffer[MAX_NUM_OF_UARTS] = {UART_DMA_BUFFER_A};

uint8_t proto_queuedPacket[1000] = {0};
uint16_t proto_queuedPacketIndex = 0;
uint32_t proto_errorCount = 0;

/*	Function definitions	*/

void pkt_bufferFull(bool bufferIndex, uint8_t comPortIndex)
{
	uint8_t uartIdx = 3, byte;
	//bufferChanged[p_comPortConfig->mem_index] = TRUE;
	ioport_set_pin_level(LED_0_PIN, LED_0_ACTIVE);
	previousBuffer[comPortIndex] = bufferIndex;
	
	if (previousBuffer[uartIdx] == UART_DMA_BUFFER_A)
	{
		for (int charIdx = 0; charIdx < p_uartConfig[uartIdx]->dma_bufferDepth; charIdx++)
		{
			drv_uart_DMA_getChar(p_uartConfig[uartIdx], &byte, UART_DMA_BUFFER_A);
			pkt_ProcessIncomingByte(byte, comPortIndex);
		}
		drv_uart_DMA_reInit(p_uartConfig[comPortIndex], UART_DMA_BUFFER_A);
	}
	else
	{
		for (int charIdx = 0; charIdx < p_uartConfig[uartIdx]->dma_bufferDepth; charIdx++)
		{
			drv_uart_DMA_getChar(p_uartConfig[uartIdx], &byte, UART_DMA_BUFFER_B);
			pkt_ProcessIncomingByte(byte, comPortIndex);
		}
		drv_uart_DMA_reInit(p_uartConfig[comPortIndex], UART_DMA_BUFFER_B);
	}
	ioport_set_pin_level(LED_0_PIN, LED_0_INACTIVE);
}

void verifyPacket(rawPacket_t* packet)
{
	status_t status = STATUS_PASS;
	imuFrame_t* imuDataRx;
	imuDataRx = (imuFrame_t*) (&packet->payload[3]);
	if ((imuDataRx->Acceleration_x != imuFrameData.Acceleration_x) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Acceleration_y != imuFrameData.Acceleration_y) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Acceleration_z != imuFrameData.Acceleration_z) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Magnetic_x != imuFrameData.Magnetic_x) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Magnetic_y != imuFrameData.Magnetic_y) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Magnetic_z != imuFrameData.Magnetic_z) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Rotation_x != imuFrameData.Rotation_x) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Rotation_y != imuFrameData.Rotation_y) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Rotation_z != imuFrameData.Rotation_z) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Quaternion_w != imuFrameData.Quaternion_w) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Quaternion_x != imuFrameData.Quaternion_x) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Quaternion_y != imuFrameData.Quaternion_y) && (status != STATUS_FAIL))
	{
		status |= STATUS_FAIL;
	}
	if ((imuDataRx->Quaternion_z != imuFrameData.Quaternion_z) && (status != STATUS_FAIL))
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

void sendQueuedPacket(uint8_t comPortIndex)
{
	//drv_uart_DMA_putData(pktConfig->uartModule, queuedPacket, queuedPacketIndex);
	//drv_uart_DMA_wait_endTx(pktConfig->uartModule);
	drv_uart_putData(p_uartConfig[comPortIndex], queuedPacket, queuedPacketIndex);
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

void pkt_SendRawPacket(uint8_t* payload, uint16_t payloadSize, uint8_t comPortIndex)
{
	int i = 0;
	//TODO: set gpio for transmission mode on RS485
	queuedPacketIndex = 0;
	//first send start byte
	sendByte(RAW_PACKET_START_BYTE);
	//send the payload size
	sendByteWithEscape((uint8_t)(payloadSize & 0x00ff));
	sendByteWithEscape((uint8_t)((payloadSize>>8) & 0x00ff));
	//indicate that it's a protobuf packet
	//sendByte(0x04);
	for(i=0;i<payloadSize;i++)
	{
		sendByteWithEscape(payload[i]);
	}
	if(p_uartConfig[comPortIndex]->pktConfig.transmitEnable != NULL)
	{
		(*(p_uartConfig[comPortIndex]->pktConfig.transmitEnable))();
	}
	
	sendQueuedPacket(comPortIndex);
	
	//TODO: set gpio for receive mode on RS485
	if(p_uartConfig[comPortIndex]->pktConfig.transmitDisable != NULL)
	{
		(*(p_uartConfig[comPortIndex]->pktConfig.transmitDisable))();
	}
}

void sendCompletePacket(uint8_t comPortIndex)
{
	if (enableStreaming == TRUE)
	{
		drv_uart_putData(p_uartConfig[comPortIndex], proto_queuedPacket, proto_queuedPacketIndex);
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

void pkt_SendRawPacketProto(uint8_t* payload, uint16_t payloadSize, uint8_t comPortIndex)
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
	sendCompletePacket(comPortIndex);
}

static __attribute__((optimize("O0"))) status_t pkt_ProcessIncomingByte(uint8_t byte, uint8_t comPortIndex)
{
	//if byte is start byte
	status_t status = STATUS_EAGAIN;
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			debugStructure.receiveErrorCount++;
			//status = STATUS_FAIL;
		}
		//reset the counts and everything for reception of the packet
		p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived = 0;
		p_uartConfig[comPortIndex]->pktConfig.packet.escapeFlag = false;
		p_uartConfig[comPortIndex]->pktConfig.packet.payloadSize = 0;
		return status;
	}
	//if byte is escape byte
	if(byte == RAW_PACKET_ESCAPE_BYTE)
	{
		//set escape flag, so the next byte is properly offset.
		p_uartConfig[comPortIndex]->pktConfig.packet.escapeFlag = true;
		return status;
	}
	//if escape byte flag is set
	if(p_uartConfig[comPortIndex]->pktConfig.packet.escapeFlag == true)
	{
		//un-escape the byte and process it as any other byte.
		byte = byte - RAW_PACKET_ESCAPE_OFFSET;
		//unset the flag
		p_uartConfig[comPortIndex]->pktConfig.packet.escapeFlag = false;
	}
	
	//if receive count is  0
	if(p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived == 0)
	{
		//this is the first byte of the payload size
		//copy byte to LSB of the payload size
		p_uartConfig[comPortIndex]->pktConfig.packet.payloadSize |= (uint16_t)byte;
		//increment received count
		p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived++;
	}
	else if(p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived == 1)
	{
		//this is the second byte of the payload size
		//copy byte to MSB of the payload size
		p_uartConfig[comPortIndex]->pktConfig.packet.payloadSize |= (uint16_t)(byte<<8);
		//increment received count
		p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived++;
	}
	else
	{	//copy byte to payload at point receivedBytes - 2
		p_uartConfig[comPortIndex]->pktConfig.packet.payload[p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived - 2] = byte;
		//check if we received the whole packet.
		if(p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived-1 == p_uartConfig[comPortIndex]->pktConfig.packet.payloadSize)
		{
			//set the status to pass, we have the packet!
			status = STATUS_PASS;
			#ifdef ENABLE_SENSOR_PACKET_TEST
			verifyPacket(p_uartConfig[comPortIndex]->packet);
			#endif
			p_uartConfig[comPortIndex]->pktConfig.packetReceivedCallback(&p_uartConfig[comPortIndex]->pktConfig.packet);
			//reset everything to zero
			p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived = 0;
		}
		else
		{
			p_uartConfig[comPortIndex]->pktConfig.packet.bytesReceived++;
		}
	}
	return status;	
}

status_t pkt_getRawPacket(uint32_t maxTime, uint8_t minSize, uint8_t comPortIndex)
{
	status_t result = STATUS_EAGAIN;
	char val;
	int pointer = 0;
	uint32_t startTime = xTaskGetTickCount();
	uint16_t numBytesRead = 0;
	
	numBytesRead = drv_uart_DMA_readRxBytes(p_uartConfig[comPortIndex]);
	
	if (minSize == NULL)
	{
		minSize = numBytesRead; //IF size is not specified then process whatever is received
	}
	
	if(numBytesRead >= minSize)
	{
		while(result == STATUS_EAGAIN) //TODO add timeout
		{
			result = drv_uart_DMA_getChar(p_uartConfig[comPortIndex], &val, UART_DMA_BUFFER_A);
			if(result != STATUS_EOF)
			{
				if(pointer < RAW_PACKET_MAX_SIZE)
				{
					result = pkt_ProcessIncomingByte(val, comPortIndex);
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
		if (result == STATUS_PASS)
		{
			//changeSensorState(SENSOR_READY);
		}
		else
		{
			#ifdef ENABLE_SENSOR_PACKET_TEST
			if (packetState == TRUE)
			{
				puts("Good, InCmp \r");		//packet is incomplete but the data is good (if it ends up here it's not planet EARTH !!!)
			}
			else
			{
				puts("Bad,  InCmp \r");		//packet is incomplete and the data is bad.
			}
			//printf("Err Cnt: %d, , %d\r\n", debugStructure.receiveErrorCount, packet->bytesReceived);
			#endif
		}
	}
	//Incomplete packet: bytes received are less than minimum
	else
	{
		//printf("Sensor disconnected, size: %d\r\n", drv_uart_DMA_readRxBytes(pktConfig->uartModule));
		if (numBytesRead == 0)
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

void pkt_packetParserInit(pkt_packetParserConfiguration_t* config, uint8_t comPortIndex)
{
	// NOTE: the packerParser config can be overwritten, no check for null
	pktConfig[comPortIndex] = config;
}

void enableSdWrite(bool state)
{
	enableRecording = state;
}

/*	Interrupt driven mode functions	*/
/*
void pkt_registerPacketAndCallback(pkt_packetParserConfiguration_t* config, rawPacket_t* packet, packetCallback_t callBack)
{
	//localPackets[config->uartModule->mem_index].p_packet = packet;
	//localPackets[config->uartModule->mem_index].callBackFunction = callBack;
}

static __attribute__((optimize("O0"))) status_t pkt_ProcessIncomingByteFromUart(uint8_t byte, drv_uart_config_t* p_comPortConfig)
{
	//if byte is start byte
	status_t status = STATUS_EAGAIN;
	
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(p_comPortConfig->pktConfig.packet.bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			debugStructure.receiveErrorCount++;
			//printf("Receive error count: %d\r\n", debugStructure.receiveErrorCount);
			//status = STATUS_FAIL;
		}
		//reset the counts and everything for reception of the packet
		p_comPortConfig->pktConfig.packet.bytesReceived = 0;
		p_comPortConfig->pktConfig.packet.escapeFlag = false;
		p_comPortConfig->pktConfig.packet.payloadSize = 0;
		return status;
	}
	//if byte is escape byte
	if(byte == RAW_PACKET_ESCAPE_BYTE)
	{
		//set escape flag, so the next byte is properly offset.
		p_comPortConfig->pktConfig.packet.escapeFlag = true;
		return status;
	}
	//if escape byte flag is set
	if(p_comPortConfig->pktConfig.packet.escapeFlag == true)
	{
		//un-escape the byte and process it as any other byte.
		byte = byte - RAW_PACKET_ESCAPE_OFFSET;
		//unset the flag
		p_comPortConfig->pktConfig.packet.escapeFlag = false;
	}
	
	//if receive count is  0
	if(p_comPortConfig->pktConfig.packet.bytesReceived == 0)
	{
		//this is the first byte of the payload size
		//copy byte to LSB of the payload size
		p_comPortConfig->pktConfig.packet.payloadSize |= (uint16_t)byte;
		//increment received count
		p_comPortConfig->pktConfig.packet.bytesReceived++;
	}
	else if(p_comPortConfig->pktConfig.packet.bytesReceived == 1)
	{
		//this is the second byte of the payload size
		//copy byte to MSB of the payload size
		p_comPortConfig->pktConfig.packet.payloadSize |= (uint16_t)(byte<<8);
		//increment received count
		p_comPortConfig->pktConfig.packet.bytesReceived++;
	}
	else
	{	//copy byte to payload at point receivedBytes - 2
		p_comPortConfig->pktConfig.packet.payload[p_comPortConfig->pktConfig.packet.bytesReceived - 2] = byte;
		//check if we received the whole packet.
		if(p_comPortConfig->pktConfig.packet.bytesReceived-1 == p_comPortConfig->pktConfig.packet.payloadSize)
		{
			//set the status to pass, we have the packet!
			status = STATUS_PASS;
			#ifdef ENABLE_SENSOR_PACKET_TEST
			verifyPacket(p_comPortConfig->pktConfig.packet);
			#endif
			p_comPortConfig->pktConfig.packetReceivedCallback(p_comPortConfig->pktConfig.packet);
			//reset everything to zero
			p_comPortConfig->pktConfig.packet.bytesReceived = 0;
		}
		else
		{
			p_comPortConfig->pktConfig.packet.bytesReceived++;
		}
	}
	return status;
}
*/