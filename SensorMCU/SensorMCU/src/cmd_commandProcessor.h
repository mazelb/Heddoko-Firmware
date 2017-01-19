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

#define PACKET_COMMAND_ID_UPDATE				0x11
#define PACKET_COMMAND_ID_GET_FRAME				0x12
#define PACKET_COMMAND_ID_GET_FRAME_RESP		0x13
#define PACKET_COMMAND_ID_SETUP_MODE			0x14
#define PACKET_COMMAND_ID_BUTTON_PRESS			0x15
#define PACKET_COMMAND_ID_SET_IMU_ID			0x16
#define PACKET_COMMAND_ID_SET_IMU_ID_RESP		0x17
#define PACKET_COMMAND_ID_GET_STATUS			0x18
#define PACKET_COMMAND_ID_GET_STATUS_RESP		0x19

#define PACKET_COMMAND_ID_UPDATE_CONFIG_PARAM	0x1A
#define PACKET_COMMAND_ID_GET_CONFIG_PARAM		0x1B
#define PACKET_COMMAND_ID_GET_CONFIG_PARAM_RESP	0x1C
#define PACKET_COMMAND_ID_UPDATE_WARMUP_PARAM	0x1D
#define PACKET_COMMAND_ID_GET_WARMUP_PARAM		0x1E
#define PACKET_COMMAND_ID_GET_WARMUP_PARAM_RESP	0x1F

#define PACKET_COMMAND_ID_RESET_FAKE			0x20	
#define PACKET_COMMAND_ID_UPDATE_FAKE			0x21
#define PACKET_COMMAND_ID_ENABLE_HPR			0x22
#define PACKET_COMMAND_ID_CHANGE_BAUD			0x23
#define PACKET_COMMAND_ID_SET_RATES				0x24
#define PACKET_COMMAND_ID_SET_WARMUP_PARAM		0x25
#define PACKET_COMMAND_ID_SET_RANGE_PARAM		0x26
#define PACKET_COMMAND_ID_SET_CONFIG			0x27
#define PACKET_COMMAND_ID_GET_CONFIG			0x28
#define PACKET_COMMAND_ID_GET_CONFIG_RESP		0x29
#define PACKET_COMMAND_ID_SAVE_TO_NVM			0x2A

#define PACKET_COMMAND_ID_TOGGLE_PASSTHROUGH    0x2B
#define PACKET_COMMAND_ID_READ_EEPROM_PACKET    0x2C
#define PACKET_COMMAND_ID_EEPROM_PACKET			0x2D
#define PACKET_COMMAND_ID_WRITE_EEPROM_PACKET   0x2E
#define PACKET_COMMAND_ID_WRITE_EEPROM_RESP		0x2F


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
void sendGetStatusResponse();
void togglePassthrough(uint8_t enable);
void getEepromPacket(uint16_t address);
void cmd_processPacket(rawPacket_t* packet);




#endif /* CMD_COMMANDPROCESSOR_H_ */