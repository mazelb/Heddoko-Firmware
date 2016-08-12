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

uint32_t errorCount = 0;

//forward static function declarations
static void queueByte(uint8_t byte, uint16_t* index, uint8_t* destination);
static void queueByteWithEscape(uint8_t byte, uint16_t* index, uint8_t* destination);
static void sendByte(uint8_t byte, drv_uart_config_t* uartConfig);
static void sendByteWithEscape(uint8_t byte, drv_uart_config_t* uartConfig);

status_t pkt_serializeRawPacket(uint8_t* destination, size_t maxDestinationLength, uint16_t* destinationLength, uint8_t* payload, uint16_t payloadSize)
{
	int i = 0;
	status_t status = STATUS_PASS;	
	*destinationLength =0;
	//first send start byte
	queueByte(RAW_PACKET_START_BYTE,destinationLength,destination);
	//send the payload size
	queueByteWithEscape((uint8_t)(payloadSize&0x00ff),destinationLength,destination);
	queueByteWithEscape((uint8_t)((payloadSize>>8)&0x00ff),destinationLength,destination);
	for(i=0;i<payloadSize;i++)
	{
		//check that we have room in the destination buffer to store the byte.
		if(destinationLength > maxDestinationLength)
		{
			status = STATUS_FAIL;
		}
		queueByteWithEscape(payload[i],destinationLength,destination);
	}
	return status;
}

status_t pkt_sendRawPacket(drv_uart_config_t* uartConfig, uint8_t* payload, uint16_t payloadSize)
{
	int i = 0;
	status_t status = STATUS_PASS;
	//first send start byte
	sendByte(RAW_PACKET_START_BYTE,uartConfig);
	//send the payload size
	sendByteWithEscape((uint8_t)(payloadSize&0x00ff),uartConfig);
	sendByteWithEscape((uint8_t)((payloadSize>>8)&0x00ff),uartConfig);
	for(i=0;i<payloadSize;i++)
	{
		sendByteWithEscape(payload[i],uartConfig);
	}
	return status;
}

/***********************************************************************************************
 * drv_uart_getPacketTimed(drv_uart_config_t* uartConfig, pkt_rawPacket_t* packet, uint32_t maxTime)
 * @brief returns a raw packet that is processed successfully.
 * @param uartConfig the configuration structure for the uart
 * @param packet the pointer to the packet where the bytes will be stored
 * @param maxTime the maximum time in ticks the function should wait for the response. 
 * @return STATUS_PASS if a string is returned,	STATUS_FAIL if the string found is larger than the buffer, or timed out
 ***********************************************************************************************/	
status_t pkt_getPacketTimed(drv_uart_config_t* uartConfig, pkt_rawPacket_t* packet, uint32_t maxTime)
{
	status_t result = STATUS_PASS;
	char val;
	int pointer = 0;
	uint32_t startTime = xTaskGetTickCount();
	while(1) 
	{
		if(uartConfig->mode == DRV_UART_MODE_DMA)
		{		
			result = STATUS_FAIL;//uart_dma_getByte(uartConfig,&fifo_mem_block,&val);
		}
		else
		{
			result = drv_uart_getChar(uartConfig,&val);
		}
		if(result == STATUS_PASS)
		{
			//process the byte as it comes in
			if(pkt_processIncomingByte(packet,val) == STATUS_PASS)
			{
				//the packet is complete
				result = STATUS_PASS;
				break;
			}			
		}
		else
		{
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
	return result; 
}
status_t pkt_processIncomingByte(pkt_rawPacket_t* rawPacket, uint8_t byte)
{
	status_t status = STATUS_EAGAIN;
	//if byte is start byte
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(rawPacket->bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			//debugStructure.receiveErrorCount++;
			errorCount++; //not sure what this will do...
		}
		//reset the counts and everything for reception of the packet
		rawPacket->bytesReceived = 0;
		rawPacket->escapeFlag = false;
		rawPacket->payloadSize = 0;
		rawPacket->inError = false;
		return STATUS_EAGAIN;
	}
	//if byte is escape byte
	if(byte == RAW_PACKET_ESCAPE_BYTE)
	{
		//set escape flag, so the next byte is properly offset.
		rawPacket->escapeFlag = true;
		return STATUS_EAGAIN;
	}
	//if escape byte flag is set
	if(rawPacket->escapeFlag == true)
	{
		//un-escape the byte and process it as any other byte.
		byte = byte - RAW_PACKET_ESCAPE_OFFSET;
		//unset the flag
		rawPacket->escapeFlag = false;
	}
	
	//if receive count is  0
	if(rawPacket->bytesReceived == 0)
	{
		//this is the first byte of the payload size
		//copy byte to LSB of the payload size
		rawPacket->payloadSize |= (uint16_t)byte;
		//increment received count
		rawPacket->bytesReceived++;
	}
	else if(rawPacket->bytesReceived == 1)
	{
		//this is the second byte of the payload size
		//copy byte to MSB of the payload size
		rawPacket->payloadSize |= (uint16_t)(byte<<8);
		//increment received count
		rawPacket->bytesReceived++;
		if(rawPacket->payloadSize > RAW_PACKET_MAX_SIZE)
		{
			//set the error flag, something weird is going on...
			rawPacket->inError = true;			
			errorCount++;
			
		}
	}
	else
	{			
		//only process the bytes if not in error
		if(rawPacket->inError == false)
		{		
			//copy byte to payload at point receivedBytes - 2
			rawPacket->payload[rawPacket->bytesReceived - 2] = byte;
			//check if we received the whole packet.
			if(rawPacket->bytesReceived-1 == rawPacket->payloadSize)
			{
				//We have the packet!
				//set the return code to PASS to let the app know we have a packet. 
				status = STATUS_PASS;
				//reset everything to zero
				rawPacket->bytesReceived = 0;
			}
			else
			{
				rawPacket->bytesReceived++;
			}
		}
	}
	return status; 
	
}

//Static functions declarations

static void queueByte(uint8_t byte, uint16_t* index, uint8_t* destination)
{
	//send a byte function
	destination[(*index)++] = byte;
	return;
}
static void queueByteWithEscape(uint8_t byte, uint16_t* index, uint8_t* destination)
{
	if(byte == RAW_PACKET_START_BYTE || byte == RAW_PACKET_ESCAPE_BYTE)
	{
		queueByte(RAW_PACKET_ESCAPE_BYTE,index,destination);
		queueByte(byte + RAW_PACKET_ESCAPE_OFFSET,index,destination);
	}
	else
	{
		queueByte(byte,index,destination);
	}
}

static void sendByte(uint8_t byte, drv_uart_config_t* uartConfig)
{
	//send a byte function
	drv_uart_putChar(uartConfig, byte);
	return;
}
static void sendByteWithEscape(uint8_t byte, drv_uart_config_t* uartConfig)
{
	if(byte == RAW_PACKET_START_BYTE || byte == RAW_PACKET_ESCAPE_BYTE)
	{
		sendByte(RAW_PACKET_ESCAPE_BYTE,uartConfig);
		sendByte(byte + RAW_PACKET_ESCAPE_OFFSET,uartConfig);
	}
	else
	{
		sendByte(byte,uartConfig);
	}
}

status_t pkt_getPacketTimedNew(drv_uart_config_t* uartConfig, pkt_rawPacketNew_t* packet, uint32_t maxTime)
{
	status_t result = STATUS_PASS;
	char val;
	int pointer = 0;
	uint32_t startTime = xTaskGetTickCount();
	while(1)
	{
		if(uartConfig->mode == DRV_UART_MODE_DMA)
		{
			result = STATUS_FAIL;//uart_dma_getByte(uartConfig,&fifo_mem_block,&val);
		}
		else
		{
			result = drv_uart_getChar(uartConfig,&val);
		}
		if(result == STATUS_PASS)
		{
			//process the byte as it comes in
			if(pkt_processIncomingByteNew(packet,val) == STATUS_PASS)
			{
				//the packet is complete
				result = STATUS_PASS;
				break;
			}
		}
		else
		{
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
	return result;
}

status_t pkt_processIncomingByteNew(pkt_rawPacketNew_t* rawPacket, uint8_t byte)
{
	status_t status = STATUS_EAGAIN;
	//if byte is start byte
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(rawPacket->bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			//debugStructure.receiveErrorCount++;
			errorCount++; //not sure what this will do...
		}
		//reset the counts and everything for reception of the packet
		rawPacket->bytesReceived = 0;
		rawPacket->escapeFlag = false;
		rawPacket->payloadSize = 0;
		rawPacket->inError = false;
		return STATUS_EAGAIN;
	}
	//if byte is escape byte
	if(byte == RAW_PACKET_ESCAPE_BYTE)
	{
		//set escape flag, so the next byte is properly offset.
		rawPacket->escapeFlag = true;
		return STATUS_EAGAIN;
	}
	//if escape byte flag is set
	if(rawPacket->escapeFlag == true)
	{
		//un-escape the byte and process it as any other byte.
		byte = byte - RAW_PACKET_ESCAPE_OFFSET;
		//unset the flag
		rawPacket->escapeFlag = false;
	}
	
	//if receive count is  0
	if(rawPacket->bytesReceived == 0)
	{
		//this is the first byte of the payload size
		//copy byte to LSB of the payload size
		rawPacket->payloadSize |= (uint16_t)byte;
		//increment received count
		rawPacket->bytesReceived++;
	}
	else if(rawPacket->bytesReceived == 1)
	{
		//this is the second byte of the payload size
		//copy byte to MSB of the payload size
		rawPacket->payloadSize |= (uint16_t)(byte<<8);
		//increment received count
		rawPacket->bytesReceived++;
		if(rawPacket->payloadSize > RAW_PACKET_MAX_SIZE)
		{
			//set the error flag, something weird is going on...
			rawPacket->inError = true;
			errorCount++;
		}
	}
	else
	{
		//only process the bytes if not in error
		if(rawPacket->inError == false)
		{
			//copy byte to payload at point receivedBytes - 2
			rawPacket->p_payload[rawPacket->bytesReceived - 2] = byte;
			//check if we received the whole packet.
			if(rawPacket->bytesReceived-1 == rawPacket->payloadSize)
			{
				//We have the packet!
				//set the return code to PASS to let the app know we have a packet.
				status = STATUS_PASS;
				//reset everything to zero
				rawPacket->bytesReceived = 0;
			}
			else
			{
				rawPacket->bytesReceived++;
			}
		}
	}
	return status;
	
}