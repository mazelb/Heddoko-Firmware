/*
 * cmd_commandProcessor.h
 *
 * Created: 3/21/2016 1:33:49 PM
 *  Author: sean
 */ 


#ifndef CMD_COMMANDPROCESSOR_H_
#define CMD_COMMANDPROCESSOR_H_
#include "pkt_packetParser.h"
#include "drv_i2c.h"
#define PACKET_TYPE_MASTER_CONTROL 0x01
#define PACKET_TYPE_IMU_SENSOR	   0x03

#define PACKET_COMMAND_ID_UPDATE			0x11
#define PACKET_COMMAND_ID_GET_FRAME			0x12
#define PACKET_COMMAND_ID_GET_FRAME_RESP	0x13
#define PACKET_COMMAND_ID_SETUP_MODE		0x14
#define PACKET_COMMAND_ID_BUTTON_PRESS		0x15
#define PACKET_COMMAND_ID_SET_IMU_ID		0x16
#define PACKET_COMMAND_ID_SET_IMU_ID_RESP	0x17
#define PACKET_COMMAND_ID_GET_STATUS		0x18
#define PACKET_COMMAND_ID_GET_STATUS_RESP	0x19

typedef struct  
{
	uint32_t statusMask;
	uint32_t receiveErrorCount;
	uint32_t quatReadErrorCount;
	uint32_t magReadErrorCount;
	uint32_t accelReadErrorCount;
	uint32_t gyroReadErrorCount;
}cmd_debugStructure_t;

int resetAndInitialize(slave_twi_config_t* slave_config);
void sendButtonPressEvent();
void cmd_processPacket(rawPacket_t* packet);



#endif /* CMD_COMMANDPROCESSOR_H_ */