/*
 * pkt_packetParser.c
 *
 * Created: 2016-03-29 3:48:03 PM
 *  Author: Hriday Mehta
 */ 


#include <asf.h>
#include <string.h>
#include "pkt_packetParser.h"
#include "cmd_commandProcessor.h"
extern cmd_debugStructure_t debugStructure;

pkt_packetParserConfiguration_t *pktConfig;
uint8_t queuedPacket[pkt_MAX_QUEUED_PACKET_SIZE] = {0};
uint16_t queuedPacketIndex = 0;
uint32_t errorCount = 0;
volatile rawPacket_t packet =
{
	.bytesReceived = 0,
	.escapeFlag = false,
	.payloadSize = 0
};



void sendQueuedPacket()
{
	usart_write_buffer_wait(pktConfig->uartModule,&queuedPacket,queuedPacketIndex);
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
//

__attribute__((optimize("O0"))) void pkt_ProcessIncomingByte(uint8_t byte)
{
	//if byte is start byte
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(packet.bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			debugStructure.receiveErrorCount++;
		}
		//reset the counts and everything for reception of the packet
		packet.bytesReceived = 0;
		packet.escapeFlag = false;
		packet.payloadSize = 0;
		return;
	}
	//if byte is escape byte
	if(byte == RAW_PACKET_ESCAPE_BYTE)
	{
		//set escape flag, so the next byte is properly offset.
		packet.escapeFlag = true;
		return;
	}
	
	//if escape byte flag is set
	if(packet.escapeFlag == true)
	{
		//un-escape the byte and process it as any other byte.
		byte = byte - RAW_PACKET_ESCAPE_OFFSET;
		//unset the flag
		packet.escapeFlag = false;
	}
	
	//if receive count is  0
	if(packet.bytesReceived == 0)
	{
		//this is the first byte of the payload size
		//copy byte to LSB of the payload size
		packet.payloadSize |= (uint16_t)byte;
		//increment received count
		packet.bytesReceived++;
	}
	else if(packet.bytesReceived == 1)
	{
		//this is the second byte of the payload size
		//copy byte to MSB of the payload size
		packet.payloadSize |= (uint16_t)(byte<<8);
		//increment received count
		packet.bytesReceived++;
	}
	else
	{	//copy byte to payload at point receivedBytes - 2
		packet.payload[packet.bytesReceived - 2] = byte;
		//check if we received the whole packet.
		if(packet.bytesReceived-1 == packet.payloadSize)
		{
			//process the packet
			pktConfig->packetReceivedCallback(&packet);
			//reset everything to zero
			packet.bytesReceived = 0;
		}
		else
		{
			packet.bytesReceived++;
		}
	}
	
}
void pkt_packetParserInit(pkt_packetParserConfiguration_t* config)
{
	pktConfig = config;
}

