/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include "pkt_packetParser.h"

pkt_packetParserConfiguration_t *pktConfig;
uint8_t queuedPacket[1000] = {0};
uint16_t queuedPacketIndex = 0;
uint32_t errorCount = 0;
bool enableStreaming = false;


void sendCompletePacket()
{
//	if (enableStreaming == true)
//	{
        UART_SpiUartPutArray(queuedPacket, (uint32) queuedPacketIndex);
//	}
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
	sendCompletePacket();
}

status_t pkt_ProcessIncomingByte(uint8_t byte, rawPacket_t* packet)
{
	//if byte is start byte
	status_t status = STATUS_EAGAIN;
    
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(packet->bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			//receiveErrorCount++;
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
	uint32 startTime = CySysTickGetValue();     //TODO: seems to be generating interrupt not at every 1ms.
	uint16_t numBytesRead = 0, numBytesProcessed = 0;
    bool value = false;
	
	numBytesRead = UART_SpiUartGetRxBufferSize();
    
    if(numBytesRead == 0)
    {
        return STATUS_FAIL;
    }
    
	if (minSize == 0)
    {
        minSize = numBytesRead; //IF size is not specified then process whatever is received
    }
    
	if(numBytesRead >= minSize)
	{
		while ((result == STATUS_EAGAIN) && (numBytesProcessed <= numBytesRead))  // //
		{
            if (CySysTickGetValue() - startTime <= maxTime)
            {
                break;  //return if elasped time reaches maxTime
            }
            
            CyDelay(1);
            
			val = (char) UART_UartGetByte();
            numBytesProcessed++;
			if(pointer < RAW_PACKET_MAX_SIZE)
			{
				result = pkt_ProcessIncomingByte(val,packet);
				pointer++;
			}
			else
			{
				//we overwrote the buffer
				result = STATUS_FAIL;
				break;
			}
		}
	}
	//Incomplete packet: bytes received are less than minimum
	else
	{
		if (numBytesRead == 0)
		{
			UART_UartPutString("No data received\r\n");
		}
		else
		{
			UART_UartPutString("Incomplete data received\r\n");
		}
		result = STATUS_FAIL;
	}
   // UART_SpiUartClearRxBuffer();
	return result;
}

void pkt_packetParserInit(pkt_packetParserConfiguration_t* config)
{
	pktConfig = config;
}

/* [] END OF FILE */
