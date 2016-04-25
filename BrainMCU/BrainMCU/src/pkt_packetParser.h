/*
 * pkt_packetParser.h
 *
 * Created: 2016-03-29 3:44:14 PM
 *  Author: Hriday Mehta
 */ 


#ifndef PKT_PACKETPARSER_H_
#define PKT_PACKETPARSER_H_


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
	struct usart_module* uartModule;
}pkt_packetParserConfiguration_t;


__attribute__((optimize("O0"))) void pkt_ProcessIncomingByte(uint8_t byte);
void pkt_SendRawPacket(uint8_t* payload, uint16_t payloadSize);
void pkt_packetParserInit(pkt_packetParserConfiguration_t* config);


#endif /* PKT_PACKETPARSER_H_ */