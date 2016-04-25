/*
 * brd_board.c
 *
 * Created: 2/29/2016 2:38:45 PM
 *  Author: sean
 */ 
#include "brd_board.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "drv_led.h"
#include "drv_i2c.h"
#include "LTC2941-1.h"


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

drv_led_config_t ledConfiguration =
{
	.redLed = DRV_GPIO_PIN_LED_RED,
	.greenLed = DRV_GPIO_PIN_LED_GREEN,
	.blueLed = DRV_GPIO_PIN_LED_BLUE
};

drv_twi_config_t twiConfig[] =
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
		.IRQtype = TWI0_IRQn,
		.mem_index = 0
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
		.IRQtype = TWI1_IRQn,
		.mem_index = 1
	}
};

slave_twi_config_t ltc2941Config =
{
	.id = 0,
	.address = 0x64,
	.drv_twi_options = &twiConfig[0]
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
	stdio_serial_init(UART1, &usart_serial_options);
	/* Specify that stdout should not be buffered. */
	#if defined(__GNUC__)
		setbuf(stdout, NULL);
	#else
	/* Already the case in IAR's Normal DLIB default configuration: printf()
	 * emits one character at a time.
	 */
	#endif
}

void brd_board_init()
{
	//configure the interrupt handlers first. 
	brd_enableWatchdog();
	//configure the gpio
	drv_gpio_initializeAll();
	//configure LED driver
	drv_led_init(&ledConfiguration);
	//Set LED white to indicate power on
	drv_led_set(DRV_LED_WHITE,DRV_LED_SOLID);
	vTaskDelay(200);
	pmc_switch_sclk_to_32kxtal(PMC_OSC_XTAL);
	while (!pmc_osc_is_ready_32kxtal());
	rtc_set_hour_mode(RTC, 0);
	rtc_clear_date_alarm(RTC);
	rtc_clear_time_alarm(RTC);
	drv_led_set(DRV_LED_OFF,DRV_LED_SOLID);	
	brd_initAllUarts();
	configure_console();
	//try to configure the USB
	udc_start(); 
	drv_i2c_init(&twiConfig[0]);
	ltc2941Init(&ltc2941Config);
}


void brd_initAllUarts()
{
	if(drv_uart_init(&uart0Config) != STATUS_PASS)
	{
		while(1); //spin here
	}
	if(drv_uart_init(&uart1Config) != STATUS_PASS)
	{
		while(1); //spin here
	}
}

void brd_deInitAllUarts()
{
	drv_uart_deInit(&uart0Config);
	drv_uart_deInit(&uart1Config);
}


/**
 *  \brief Handler for watchdog interrupt.
 */
void WDT_Handler(void)
{
	/* Clear status bit to acknowledge interrupt by dummy read. */
	wdt_get_status(WDT);

	//debugPrintString("Restarting system!!!\r");
	rstc_start_software_reset(RSTC);	
}

/**
 * configureWatchDog(void)
 * @brief Initialize the watchdog timer, only done once on power up. 
 */
void brd_enableWatchdog()
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
	//WDT_MR_WDFIEN |  /* Enable WDT fault interrupt. */
	uint32_t wdt_mode = WDT_MR_WDRSTEN | 			
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