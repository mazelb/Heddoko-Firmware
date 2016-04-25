/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
/*
 * DebugLog.h
 *
 * Created: 9/23/2015 2:51:50 PM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#ifndef DEBUGLOG_H_
#define DEBUGLOG_H_


#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Serial.h"
#include "GPIO.h"
#include "task_main.h"

#define TASK_DEBUGLOG_STACK_SIZE                (2048/sizeof(portSTACK_TYPE))
#define TASK_DEBUGLOG_STACK_PRIORITY            (tskIDLE_PRIORITY + 1)
#define DATA_SIZE 200
/*	Global buffer to store Debug Log	*/
struct DEBUGLOG
{
	char DebugLogBuf[DATA_SIZE];
	unsigned int DebugLogBufTail, DebugLogBufHead;
	bool DebugLockBit;
};


/*	Tasks Declaration	*/
extern void TaskDebugLog(void *pvParameters);
extern void DebugLogBufPrint(const char* InputString);
extern void DebugLogCreate(void);
extern void DebugLogSave(void);


#endif /* DEBUGLOG_H_ */