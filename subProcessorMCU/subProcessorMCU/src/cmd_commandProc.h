/*
 * cmd_commandProc.h
 *
 * Created: 3/1/2016 9:00:36 AM
 *  Author: sean
 */ 


#ifndef CMD_COMMANDPROC_H_
#define CMD_COMMANDPROC_H_
#include "common.h"
#define CMD_INCOMING_CMD_SIZE_MAX 100
typedef enum
{
	CMD_COMMAND_SOURCE_DAUGHTER,
	CMD_COMMAND_SOURCE_USB,
	CMD_COMMAND_SOURCE_LOCAL
}cmd_commandSource_t;

typedef struct  
{
	cmd_commandSource_t packetSource;
	char packetData[CMD_INCOMING_CMD_SIZE_MAX];	
	uint16_t packetSize; 
}cmd_commandPacket_t;

void cmd_task_commandProcesor(void *pvParameters);
status_t cmd_sendDateTimeCommand();
void cmd_initPacketStructure(cmd_commandPacket_t* packet); 

#endif /* CMD_COMMANDPROC_H_ */