/*
 * pkt_packetParser.c
 *
 * Created: 3/21/2016 9:52:32 AM
 *  Author: sean
 */ 
#include <asf.h>
#include <string.h>
#include "pkt_packetParser.h"

uint32_t errorCount = 0;

//forward static function declarations
static void queueByte(uint8_t byte, uint16_t* index, uint8_t* destination);
static void queueByteWithEscape(uint8_t byte, uint16_t* index, uint8_t* destination);


status_t pkt_serializeRawPacket(uint8_t* destination, size_t maxDestinationLength, uint16_t* destinationLength, uint8_t* payload, uint16_t payloadSize)
{
	int i = 0;
	status_t status = STATUS_PASS;
	//TODO: set gpio for transmission mode on RS485
	
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
	}
	else
	{	//copy byte to payload at point receivedBytes - 2
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