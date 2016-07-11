/*
 * pkt_packetParser.h
 *
 * Created: 3/21/2016 9:57:17 AM
 *  Author: sean
 */ 


#ifndef pkt_PACKETPARSER_H_
#define pkt_PACKETPARSER_H_

#include "common.h"

#define pkt_MAX_QUEUED_PACKET_SIZE 100
#define RAW_PACKET_MAX_SIZE 200
#define RAW_PACKET_START_BYTE 0xDE
#define RAW_PACKET_ESCAPE_BYTE 0xDF
#define RAW_PACKET_ESCAPE_OFFSET 0x10

typedef struct
{
	volatile uint8_t payload[RAW_PACKET_MAX_SIZE];
	uint16_t payloadSize; //size of the payload
	uint16_t bytesReceived; //number of bytes received
	bool escapeFlag; //flag indicating the next byte was escaped
}rawPacket_t;

typedef void (*voidFunction_t)(void);
typedef void (*packetCallback_t)(rawPacket_t* packet);

typedef struct
{
	voidFunction_t transmitDisable;
	voidFunction_t transmitEnable;
	packetCallback_t packetReceivedCallback;
	rawPacket_t packet;
}pkt_packetParserConfiguration_t;

void pkt_bufferFull(bool bufferIndex, uint8_t comPortIndex);
void pkt_SendRawPacket(uint8_t* payload, uint16_t payloadSize, uint8_t comPortIndex);
void pkt_SendRawPacketProto(uint8_t* payload, uint16_t payloadSize, uint8_t comPortIndex);
void pkt_packetParserInit(pkt_packetParserConfiguration_t* config, uint8_t comPortIndex);
status_t pkt_getRawPacket(uint32_t maxTime, uint8_t minSize, uint8_t comPortIndex);
void enableSdWrite(bool state);
void pkt_registerPacketAndCallback(pkt_packetParserConfiguration_t* config, rawPacket_t* packet, packetCallback_t callBack);
void pkt_ParserTask(void *pvParameters);

#endif /* pkt_PACKETPARSER_H_ */