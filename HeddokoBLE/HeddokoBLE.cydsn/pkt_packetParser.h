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

#ifndef pkt_PACKETPARSER_H
#define pkt_PACKETPARSER_H

#include <project.h>
#include "stdbool.h"
#include "main.h"

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
	packetCallback_t packetReceivedCallback;
}pkt_packetParserConfiguration_t;

status_t pkt_ProcessIncomingByte(uint8_t byte, rawPacket_t* packet);
void pkt_SendRawPacket(uint8_t* payload, uint16_t payloadSize);
void pkt_packetParserInit(pkt_packetParserConfiguration_t* config);
status_t pkt_getRawPacket(rawPacket_t* packet, uint32_t maxTime, uint8_t minSize);
void enableSdWrite(bool state);
    
#endif

/* [] END OF FILE */
