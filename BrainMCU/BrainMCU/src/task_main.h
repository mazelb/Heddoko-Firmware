/*
 * task_main.h
 *
 * Created: 9/23/2015 3:34:06 PM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#ifndef TASK_MAIN_H_
#define TASK_MAIN_H_


#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Serial.h"
#include "GPIO.h"
#include "DebugLog.h"
#include "drv_gpio.h"

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);

extern void SysTickHandler(void);

void toggleJackEnables(drv_gpio_pin_state_t pinState);

/*	Tasks Declaration	*/
extern void TaskMain	(void *pvParameters);

int StartTimer (void);

#endif /* TASK_MAIN_H_ */