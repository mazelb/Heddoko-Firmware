/*
 * drv_led.h
 *
 * Created: 10/27/2015 10:45:11 AM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 

#include "asf.h"
#include "common.h"
#include "drv_gpio.h"

#ifndef DRV_LED_H_
#define DRV_LED_H_

#define LED_BLINK_RATE 250

typedef struct
{
	drv_gpio_pins_t redLed;
	drv_gpio_pins_t blueLed;
	drv_gpio_pins_t greenLed;
}drv_led_config_t;

typedef enum
{
	DRV_LED_FLASH,
	DRV_LED_SOLID
}drv_led_state_type_t;

typedef enum
{
	DRV_LED_RED,
	DRV_LED_BLUE,
	DRV_LED_GREEN,
	DRV_LED_YELLOW,
	DRV_LED_WHITE,
	DRV_LED_TURQUOISE,
	DRV_LED_PURPLE,
	DRV_LED_OFF
}drv_led_color_type_t;

//static function declarations
status_t drv_led_init(drv_led_config_t* configLed);
status_t drv_led_set(drv_led_color_type_t ledColor, drv_led_state_type_t ledState);
void drv_led_activate_timer();

#endif /* DRV_LED_H_ */