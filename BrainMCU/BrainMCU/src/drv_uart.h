/*
 * drv_uart.h
 *
 * Created: 9/21/2015 8:27:36 AM
 *  Author: Sean Cloghesy
 * Q1 is connected to USART1 (PA22 = TX, PA21 = RX)
 * Q2 is connected to UART0  (PA10 = TX, PA9 = RX)
 * Q3 is connected to USART0 (PA6 = TX, PA5 = RX)
 */ 
/**
 * @file  drv_uart.h
 * @brief Low level uart driver with circular buffer on receive
 * @author Heddoko
 * @note This driver can be used on UART0, UART1, USART0 and USART1
 *
 * Copyright Heddoko(TM) 2015, all rights reserved
 */
#include "common.h"
#include "asf.h"

#ifndef DRV_UART_H_
#define DRV_UART_H_
#define FIFO_BUFFER_SIZE 512

typedef struct
{
	uint8_t data_buf[FIFO_BUFFER_SIZE];
	uint16_t i_first;
	uint16_t i_last;
	uint16_t num_bytes;
}sw_fifo_typedef;

typedef struct
{
	Usart *p_usart;
	usart_serial_options_t uart_options; 
	int mem_index; 		
}drv_uart_config_t;

status_t drv_uart_init(drv_uart_config_t* uartConfig);
status_t drv_uart_putChar(drv_uart_config_t* uartConfig, char c); 
status_t drv_uart_getChar(drv_uart_config_t* uartConfig, char* c); 
status_t drv_uart_deInit(drv_uart_config_t* uartConfig); 
status_t drv_uart_isInit(drv_uart_config_t* uartConfig);
status_t drv_uart_getline(drv_uart_config_t* uartConfig, char* str, size_t str_size); 
status_t drv_uart_getlineTimed(drv_uart_config_t* uartConfig, char* str, size_t strSize, uint32_t maxTime);
status_t drv_uart_getlineTimedSized(drv_uart_config_t* uartConfig, char* str, size_t strSize, uint32_t maxTime, uint8_t* strLength);
uint32_t drv_uart_getNumBytes(drv_uart_config_t* uartConfig);
void drv_uart_putString(drv_uart_config_t* uartConfig, char* str);
void drv_uart_putData(drv_uart_config_t* uartConfig, char* str, size_t length); 
void drv_uart_flushRx(drv_uart_config_t* uartConfig);
uint32_t drv_uart_getDroppedBytes(drv_uart_config_t* uartConfig); 
#endif /* DRV_UART_H_ */