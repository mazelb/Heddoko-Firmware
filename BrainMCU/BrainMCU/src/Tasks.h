/*
 * Tasks.h
 *
 * Created: 9/9/2015 12:24:06 PM
 *  Author: Hriday Mehta
 */ 


#ifndef TASKS_H_
#define TASKS_H_

#include <asf.h>
#include "BrainMCU.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Serial.h"
#include "UART_functionality.h"
#include "GPIO.h"
#include "DebugLog.h"

#define TASK_LED_STACK_SIZE                (1024/sizeof(portSTACK_TYPE))
#define TASK_LED_STACK_PRIORITY            (tskIDLE_PRIORITY)
#define TASK_MAIN_STACK_SIZE                (2048/sizeof(portSTACK_TYPE))
#define TASK_MAIN_STACK_PRIORITY            (tskIDLE_PRIORITY + 2)

static bool svStartStopToggle=0, svPowerSwIrqFlag, svActionSw1IrqFlag, svActionSw2IrqFlag, svSwBtnIrqFlag, svInitCmpFlag=0, svMainTaskCmpFlag=0;
static uint8_t svMenuLevel=0;
static uint32_t svCurrentTicks, svOldTicks, svPioaIsrVal=0;

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);

extern void SysTickHandler(void);

/*	Tasks Declaration	*/
extern void TaskLed		(void *pvParameters);
extern void TaskMain	(void *pvParameters);

int StartTimer (void);

#endif /* TASKS_H_ */