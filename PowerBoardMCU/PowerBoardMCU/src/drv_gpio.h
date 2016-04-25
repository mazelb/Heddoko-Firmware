/*
 * drv_gpio.h
 *
 * Created: 9/25/2015 8:12:59 AM
 *  Author: Sean Cloghesy
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 

#include "asf.h"
#include "common.h"

#ifndef DRV_GPIO_H_
#define DRV_GPIO_H_



#define DRV_GPIO_ID_PIN_JC_EN2		PIO_PA17_IDX /*	Jack 2 Enable	*/
#define DRV_GPIO_ID_PIN_JC_EN1		PIO_PA18_IDX /*	Jack 1 Enable   */
#define DRV_GPIO_ID_PIN_JC1_DET		PIO_PA11_IDX /*	Jack 1 detect	*/
#define DRV_GPIO_ID_PIN_JC2_DET		PIO_PA1_IDX /*	Jack 2 detect	*/
#define DRV_GPIO_ID_PIN_PWR_EN		PIO_PA19_IDX /*	Power Enable	*/
#define DRV_GPIO_ID_PIN_CHRG_SEL	PIO_PA16_IDX /*	SEL input on charger IC*/
#define DRV_GPIO_ID_PIN_CHRG_PG		PIO_PA15_IDX /*	Power Good on charger IC*/
#define DRV_GPIO_ID_PIN_CHRG_STAT2	PIO_PA14_IDX /*	STAT2 on charger IC*/
#define DRV_GPIO_ID_PIN_CHRG_STAT1	PIO_PA13_IDX /*	STAT1 on charger IC*/
#define DRV_GPIO_ID_PIN_GPIO		PIO_PA6_IDX /*	Extra GPIO	*/
#define DRV_GPIO_ID_PIN_PWR_BTN		PIO_PA5_IDX /*	Power Button	*/
#define DRV_GPIO_ID_PIN_USB_DET		PIO_PA0_IDX /*	USB VBUS Detect	*/
#define DRV_GPIO_ID_PIN_LED_BLUE	PIO_PB0_IDX /*	Blue LED (active low)*/
#define DRV_GPIO_ID_PIN_LED_GREEN	PIO_PB1_IDX /*	Green LED	*/
#define DRV_GPIO_ID_PIN_LED_RED		PIO_PA20_IDX /*	Red LED	*/


typedef enum
{	
	DRV_GPIO_PIN_JC_EN2,
	DRV_GPIO_PIN_JC_EN1,
	DRV_GPIO_PIN_JC1_DET,
	DRV_GPIO_PIN_JC2_DET,
	DRV_GPIO_PIN_PWR_EN,
	DRV_GPIO_PIN_CHRG_SEL,
	DRV_GPIO_PIN_CHRG_PG,
	DRV_GPIO_PIN_CHRG_STAT2,
	DRV_GPIO_PIN_CHRG_STAT1,
	DRV_GPIO_PIN_GPIO,
	DRV_GPIO_PIN_PWR_BTN,
	DRV_GPIO_PIN_USB_DET,
	DRV_GPIO_PIN_LED_BLUE,
	DRV_GPIO_PIN_LED_GREEN,
	DRV_GPIO_PIN_LED_RED	
}drv_gpio_pins_t;
	
typedef enum 
{
	DRV_GPIO_PIN_MODE_OUTPUT,
	DRV_GPIO_PIN_MODE_INPUT
}drv_gpio_pinMode_t;
typedef enum
{
	DRV_GPIO_INTERRUPT_HIGH_EDGE, 
	DRV_GPIO_INTERRUPT_LOW_EDGE,
	DRV_GPIO_INTERRUPT_HIGH_LVL, 
	DRV_GPIO_INTERRUPT_LOW_LVL,
	DRV_GPIO_INTERRUPT_NONE	
}drv_gpio_interrupt_t;

typedef enum
{
	DRV_GPIO_PIN_STATE_LOW,
	DRV_GPIO_PIN_STATE_HIGH,
	DRV_GPIO_PIN_STATE_PULLED_HIGH, 
	DRV_GPIO_PIN_STATE_PULLED_LOW 	
}drv_gpio_pin_state_t;

typedef struct
{
	uint32_t pinId;
	drv_gpio_pinMode_t pinMode; 
	drv_gpio_pin_state_t initialPinState;
	drv_gpio_interrupt_t interruptType; 
	void* interruptHandler;
	uint8_t pullUpEnabled;
	uint8_t debounceEnabled; 
	bool gpioSetFlag; 
	drv_gpio_pin_state_t currentPinState; 	
}drv_gpio_config_t;


status_t drv_gpio_initializeAll(void); 
status_t drv_gpio_initializeForBootloader(void);
status_t drv_gpio_ConfigureBLEForProgramming(void);
status_t drv_gpio_config(drv_gpio_config_t* gpioConfig);
status_t drv_gpio_setPinState(drv_gpio_pins_t pinId, drv_gpio_pin_state_t state);
status_t drv_gpio_getPinState(drv_gpio_pins_t pinId, drv_gpio_pin_state_t* state);
status_t drv_gpio_togglePin(drv_gpio_pins_t pinId);
status_t drv_gpio_config_interrupt(drv_gpio_pins_t pin, drv_gpio_interrupt_t pinInt);
status_t drv_gpio_config_interrupt_handler(drv_gpio_pins_t pin, drv_gpio_interrupt_t pinInt, void* handler);
status_t drv_gpio_enable_interrupt(drv_gpio_pins_t pin);
status_t drv_gpio_save_interrupt_mask_all(void);
status_t drv_gpio_disable_interrupt_all(void);
bool drv_gpio_check_Int(drv_gpio_pins_t pin);
bool drv_gpio_clear_Int(drv_gpio_pins_t pin);

#define	DEBOUNCE_PERIOD	5

//status_t drv_uart_isInit(drv_gpio_config_t* gpioConfig);

#endif /* DRV_GPIO_H_ */