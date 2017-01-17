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
#include "nvm.h"


extern sensorSettings_t settings;
extern uint32_t warmUpParameterValues[35]; 	
/** Handler for the device SysTick module, called when the SysTick counter
 *  reaches the set period.
 *
 *  \note As this is a raw device interrupt, the function name is significant
 *        and must not be altered to ensure it is hooked into the device's
 *        vector table.
 */
void SysTick_Handler(void)
{
	port_pin_toggle_output_level(LED_BLUE_PIN);
}
//declare the configuration for the
volatile struct usart_module cmd_uart_module;

static void config_gpio(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_BLUE_PIN, &pin_conf);
	port_pin_set_config(LED_RED_PIN, &pin_conf);
	port_pin_set_config(LED_GREEN_PIN, &pin_conf);
	port_pin_set_output_level(LED_BLUE_PIN, LED_INACTIVE);
	port_pin_set_output_level(LED_RED_PIN, LED_INACTIVE);
	port_pin_set_output_level(LED_GREEN_PIN, LED_INACTIVE);
	port_pin_set_config(GPIO_RS485_DATA_DIRECTION_RE, &pin_conf);
	port_pin_set_config(GPIO_RS485_DATA_DIRECTION_DE, &pin_conf);
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_RE, GPIO_RS485_DATA_RECEIVE);
	port_pin_set_output_level(GPIO_RS485_DATA_DIRECTION_DE, GPIO_RS485_DATA_RECEIVE);
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	//Setup the EM_INT pin as an 	
	port_pin_set_config(GPIO_EM_MICRO_INT_PIN, &pin_conf);
}


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
	#ifdef HW_V1_2 //this may be temporary, it's due to the SA0 pin seems to be floating.
	.address = 0x28,
	#else
	.address = 0x28,
	#endif  
	.drv_twi_options = &twiConfig
};

slave_twi_config_t eepromConfig=
{
	.emId = 0,
	.address = 0x50, //address for the eeprom	
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
	//SW1_EIC_LINE,
	//EXTINT_CALLBACK_TYPE_DETECT);
	//extint_chan_enable_callback(SW1_EIC_LINE,
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
	//eint_chan_conf.gpio_pin           = SW1_PIN;
	eint_chan_conf.gpio_pin_pull = SYSTEM_PINMUX_PIN_PULL_UP;
	//eint_chan_conf.gpio_pin_mux       = SW1_EIC_MUX;
	eint_chan_conf.detection_criteria = EXTINT_DETECT_FALLING;
	eint_chan_conf.filter_input_signal = true;
	//extint_chan_set_config(SW1_EIC_LINE, &eint_chan_conf);	
	eint_chan_conf.gpio_pin           = SW2_PIN;
	eint_chan_conf.gpio_pin_mux       = SW2_EIC_MUX;
	extint_chan_set_config(SW2_EIC_LINE, &eint_chan_conf);	
}

__attribute__((optimize("O0"))) static void configure_uart(void)
{
	struct usart_config usart_conf;
	//load up the default usart settings.
	//usart_get_config_defaults(&usart_conf);
	usart_conf.mux_setting = CMD_UART_MUX_SETTING;
	usart_conf.pinmux_pad0 = CMD_UART_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = CMD_UART_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = CMD_UART_PAD2;
	usart_conf.pinmux_pad3 = CMD_UART_PAD3;
	usart_conf.baudrate    = settings.baud;
	usart_conf.sample_rate = USART_SAMPLE_RATE_16X_ARITHMETIC;
	status_code_genare_t status = STATUS_NO_CHANGE;
	Sercom* serComPtr = CMD_UART_MODULE;  
	SercomUsart *const usart_hw = &(serComPtr->USART); 

	
	status = usart_init(&cmd_uart_module, CMD_UART_MODULE, &usart_conf);
	//usart_hw->BAUD.reg =  0x00030000UL;
	//usart_hw->CTRLB.reg = 0x0000B15BUL;
	//usart_hw->CTRLA.reg = 0x40310084UL;

	//usart_hw->INTFLAG.reg = 0x00000001UL;
	usart_enable(&cmd_uart_module);	
}

void reConfigure_uart()
{
	usart_disable(&cmd_uart_module);
	configure_uart();
}

volatile uint8_t receivedByte = 0;
void receiveCallback(const struct usart_module *const usart_module)
{		
	//setup the next read. 
	//uint8_t byte = (uint8_t)receivedByte;
	
	pkt_ProcessIncomingByte(receivedByte);		
	//usart_read_job(	&cdc_uart_module,&receivedByte);			
}


__attribute__((optimize("O0"))) void configure_eeprom(void)
{
	 ///* Setup EEPROM emulator service */
	 //enum status_code error_code = eeprom_emulator_init();
	 //if (error_code == STATUS_ERR_NO_MEMORY) 
	 //{
		 //while (true) 
		 //{
		 ///* No EEPROM section has been set in the device's fuses */
		 //}
	 //}
	 //else if (error_code != STATUS_OK) 
	 //{
		 ///* Erase the emulated EEPROM memory (assume it is unformatted or
		 //* irrecoverably corrupt) */
		 //eeprom_emulator_erase_memory();
		 //eeprom_emulator_init();
	 //}
	 	enum status_code error_code = STATUS_OK;
	 	struct nvm_config config;
	 	struct nvm_parameters parameters;

	/* Retrieve the NVM controller configuration - enable manual page writing
	 * mode so that the emulator has exclusive control over page writes to
	 * allow for caching */
	nvm_get_config_defaults(&config);
	config.manual_page_write = true;

	/* Apply new NVM configuration */
	do {
		error_code = nvm_set_config(&config);
	} while (error_code == STATUS_BUSY);

	/* Get the NVM controller configuration parameters */
	nvm_get_parameters(&parameters);

	///* Ensure the device fuses are configured for at least one master page row,
	 //* one user EEPROM data row and one spare row */
	//if (parameters.eeprom_number_of_pages < (3 * NVMCTRL_ROW_PAGES)) 
	//{
		//return STATUS_ERR_NO_MEMORY;
	//} 
	
	uint8_t dataTest[64] = {0};
	//memset(dataTest,0xA5,64);
	do {
		error_code = nvm_erase_row(
		(uint32_t)(0x3F00));
	} while (error_code == STATUS_BUSY);
	
	do {
		error_code = nvm_write_buffer(
		(uint32_t)(0x3F00),
		dataTest,
		NVMCTRL_PAGE_SIZE);
	} while (error_code == STATUS_BUSY);
	//memset(dataTest,0x00,64);
	//
	do {
		error_code = nvm_execute_command(
		NVM_COMMAND_WRITE_PAGE,
		(uint32_t)(0x3F00), 0);
	} while (error_code == STATUS_BUSY);
	//
	do {
		error_code = nvm_read_buffer(
		(uint32_t)(0x3F00),
		dataTest,
		NVMCTRL_PAGE_SIZE);
	} while (error_code == STATUS_BUSY);	
	
 
}

int main(void)
{
	system_init();
	configure_uart();
	/*Configure system tick to generate periodic interrupts */
	//SysTick_Config(system_gclk_gen_get_hz(GCLK_GENERATOR_0));
	config_gpio();
	configure_extint();
	configure_eic_callback();
	//initialize the warm up parameters
	memset(warmUpParameterValues,0xA5,140);

	if(loadSettings() != STATUS_PASS)
	{
		//load the default settings into memory. 
		writeSettings();
	}
	
	//configure_eeprom();
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
	port_pin_set_output_level(LED_BLUE_PIN,LED_ACTIVE);
	//delay_ms(5000); 
	//port_pin_set_output_level(LED_BLUE_PIN,LED_INACTIVE);	
	//port_pin_set_output_level(LED_GREEN_PIN,LED_ACTIVE);
	//delay_ms(5000); 
	//port_pin_set_output_level(LED_GREEN_PIN,LED_INACTIVE);
	//port_pin_set_output_level(LED_RED_PIN,LED_ACTIVE);
	
	//sendButtonPressEvent();
	
	sendGetStatusResponse();
	//port_pin_set_output_level(LED_GREEN_PIN,LED_INACTIVE);
	
	//togglePassthrough(0x01);
	//delay_ms(1000);
	//getEepromPacket(0x0000);
	//delay_ms(1000);
	//togglePassthrough(0x00);
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