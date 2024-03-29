/**
* @file pkt_packetParser.h
* @brief Processor for incoming raw packets
* @author Sean Cloghesy
* @date March 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/

#ifndef PKT_PACKETPARSER_H_
#define PKT_PACKETPARSER_H_
#include "common.h"
#include "drv_uart.h"
#include "pkt_packetCommandsList.h"
#define RAW_PACKET_MAX_SIZE 1024 //TODO will have to allocate this memory dynamically.
#define RAW_PACKET_START_BYTE 0xDE
#define RAW_PACKET_ESCAPE_BYTE 0xDF
#define RAW_PACKET_ESCAPE_OFFSET 0x10

typedef struct
{
	volatile uint8_t payload[RAW_PACKET_MAX_SIZE];
	uint16_t payloadSize; //size of the payload
	uint16_t bytesReceived; //number of bytes received
	bool escapeFlag; //flag indicating the next byte was escaped
	bool inError;
}pkt_rawPacket_t;
//typedef void (*voidFunction_t)(void);
//typedef void (*packetCallback_t)(rawPacket_t* packet);


status_t pkt_serializeRawPacket(uint8_t* destination, size_t maxDestinationLength, uint16_t* destinationLength, uint8_t* payload, uint16_t payloadSize);
status_t pkt_sendRawPacket(drv_uart_config_t* uartConfig, uint8_t* payload, uint16_t payloadSize);
status_t pkt_getPacketTimed(drv_uart_config_t* uartConfig, pkt_rawPacket_t* packet, uint32_t maxTime);
status_t pkt_processIncomingByte(pkt_rawPacket_t* rawPacket, uint8_t byte);


#endif /* PKT_PACKETPARSER_H_ */