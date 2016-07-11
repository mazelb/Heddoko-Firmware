/*
 * cmd_commandProcessor.h
 *
 * Created: 2016-04-18 12:27:20 PM
 *  Author: Hriday Mehta
 */ 


#ifndef CMD_COMMANDPROCESSOR_H_
#define CMD_COMMANDPROCESSOR_H_

#include "pkt_packetParser.h"

#define PACKET_TYPE_MASTER_CONTROL					0x01
#define PACKET_TYPE_POWER_BOARD						0x02	//TODO: verify the command
#define PACKET_TYPE_IMU_SENSOR						0x03
#define PACKET_TYPE_BLUETOOTH_MODULE				0x04

/*	IMU Sensor module commands	*/
#define PACKET_COMMAND_ID_UPDATE					0x11
#define PACKET_COMMAND_ID_GET_FRAME					0x12
#define PACKET_COMMAND_ID_GET_FRAME_RESP			0x13
#define PACKET_COMMAND_ID_SETUP_MODE				0x14
#define PACKET_COMMAND_ID_BUTTON_PRESS				0x15
#define PACKET_COMMAND_ID_SET_IMU_ID				0x16
#define PACKET_COMMAND_ID_SET_IMU_ID_RESP			0x17
#define PACKET_COMMAND_ID_GET_STATUS				0x18
#define PACKET_COMMAND_ID_GET_STATUS_RESP			0x19
#define PACKET_COMMAND_ID_RESET_FAKE				0x20
#define PACKET_COMMAND_ID_UPDATE_FAKE				0x21
#define PACKET_COMMAND_ID_ENABLE_HPR				0x22
#define PACKET_COMMAND_ID_CHANGE_BAUD				0x23
#define PACKET_COMMAND_ID_SET_RATES					0x24
#define PACKET_COMMAND_ID_CHANGE_PADDING			0x25

/*	Bluetooth module commands	*/
#define PACKET_COMMAND_ID_GPS_DATA_REQ              0x41
#define PACKET_COMMAND_ID_GPS_DATA_RESP             0x42
#define PACKET_COMMAND_ID_SSID_DATA_REQ             0x43
#define PACKET_COMMAND_ID_SSID_DATA_RESP            0x44
#define PACKET_COMMAND_ID_PASSPHRASE_DATA_REQ       0x45
#define PACKET_COMMAND_ID_PASSPHRASE_DATA_RESP      0x46
#define PACKET_COMMAND_ID_SECURITY_TYPE_DATA_REQ    0x47
#define PACKET_COMMAND_ID_SECURITY_TYPE_DATA_RESP   0x48
#define PACKET_COMMAND_ID_SEND_RAW_DATA_REQ         0x49
#define PACKET_COMMAND_ID_SEND_RAW_DATA_RESP        0x4a
#define PACKET_COMMAND_ID_GET_RAW_DATA_REQ          0x4b
#define PACKET_COMMAND_ID_GET_RAW_DATA_RESP         0x4c

typedef struct
{
	uint32_t statusMask;
	uint32_t receiveErrorCount;
	uint32_t quatReadErrorCount;
	uint32_t magReadErrorCount;
	uint32_t accelReadErrorCount;
	uint32_t gyroReadErrorCount;
}cmd_debugStructure_t;

void sendButtonPressEvent();
void cmd_processPacket(rawPacket_t* packet);
void protoPacketInit();
void sendProtoPacket();
void clearProtoPacket();


#endif /* CMD_COMMANDPROCESSOR_H_ */