/**
* @file drv_gpio.h
* @brief Driver for General Purpose IO (GPIO)
* @author Sean Cloghesy (sean@heddoko.com)
* @date September 2015
* Copyright Heddoko(TM) 2015, all rights reserved
*/

#include "asf.h"
#include "common.h"

#ifndef DRV_GPIO_H_
#define DRV_GPIO_H_


#define DRV_GPIO_ID_PIN_PW_SW		PIO_PA0_IDX /*	Power button	*/
#define DRV_GPIO_ID_PIN_AC_SW1		PIO_PA2_IDX /*	Action Switch 1	*/
#define DRV_GPIO_ID_PIN_AC_SW2		PIO_PB14_IDX /*	Action Switch 2	*/
#define DRV_GPIO_ID_PIN_BLE_RST		PIO_PA23_IDX /*	BLE Reset	*/
#define DRV_GPIO_ID_PIN_GREEN_LED	PIO_PB1_IDX /*	Green LED	*/
#define DRV_GPIO_ID_PIN_BLUE_LED	PIO_PB0_IDX /*	Blue LED	*/
#define DRV_GPIO_ID_PIN_RED_LED		PIO_PA18_IDX /*	Red LED	*/
#define DRV_GPIO_ID_PIN_SD_CD		PIO_PA17_IDX /* SD CARD DETECT	*/  //TODO add ifdef for new HW PIO_PC12
#define DRV_GPIO_ID_PIN_USB_DET		PIO_PB13_IDX /*	USB Connected Detect */
#define DRV_GPIO_ID_PIN_SUBP_GPIO	PIO_PA20_IDX /*	Sub-processor/ power board GPIO */
#define DRV_GPIO_ID_PIN_AUX_GPIO	PIO_PA20_IDX /*	Auxiliary port GPIO */
#define DRV_GPIO_ID_PIN_PIEZO_OUT	PIO_PA15_IDX /*	Piezo Buzzer Output */
#define DRV_GPIO_ID_PIN_HAPTIC_OUT	PIO_PA19_IDX /*	Haptic Output */
#define DRV_GPIO_ID_PIN_WIFI_IRQ	PIO_PA1_IDX /*	WIFI Module IRQ Pin */
#define DRV_GPIO_ID_PIN_WIFI_EN		PIO_PA6_IDX /*	WIFI Module Enable Pin */
#define DRV_GPIO_ID_PIN_WIFI_CS		PIO_PA11_IDX /*	WIFI Module Chip Select */
#define DRV_GPIO_ID_PIN_WIFI_RST	PIO_PA24_IDX /*	WIFI Module Reset */
#define DRV_GPIO_ID_PIN_WIFI_WAKE	PIO_PA25_IDX /*	WIFI Module Wake */





typedef enum
{	
	DRV_GPIO_PIN_PW_SW,
	DRV_GPIO_PIN_AC_SW1,
	DRV_GPIO_PIN_AC_SW2,
	DRV_GPIO_PIN_BLE_RST,
	DRV_GPIO_PIN_GREEN_LED,
	DRV_GPIO_PIN_BLUE_LED,
	DRV_GPIO_PIN_RED_LED,
	DRV_GPIO_PIN_SD_CD,
	DRV_GPIO_PIN_USB_DET,
	DRV_GPIO_PIN_SUBP_GPIO,
	DRV_GPIO_PIN_AUX_GPIO,
	DRV_GPIO_PIN_PIEZO_OUT,
	DRV_GPIO_PIN_HAPTIC_OUT,
	DRV_GPIO_PIN_WIFI_IRQ,
	DRV_GPIO_PIN_WIFI_EN,
	DRV_GPIO_PIN_WIFI_CS,
	DRV_GPIO_PIN_WIFI_RST,
	DRV_GPIO_PIN_WIFI_WAKE
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
void drv_gpio_service_Int(drv_gpio_pins_t pin, uint32_t ul_mask, bool *intGeneratedFlag);

#define	DEBOUNCE_PERIOD	5

//status_t drv_uart_isInit(drv_gpio_config_t* gpioConfig);

#endif /* DRV_GPIO_H_ */