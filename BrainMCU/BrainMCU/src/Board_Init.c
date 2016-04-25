/*
 * Board_Init.c
 *
 * Created: 9/16/2015 5:35:27 PM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 

#include "Board_Init.h"
#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "conf_board.h"
#include "Functionality_Tests.h"
#include "settings.h"
#include "drv_uart.h"
#include "drv_gpio.h"
#include "task_main.h"
#include "DebugLog.h"
#include "common.h"
#include "drv_led.h"
#include "rtc.h"
#include "task_commandProc.h"
#include "drv_i2c.h"

//configuration structures

FATFS fs;
extern brainSettings_t brainSettings;
extern slave_twi_config_t em7180Config[];
void configureWatchDog();

drv_uart_config_t uart0Config =
{
	.p_usart = UART0,
	.mem_index = 0,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};
drv_uart_config_t uart1Config =
{
	.p_usart = UART1,
	.mem_index = 1,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};
drv_uart_config_t usart0Config =
{
	.p_usart = USART0,
	.mem_index = 2,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};
drv_uart_config_t usart1Config =
{
	.p_usart = USART1,
	.mem_index = 3,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};

#ifdef ENABLE_EM_SENSORS
//EM7180 part
drv_twi_config_t twiConfig[2] =
{
	{
		.p_i2c = TWI0,
		.twi_options =
		{
			.chip = 0,	// to be initialized in slave config
			.master_clk = 12000000,
			.speed = 400000,
			.smbus = 0
		},
		.freeRtos_options = 
		{
			.receive_buffer = NULL,
			.receive_buffer_size = 0,
			.interrupt_priority = 14,	//The higher the value lower is the priority
			.operation_mode = TWI_I2C_MASTER,
			.options_flags = (USE_TX_ACCESS_MUTEX | USE_RX_ACCESS_MUTEX)// | WAIT_TX_COMPLETE | WAIT_RX_COMPLETE)
		},
		.freertos_twi = NULL
	},
	{
		.p_i2c = TWI1,
		.twi_options =
		{
			.chip = 0,	// to be initialized in slave config
			.master_clk = 12000000,
			.speed = 400000,
			.smbus = 0
		},
		.freeRtos_options =
		{
			.receive_buffer = NULL,
			.receive_buffer_size = 0,
			.interrupt_priority = 14,	//The higher the value lower is the priority
			.operation_mode = TWI_I2C_MASTER,
			.options_flags = (USE_TX_ACCESS_MUTEX | USE_RX_ACCESS_MUTEX)// | WAIT_TX_COMPLETE | WAIT_RX_COMPLETE)
		},
		.freertos_twi = NULL
	}
};
#endif

drv_led_config_t ledConfiguration = 
{
	.redLed = DRV_GPIO_PIN_RED_LED,
	.greenLed = DRV_GPIO_PIN_GREEN_LED,
	.blueLed = DRV_GPIO_PIN_BLUE_LED
};

/**
 * \brief Configure the console UART for using printf.
 */
static void configure_console(void)
{
	const usart_serial_options_t usart_serial_options = 
	{
		.baudrate   = CONF_TEST_BAUDRATE,
		.charlength = CONF_TEST_CHARLENGTH,
		.paritytype = CONF_TEST_PARITY,
		.stopbits   = CONF_TEST_STOPBITS,
	};

	/* Configure console UART. */
	#ifdef USE_Q1_Q2
	stdio_serial_init(UART1, &usart_serial_options);
	#else
	stdio_serial_init(UART0, &usart_serial_options);
	#endif
	/* Specify that stdout should not be buffered. */
	#if defined(__GNUC__)
		setbuf(stdout, NULL);
	#else
	/* Already the case in IAR's Normal DLIB default configuration: printf()
	 * emits one character at a time.
	 */
	#endif
}
/**
 * powerOnInit(void)
 * @brief Initialize the board after power up. 
 */
void powerOnInit(void) 
{		
		static FRESULT res;
		Ctrl_status status;

		//configure the gpio
		drv_gpio_initializeAll();
		loadSerialNumberFromNvm();
		drv_led_init(&ledConfiguration);
		drv_led_set(DRV_LED_WHITE,DRV_LED_SOLID);
		vTaskDelay(200);  
		
		pmc_switch_sclk_to_32kxtal(PMC_OSC_XTAL);
		while (!pmc_osc_is_ready_32kxtal());
		rtc_set_hour_mode(RTC, 0);
		rtc_clear_date_alarm(RTC);
		rtc_clear_time_alarm(RTC);		
		drv_led_set(DRV_LED_OFF,DRV_LED_SOLID);
		//drv_gpio_ConfigureBLEForProgramming(); 

		//initialize the 
		//configure the bluetooth dongle, if brand new
		//TODO add a check to make sure it needs to be reconfigured first.
		//drv_uart_deInit(config->uart);
		//uart0Config.uart_options.baudrate = 9600; 
		//drv_gpio_setPinState(DRV_GPIO_PIN_BT_PWR_EN, DRV_GPIO_PIN_STATE_HIGH);
			//
		//drv_uart_init(&uart0Config);	
		//vTaskDelay(1000);	
		//char tempString[50] = {0};
		////drv_uart_putString(&uart0Config,"\r\n\r\n");	
		////vTaskDelay(200);
		//drv_uart_putString(&uart0Config,"AT+NAMEHEDDOKO05");	
		//drv_uart_getlineTimed(&uart0Config,tempString,50,2000); 	
		//vTaskDelay(2000);
		//drv_uart_putString(&uart0Config,"AT+BAUD8");
		//drv_uart_getlineTimed(&uart0Config,tempString,50,2000);
		//vTaskDelay(2000);		
		////drv_gpio_setPinState(DRV_GPIO_PIN_BT_PWR_EN, DRV_GPIO_PIN_STATE_LOW);
		//drv_uart_deInit(&uart0Config); 
		//uart0Config.uart_options.baudrate = CONF_BAUDRATE;		
		//configure UART1 to be used as a STDIO function
		configure_console();		
		initAllUarts();
		
}

void initAllUarts()
{
	if(drv_uart_init(&uart0Config) != STATUS_PASS)
	{
		while(1); //spin here
	}
	if(drv_uart_init(&uart1Config) != STATUS_PASS)
	{
		while(1); //spin here
	}		
	if(drv_uart_init(&usart0Config) != STATUS_PASS)
	{
		while(1); //spin here
	}
	if(drv_uart_init(&usart1Config) != STATUS_PASS)
	{
		while(1); //spin here
	}
}

void deInitAllUarts()
{
	drv_uart_deInit(&uart0Config);
	drv_uart_deInit(&uart1Config);
	drv_uart_deInit(&usart0Config);
	drv_uart_deInit(&usart1Config);
}




/**
 *  \brief Handler for watchdog interrupt.
 */
void WDT_Handler(void)
{

	/* Clear status bit to acknowledge interrupt by dummy read. */
	wdt_get_status(WDT);

	debugPrintString("Restarting system!!!\r");
	//rstc_start_software_reset(RSTC);
	
}

/**
 * powerOnInit(void)
 * @brief Initialize the watchdog timer, only done once on power up. 
 */
void configureWatchDog()
{
	pmc_enable_periph_clk(ID_WDT);
	/* Get timeout value. */
	uint32_t timeout_value = wdt_get_timeout_value(WDT_PERIOD * 1000,
			BOARD_FREQ_SLCK_XTAL);
	if (timeout_value == WDT_INVALID_ARGUMENT) {
		while (1) {
			/* Invalid timeout value, error. */
		}
	}
	/* Configure WDT to trigger an interrupt (or reset). */
	uint32_t wdt_mode = WDT_MR_WDRSTEN | WDT_MR_WDFIEN |  /* Enable WDT fault interrupt. */
			WDT_MR_WDDBGHLT  |  /* WDT stops in debug state. */
			WDT_MR_WDIDLEHLT;   /* WDT stops in idle state. */
	
	/* Initialize WDT with the given parameters. */
	wdt_init(WDT, wdt_mode, timeout_value, timeout_value);
			
	/* Configure and enable WDT interrupt. */
	NVIC_DisableIRQ(WDT_IRQn);
	NVIC_ClearPendingIRQ(WDT_IRQn);
	NVIC_SetPriority(WDT_IRQn, 0);
	NVIC_EnableIRQ(WDT_IRQn);					
				
}