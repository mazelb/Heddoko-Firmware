/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
/*
 * task_commandProc.h
 *
 * Created: 10/28/2015 9:47:21 AM
 *  Author: sean
 */ 


#ifndef TASK_COMMANDPROC_H_
#define TASK_COMMANDPROC_H_

#include "drv_uart.h"

#define MAX_DEBUG_STRING_LENGTH	200

typedef struct  
{
	drv_uart_config_t* uart; 	
}commandProcConfig_t;

void task_commandHandler(void *pvParameters); 
void printString(char* str);
void sendPacket(char* buf, size_t length);
void __attribute__((optimize("O0"))) debugPrintString(char* str);
void __attribute__((optimize("O0"))) debugPrintStringInt(char* str, int number); 
#endif /* TASK_COMMANDPROC_H_ */