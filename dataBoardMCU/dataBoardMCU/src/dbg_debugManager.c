/*
 * dbg_debugManager.c
 *
 * Created: 7/12/2016 10:25:21 AM
 *  Author: sean
 */ 
#include "dbg_debugManager.h"
#include "drv_uart.h"

drv_uart_config_t debugUartConfig =
{
	.p_usart = UART1,
	.mem_index = 1,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
	.mode = DRV_UART_MODE_INTERRUPT
};
status_t dbg_init()
{
	status_t status = STATUS_PASS;
	status = drv_uart_init(&debugUartConfig); 
	return status; 
}

void dbg_printString(char* string)
{
	drv_uart_putString(&debugUartConfig,string); 
}