/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include <asf.h>
#include <string.h>
#include "common.h"
#include "arm_math.h"
#include "pkt_packetParser.h"
#include "cmd_commandProcessor.h"
#include "drv_i2c.h"
#include "imu.h"

#define SENSOR_ID_DEFAULT 0

/** Handler for the device SysTick module, called when the SysTick counter
 *  reaches the set period.
 *
 *  \note As this is a raw device interrupt, the function name is significant
 *        and must not be altered to ensure it is hooked into the device's
 *        vector table.
 */
void SysTick_Handler(void)
{
	port_pin_toggle_output_level(LED_0_PIN);
}
//declare the configuration for the
volatile struct usart_module cmd_uart_module;
/** Configure GPIO, turn it off*/
#define GPIO_RS485_DATA_DIRECTION_RE PIN_PA02
#define GPIO_RS485_DATA_DIRECTION_DE PIN_PA04
#define GPIO_RS485_DATA_RECEIVE false
#define GPIO_RS485_DATA_TRANSMIT true
static void config_gpio(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &pin_conf);
	port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
	port_pin_set_config(GPIO_RS485_DATA_DIRECTION_RE, &pin_conf);
	port_pin_set_config(GPIO_RS485_DATA_DIRECTION_DE, &pin_conf);
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_RE, GPIO_RS485_DATA_RECEIVE);
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_DE, GPIO_RS485_DATA_RECEIVE);	
}
sensorSettings_t settings = 
{
	.sensorId = SENSOR_ID_DEFAULT,
	.serialNumber = {0,0,0,0,0,0,0,0,0,0,0,0},
	.setupModeEnabled = false
};

/*	I2C structures declarations	*/
drv_twi_config_t twiConfig = 
{
	.p_i2c = SERCOM0,
	.twi_options = 
	{
		.baud_rate = I2C_MASTER_BAUD_RATE_400KHZ,
		.baud_rate_high_speed = I2C_MASTER_BAUD_RATE_400KHZ,
		.transfer_speed = I2C_MASTER_SPEED_STANDARD_AND_FAST,
		.generator_source = GCLK_GENERATOR_0,
		.start_hold_time = I2C_MASTER_START_HOLD_TIME_DISABLED,
		.unknown_bus_state_timeout = 65535,
		.buffer_timeout = 65535,
		.run_in_standby = false,
		.pinmux_pad0 = PINMUX_PA14C_SERCOM0_PAD0,
		.pinmux_pad1 = PINMUX_PA15C_SERCOM0_PAD1,
		.scl_low_timeout = false,
		.inactive_timeout = I2C_MASTER_INACTIVE_TIMEOUT_DISABLED,
		.scl_stretch_only_after_ack_bit = false,
		.slave_scl_low_extend_timeout = false,
		.master_scl_low_extend_timeout = false,
		.sda_scl_rise_time_ns = 215
	},
	.mem_index = 0,
	.module = 
	{
		0
	}
};

slave_twi_config_t em7180Config= 
{
	.emId = 0,
	.address = 0x29,
	.drv_twi_options = &twiConfig
};

void enableRs485Transmit()
{
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_RE, GPIO_RS485_DATA_TRANSMIT);
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_DE, GPIO_RS485_DATA_TRANSMIT);	
}
void disableRs485Transmit()
{
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_RE, GPIO_RS485_DATA_RECEIVE);
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_DE, GPIO_RS485_DATA_RECEIVE);	
}

pkt_packetParserConfiguration_t packetParserConfig = 
{
	.transmitDisable = disableRs485Transmit,
	.transmitEnable = enableRs485Transmit, 
	.packetReceivedCallback = cmd_processPacket,
	.uartModule = &cmd_uart_module	
};
void readUniqueId()
{
	uint8_t* localSerialNumber = 0x0080A00C;
	memcpy(settings.serialNumber,localSerialNumber,16);
}
/** Callback function for the EXTINT driver, called when an external interrupt
 *  detection occurs.
 */
static void extint_callback(void)
{
	if(settings.setupModeEnabled == true)
	{
		sendButtonPressEvent();
	}
}

/** Configures and registers the External Interrupt callback function with the
 *  driver.
 */
static void configure_eic_callback(void)
{
	//extint_register_callback(extint_callback,
			//SW1_INT_EIC_LINE,
			//EXTINT_CALLBACK_TYPE_DETECT);
	//extint_chan_enable_callback(SW1_INT_EIC_LINE,
			//EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(extint_callback,
			SW2_EIC_LINE,
			EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(SW2_EIC_LINE,
			EXTINT_CALLBACK_TYPE_DETECT);			
}

/** Configures the External Interrupt Controller to detect changes in the board
 *  button state.
 */
static void configure_extint(void)
{
	struct extint_chan_conf eint_chan_conf;
	extint_chan_get_config_defaults(&eint_chan_conf);
	eint_chan_conf.gpio_pin           = SW1_INT_PIN;
	eint_chan_conf.gpio_pin_pull = SYSTEM_PINMUX_PIN_PULL_UP;
	eint_chan_conf.gpio_pin_mux       = SW1_INT_EIC_MUX;
	eint_chan_conf.detection_criteria = EXTINT_DETECT_FALLING;
	eint_chan_conf.filter_input_signal = true;
	extint_chan_set_config(SW1_INT_EIC_LINE, &eint_chan_conf);	
	eint_chan_conf.gpio_pin           = SW2_PIN;
	eint_chan_conf.gpio_pin_mux       = SW2_EIC_MUX;
	extint_chan_set_config(SW2_EIC_LINE, &eint_chan_conf);	
}

__attribute__((optimize("O0"))) static void configure_uart(void)
{
	struct usart_config usart_conf;
	//load up the default usart settings.
	usart_get_config_defaults(&usart_conf);
	usart_conf.mux_setting = CMD_UART_MUX_SETTING;
	usart_conf.pinmux_pad0 = CMD_UART_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = CMD_UART_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = CMD_UART_PAD2;
	usart_conf.pinmux_pad3 = CMD_UART_PAD3;
	usart_conf.baudrate    = 460800;
	usart_conf.sample_rate = USART_SAMPLE_RATE_16X_ARITHMETIC;
	status_code_genare_t status = STATUS_NO_CHANGE;
	status = usart_init(&cmd_uart_module, CMD_UART_MODULE, &usart_conf);
	usart_enable(&cmd_uart_module);	
}
volatile uint8_t receivedByte = 0;
void receiveCallback(const struct usart_module *const usart_module)
{		
	//setup the next read. 
	//uint8_t byte = (uint8_t)receivedByte;
	
	pkt_ProcessIncomingByte(receivedByte);		
	//usart_read_job(	&cdc_uart_module,&receivedByte);			
}


__attribute__((optimize("O0"))) int main(void)
{
	system_init();
	configure_uart();
	/*Configure system tick to generate periodic interrupts */
	//SysTick_Config(system_gclk_gen_get_hz(GCLK_GENERATOR_0));
	config_gpio();
	configure_extint();
	configure_eic_callback();
	status_code_genare_t uart_status = STATUS_ERR_NOT_INITIALIZED;
	/*	Initialize I2C drivers	*/
	status_t status = drv_i2c_init(&twiConfig);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_PRINT_F
		puts("I2C init failed\r\n");
		#endif
	}	
	/*Enable system interrupt*/
	system_interrupt_enable_global();	
	

	//read the unique ID for the microcontroller
	readUniqueId(); 	
	int i = 0, size = 0;
	volatile uint16_t buff = 0x00; 
	//uint8_t receivedByte = 0x00; 
	status = STATUS_FAIL;
	//while(status == STATUS_FAIL)
	//{
		status = resetAndInitialize(&em7180Config);
	//}
	pkt_packetParserInit(&packetParserConfig);	
	//usart_register_callback(&cmd_uart_module,receiveCallback,USART_CALLBACK_BUFFER_RECEIVED);
	//usart_enable_callback(&cmd_uart_module, USART_CALLBACK_BUFFER_RECEIVED);
	//usart_read_job(&cmd_uart_module,&receivedByte);
	
	//turn on the LED
	port_pin_set_output_level(LED_0_PIN,LED_0_ACTIVE);
	//delay_ms(500); 
	sendButtonPressEvent();
	
	while (true) 
	{
		uart_status = usart_read_wait(&cmd_uart_module, &buff);
		//
		//delay_ms(100);
		//buff = (uint16_t)receivedByte;
		if(uart_status == STATUS_OK && buff != -1)
		{
			pkt_ProcessIncomingByte((uint8_t)buff);
		}
	}
}