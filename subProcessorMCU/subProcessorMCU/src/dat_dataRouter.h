/*
 * dat_dataRouter.h
 *
 * Created: 2/29/2016 4:05:10 PM
 *  Author: sean
 */ 
#include "drv_uart.h"

#ifndef DAT_DATAROUTER_H_
#define DAT_DATAROUTER_H_
#include "cmd_commandProc.h"

typedef struct  
{
	drv_uart_config_t* dataBoardUart;
	//drv_uart_config_t* sensorUart;	
}dat_dataRouterConfig_t;

void dat_task_dataRouter(void *pvParameters);
status_t dat_sendPacketToDataBoard(cmd_commandPacket_t* packet);
status_t dat_sendStringToUsb(char* str);
status_t dat_sendDebugMsgToDataBoard(char* debugString);
#endif /* DAT_DATAROUTER_H_ */