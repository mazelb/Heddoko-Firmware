/*
 * task_quinticInterface.h
 *
 * Created: 9/18/2015 10:50:03 AM
 *  Author: Sean Cloghesy
 * Copyright Heddoko(TM) 2015, all rights reserved 
 */ 


#ifndef TASK_QUINTIC_INTERFACE_H_
#define TASK_QUINTIC_INTERFACE_H_

#include "common.h"
#include "drv_uart.h"
#include "drv_gpio.h"

#define IMU_MAC_ADDRESS_LENGTH 20 //plus 1 for termination 
#define IMU_PACKET_LENGTH 64
#define IMU_BUFFER_SIZE 10
#define QUINTIC_MAX_SCAN_ATTEMPTS 4
#define QUINTIC_MAX_NUMBER_OF_IMUS 5


#define CMD_RESPONSE_BUF_SIZE						  255

//Commands
#define QCMD_QN_ACK "QnAck\r\n"
#define QCMD_ACK "Ack\r\n"
#define QCMD_BEGIN "begin\r\n"
#define QCMD_SCAN "scan\r\n"
#define QCMD_SCAN_ACK "QnScanAck\r\n"
#define QCMD_CONN_ACK "QnConAck\r\n"
#define QCMD_CONNECT "connect\r\n"
#define QCMD_START "start\r\n"
#define QCMD_STOP "stop\r\n"
#define QCMD_CHECK "check\r\n"

typedef struct  
{
	uint32_t packetCnt; 
	uint32_t droppedPackets; 
	uint32_t lastPacketTime; 
	uint32_t maxPacketTime;
	uint32_t avgPacketTime;
}imuStatistics_t;

typedef struct
{
	int imuId; 
	char macAddress[IMU_MAC_ADDRESS_LENGTH]; //stored as ASCII
	//char packetBuffer[IMU_BUFFER_SIZE][IMU_PACKET_LENGTH]; 
	//int bufferHead;
	//int bufferEnd; 
	int imuValid; //indicates whether current imu should be used. 
	int imuPresent; 
	int imuConnected; 
	imuStatistics_t stats; 	 		
}imuConfiguration_t;

typedef struct 
{
	imuConfiguration_t* imuArray[QUINTIC_MAX_NUMBER_OF_IMUS]; //array that stores the configuration/data for each NOD
	drv_uart_config_t* uartDevice; //pointer to uart driver that's used by the interface, must already be initialized
	int expectedNumberOfNods; 	
	uint32_t corruptPacketCnt; //how many packets it's received that were corrupt
	drv_gpio_pins_t resetPin; 
	int isinit; 
	int qId;
	xTaskHandle taskHandle;
	char imuMask[9]; 
}quinticConfiguration_t;

//function declarations
void task_quinticHandler(void *pvParameters);
void task_quintic_initializeImus(void *pvParameters);
status_t checkConnectedImus(quinticConfiguration_t* qConfig);
status_t task_quintic_checkRssiLevel(quinticConfiguration_t* qConfig);
status_t task_quintic_sendConnectMsg(quinticConfiguration_t* qConfig);
status_t task_quintic_startRecording(quinticConfiguration_t* qConfig);
status_t task_quintic_stopRecording(quinticConfiguration_t* qConfig);
status_t task_quintic_startGetAccelData(quinticConfiguration_t* qConfig);
status_t task_quintic_stopGetAccelData(quinticConfiguration_t* qConfig);
status_t task_quintic_putQuinticToSleep(quinticConfiguration_t* qConfig);
void DisconnectImus(quinticConfiguration_t* qConfig);

#endif /* TASK_QUINTIC_INTERFACE_H_ */