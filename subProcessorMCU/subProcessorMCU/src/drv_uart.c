/*
 * drv_uart.c
 *
 * Created: 9/21/2015 8:27:54 AM
 *  Author: Sean Cloghesy 
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 
/**
 * @file  drv_uart.c
 * @brief Low level uart driver with circular buffer on receive
 * @author Heddoko
 * @note This driver can be used on UART0, UART1, USART0 and USART1
 *
 * @copy Heddoko (2015)
 */
#include "drv_uart.h"
#include <interrupt.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"

typedef struct
{
	uint8_t isinit; 
	sw_fifo_typedef* rx_fifo; 
	packet_fifo_t* packet_fifo; //set to null if this mode is not used	
	drv_uart_packetCallback_t packetCallback;	//call back is null if this is not used. 		
	uint8_t uart_rx_fifo_not_empty_flag; // this flag is automatically set and cleared by the software buffer
	uint8_t uart_rx_fifo_full_flag;      // this flag is automatically set and cleared by the software buffer
	uint8_t uart_rx_fifo_ovf_flag;       // this flag is not automatically cleared by the software buffer	
	uint32_t uart_rx_fifo_dropped_bytes; 
	sw_fifo_typedef* tx_fifo; 
	uint8_t uart_tx_fifo_not_empty_flag; // this flag is automatically set and cleared by the software buffer
	uint8_t uart_tx_fifo_full_flag;      // this flag is automatically set and cleared by the software buffer
	uint8_t uart_tx_fifo_ovf_flag;       // this flag is not automatically cleared by the software buffer	
	uint32_t uart_tx_fifo_dropped_bytes; 
		
}drv_uart_memory_buf_t;

extern unsigned long sgSysTickCount;
//global variables
volatile drv_uart_memory_buf_t uartMemBuf[2]; //2 UARTS, 2 buffers

//declare the pointers for each of the UARTS instead of the pointer

//static function declarations
static int uart_get_byte(drv_uart_memory_buf_t* memBuf, char* c); 
static status_t uart_getPacket(drv_uart_memory_buf_t* memBuf, drv_uart_rawPacket_t* packet);
static status_t uart_processPacketByte(uint8_t byte, drv_uart_memory_buf_t* memBuf);
static void uart_process_byte(Usart *p_usart, drv_uart_memory_buf_t* memBuf);
static void uart_process_tx_byte(Usart *p_usart, drv_uart_memory_buf_t* memBuf);
/***********************************************************************************************
 * drv_uart_init(drv_uart_config_t* uartConfig)
 * @brief initialize uart driver and circular buffer
 * @param uartConfig, the configuration structure for the uart, contains settings.  
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/	
status_t drv_uart_init(drv_uart_config_t* uartConfig)
{
	status_t status = STATUS_PASS; 
	
	if(uartConfig->p_usart == UART0)
	{
		uartConfig->mem_index = 0;
	}
	else if(uartConfig->p_usart == UART1)
	{	
		uartConfig->mem_index = 1;
	}
	else if (uartConfig->p_usart == USART0)
	{
		uartConfig->mem_index = 2;
	}
	else
	{
		//ERROR! The config settings have an invalid UART pointer
		return STATUS_FAIL;
	}

	//initialize the circular buffers. 
	if(uartConfig->mem_index < 0 || uartConfig->mem_index > 2)
	{
		//ERROR! an incorrect buffer index has been used.  
		return STATUS_FAIL; 
	}
	//make sure the buffer isn't already initialized. 
	if(uartMemBuf[uartConfig->mem_index].isinit == true)
	{
		//buffer already in use, call drv_uart_deinit() first
		return STATUS_FAIL; 
	}
	
	//initialize the UART hardware 
	usart_serial_init(uartConfig->p_usart, &uartConfig->uart_options);
	
	//setup interrupts for the UART
	/* Disable all the interrupts. */
	usart_disable_interrupt(uartConfig->p_usart, ALL_INTERRUPT_MASK);
	/* Configure and enable interrupt of USART. */
	if(uartConfig->p_usart == UART0)
	{
		sysclk_enable_peripheral_clock(ID_PIOA);
		PIOA->PIO_IDR        =  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
		PIOA->PIO_PUDR       =  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
		PIOA->PIO_ABCDSR[0] &= ~(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
		PIOA->PIO_ABCDSR[1] &= ~(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
		PIOA->PIO_PDR        =  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);				
		NVIC_EnableIRQ(UART0_IRQn);
	}
	else if(uartConfig->p_usart == UART1)
	{
		sysclk_enable_peripheral_clock(ID_PIOB);
		PIOB->PIO_IDR        =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);
		PIOB->PIO_PUDR       =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);
		PIOB->PIO_ABCDSR[0] &= ~(PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);
		PIOB->PIO_ABCDSR[1] &= ~(PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);
		PIOB->PIO_PDR        =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);			
		NVIC_EnableIRQ(UART1_IRQn);
	}
	else if(uartConfig->p_usart == USART0)
	{
		/* Configure USART0 Pins (PA6 = TX, PA5 = RX). */
		sysclk_enable_peripheral_clock(ID_PIOA);
		PIOA->PIO_IDR        =  (PIO_PA5A_RXD0 | PIO_PA6A_TXD0);
		PIOA->PIO_PUDR       =  (PIO_PA5A_RXD0 | PIO_PA6A_TXD0);
		PIOA->PIO_ABCDSR[0] &= ~(PIO_PA5A_RXD0 | PIO_PA6A_TXD0 | PIO_PA2B_SCK0);
		PIOA->PIO_ABCDSR[1] &= ~(PIO_PA5A_RXD0 | PIO_PA6A_TXD0 | PIO_PA2B_SCK0);
		PIOA->PIO_PDR        =  (PIO_PA5A_RXD0 | PIO_PA6A_TXD0);
		PIOA->PIO_PUDR   =  (PIO_PA5A_RXD0 | PIO_PA6A_TXD0);
		PIOA->PIO_CODR   =  (PIO_PA5A_RXD0 | PIO_PA6A_TXD0);
		PIOA->PIO_OER    =  (PIO_PA6A_TXD0);
		PIOA->PIO_PER    =  (PIO_PA5A_RXD0 | PIO_PA6A_TXD0);			
		/* configure USART0 enable Pin (PA2) Peripheral-B */

		NVIC_EnableIRQ(USART0_IRQn);
	}
	else
	{
		//ERROR! The config settings have an invalid UART pointer
		return STATUS_FAIL;
	}
	//initialize the buffers for the UART based on which mode the driver is in. 
	if(uartConfig->uartMode == DRV_UART_MODE_BYTE_BUFFER)
	{
		uartMemBuf[uartConfig->mem_index].rx_fifo = pvPortMalloc(sizeof(sw_fifo_typedef));
		//check that the memory was properly allocated.
		if(uartMemBuf[uartConfig->mem_index].rx_fifo == NULL)
		{
			return STATUS_FAIL;
		}
		memset(uartMemBuf[uartConfig->mem_index].rx_fifo->data_buf, 0,FIFO_BUFFER_SIZE);
		uartMemBuf[uartConfig->mem_index].rx_fifo->i_first = 0;
		uartMemBuf[uartConfig->mem_index].rx_fifo->i_last = 0;
		uartMemBuf[uartConfig->mem_index].rx_fifo->num_bytes = 0;
		//allocate the memory for the normal TX buffer
		uartMemBuf[uartConfig->mem_index].tx_fifo = pvPortMalloc(sizeof(sw_fifo_typedef));
		//check that the memory was properly allocated.
		if(uartMemBuf[uartConfig->mem_index].tx_fifo == NULL)
		{
			return STATUS_FAIL;
		}
		//set the packet structure pointer to null
		uartMemBuf[uartConfig->mem_index].packet_fifo = NULL;
						
	}
	else if(uartConfig->uartMode == DRV_UART_MODE_PACKET_PARSER)
	{
		uartMemBuf[uartConfig->mem_index].packet_fifo = pvPortMalloc(sizeof(packet_fifo_t));
		//check that the memory was properly allocated. 
		if(uartMemBuf[uartConfig->mem_index].packet_fifo == NULL)
		{
			return STATUS_FAIL;
		}
		//initialize the memory to zero
		memset(uartMemBuf[uartConfig->mem_index].packet_fifo, 0,sizeof(packet_fifo_t));
		//assign the callback function to the memory index. 
		uartMemBuf[uartConfig->mem_index].packetCallback = uartConfig->packetCallback; 
		//allocate the memory for the normal TX buffer
		uartMemBuf[uartConfig->mem_index].tx_fifo = pvPortMalloc(sizeof(sw_fifo_typedef));
		//check that the memory was properly allocated.
		if(uartMemBuf[uartConfig->mem_index].tx_fifo == NULL)
		{
			return STATUS_FAIL;
		}
		//set the RX fifo to null
		uartMemBuf[uartConfig->mem_index].rx_fifo = NULL;			
	}
	
	
	uartMemBuf[uartConfig->mem_index].isinit = true;
	usart_enable_interrupt(uartConfig->p_usart, UART_IER_RXRDY | UART_SR_TXEMPTY); //enable RXRDY interrupt	
	
	//clear the buffer

	
	uartMemBuf[uartConfig->mem_index].uart_rx_fifo_full_flag = 0;
	uartMemBuf[uartConfig->mem_index].uart_rx_fifo_not_empty_flag = 0;
	uartMemBuf[uartConfig->mem_index].uart_rx_fifo_ovf_flag = 0;
	

	memset(uartMemBuf[uartConfig->mem_index].tx_fifo->data_buf, 0,FIFO_BUFFER_SIZE);
	uartMemBuf[uartConfig->mem_index].tx_fifo->i_first = 0;
	uartMemBuf[uartConfig->mem_index].tx_fifo->i_last = 0;	
	uartMemBuf[uartConfig->mem_index].tx_fifo->num_bytes = 0;
	uartMemBuf[uartConfig->mem_index].uart_tx_fifo_full_flag = 0;
	uartMemBuf[uartConfig->mem_index].uart_tx_fifo_not_empty_flag = 0;
	uartMemBuf[uartConfig->mem_index].uart_tx_fifo_ovf_flag = 0;
	return status; 
}
/***********************************************************************************************
 * drv_uart_putChar(drv_uart_config_t* uartConfig, char c)
 * @brief sends a char through the configured uart
 * @param uartConfig, the configuration structure for the uart, used to identify where to send 
 * the character
 * @param c, the character that will be sent through the uart. 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/	
status_t drv_uart_putChar(drv_uart_config_t* uartConfig, char c)
{
	status_t status = STATUS_PASS;
	//disable the interrupts so we don't fuck up the pointers	
	//taskENTER_CRITICAL();
	usart_disable_interrupt(uartConfig->p_usart, UART_IER_TXEMPTY);	
	uint32_t val = 0;
	drv_uart_memory_buf_t* memBuf = &uartMemBuf[uartConfig->mem_index]; 
	if(memBuf->tx_fifo->num_bytes == FIFO_BUFFER_SIZE) // if the sw buffer is full
	{
		memBuf->uart_tx_fifo_ovf_flag = 1;                     // set the overflow flag
		memBuf->uart_tx_fifo_dropped_bytes++; //our data stream will be out of sync now...	
	}
	else if(memBuf->tx_fifo->num_bytes < FIFO_BUFFER_SIZE)  // if there's room in the sw buffer
	{
		memBuf->tx_fifo->data_buf[memBuf->tx_fifo->i_last] = c;
		memBuf->tx_fifo->i_last++;                              // increment the index of the most recently added element
		memBuf->tx_fifo->num_bytes++;                           // increment the bytes counter

	}
	if(memBuf->tx_fifo->num_bytes == FIFO_BUFFER_SIZE)
	{      // if sw buffer just filled up
		memBuf->uart_tx_fifo_full_flag = 1;                    // set the tx FIFO full flag
	}
	if(memBuf->tx_fifo->i_last == FIFO_BUFFER_SIZE)
	{         // if the index has reached the end of the buffer,
		memBuf->tx_fifo->i_last = 0;                            // roll over the index counter
	}
	memBuf->uart_tx_fifo_not_empty_flag = 1;                 // set tx-data ready flag	
	//taskEXIT_CRITICAL();
	//re-enable the interrupts
	usart_enable_interrupt(uartConfig->p_usart, UART_IER_TXEMPTY);		
	return status;	
}
/***********************************************************************************************
 * drv_uart_getChar(drv_uart_config_t* uartConfig, char* c)
 * @brief reads a char from the buffer
 * @param uartConfig, the configuration structure for the uart, used to identify where to read 
 * the character from
 * @param *c, the return character
 * @return STATUS_PASS if successful, STATUS_EOF if there were no bytes to read
 *	STATUS_FAIL if there is an error
 ***********************************************************************************************/	
status_t drv_uart_getChar(drv_uart_config_t* uartConfig, char* c)
{
	status_t status = STATUS_PASS;
	
	//If the rx_fifo doesn't exist don't even try to use this function. 
	if(uartMemBuf[uartConfig->mem_index].rx_fifo == NULL)
	{
	}
	if(uartMemBuf[uartConfig->mem_index].uart_rx_fifo_not_empty_flag == 1) //check if the buffer has information in it
	{
		usart_disable_interrupt(uartConfig->p_usart, UART_IER_RXRDY);
		//disable the interrupts so we don't fuck up the pointers		
		status = uart_get_byte(&(uartMemBuf[uartConfig->mem_index]), c); //get the byte from the buffer	
		//re-enable the interrupts
		usart_enable_interrupt(uartConfig->p_usart, UART_IER_RXRDY);
	}
	else
	{
		status = STATUS_EOF; //there's no data return End Of File status code.
	}
	return status;	
}
/***********************************************************************************************
 * drv_uart_getRawPacket(drv_uart_config_t* uartConfig, drv_uart_rawPacket_t* packet)
 * @brief reads a char from the buffer
 * @param uartConfig, the configuration structure for the uart, used to identify where to read 
 * the character from
 * @param *c, the returned packet, the memory for the packet must already be allocated
 * @return STATUS_PASS if successful, STATUS_EOF if there were no packets to read
 *	STATUS_FAIL if there is an error
 ***********************************************************************************************/	
status_t drv_uart_getRawPacket(drv_uart_config_t* uartConfig, drv_uart_rawPacket_t* packet)
{
	status_t status = STATUS_PASS;
	
	//If the packet fifo doesn't exist don't even try to use this function. 
	if(uartMemBuf[uartConfig->mem_index].packet_fifo == NULL)
	{
		return STATUS_FAIL; 
	}
	if(uartMemBuf[uartConfig->mem_index].uart_rx_fifo_not_empty_flag == 1) //check if the buffer has information in it
	{
		usart_disable_interrupt(uartConfig->p_usart, UART_IER_RXRDY);
		//disable the interrupts so we don't fuck up the pointers		
		status = uart_getPacket(&(uartMemBuf[uartConfig->mem_index]), packet); //get the byte from the buffer	
		//re-enable the interrupts
		usart_enable_interrupt(uartConfig->p_usart, UART_IER_RXRDY);
	}
	else
	{
		status = STATUS_EOF; //there's no data return End Of File status code.
	}
	
	return status;	
}
/***********************************************************************************************
 * drv_uart_deInit(drv_uart_config_t* uartConfig)
 * @brief puts the uart driver in a deinitialized state
 * @param uartConfig, the configuration structure for the uart, 
 * @return STATUS_PASS if successful,	STATUS_FAIL if there is an error
 ***********************************************************************************************/	
status_t drv_uart_deInit(drv_uart_config_t* uartConfig)
{
	status_t status = STATUS_PASS;
	/* Disable all the interrupts. */
	usart_disable_interrupt(uartConfig->p_usart, ALL_INTERRUPT_MASK);
	uart_disable(uartConfig->p_usart);
	uartMemBuf[uartConfig->mem_index].isinit = false;		
	unsigned long PinFlag = PIO_TYPE_PIO_OUTPUT_0;
	if(uartConfig->p_usart == UART0)
	{
		sysclk_disable_peripheral_clock(ID_UART0);

		//enable the pin
		PIOA->PIO_PDR    |=  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);			
		PIOA->PIO_PUDR   |=  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
		PIOA->PIO_CODR   |=  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
		//disable pin as output
		PIOA->PIO_ODR    |=  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
		//re-enable the pin
		PIOA->PIO_PER    |=  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);	
		pio_configure_pin(PIO_PA9_IDX, PinFlag);
		pio_configure_pin(PIO_PA10_IDX, PinFlag);
	}
	else if(uartConfig->p_usart == UART1)
	{
		sysclk_disable_peripheral_clock(ID_UART1);
		PIOB->PIO_PDR    =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);		
		PIOB->PIO_PUDR   =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);
		PIOB->PIO_CODR   =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);
		PIOB->PIO_ODR    =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);
		PIOB->PIO_PER    =  (PIO_PB2A_URXD1 | PIO_PB3A_UTXD1);		
		pio_configure_pin(PIO_PB2_IDX, PinFlag);
		pio_configure_pin(PIO_PB3_IDX, PinFlag);		
	}
	//free the memory used
	if(uartMemBuf[uartConfig->mem_index].rx_fifo != NULL)
	{
		vPortFree(uartMemBuf[uartConfig->mem_index].rx_fifo);
		uartMemBuf[uartConfig->mem_index].rx_fifo = NULL;
	}
	if(uartMemBuf[uartConfig->mem_index].packet_fifo != NULL)
	{
		vPortFree(uartMemBuf[uartConfig->mem_index].packet_fifo);
		uartMemBuf[uartConfig->mem_index].packet_fifo = NULL;
	}	
	return status;	
}
/***********************************************************************************************
 * drv_uart_isInit(drv_uart_config_t* uartConfig)
 * @brief returns the initialized status of the uart
 * @param uartConfig, the configuration structure for the uart
 * @return STATUS_PASS if the driver is initialized,	STATUS_FAIL if not
 ***********************************************************************************************/	
status_t drv_uart_isInit(drv_uart_config_t* uartConfig)
{
	if(uartMemBuf[uartConfig->mem_index].isinit)
	{
		return STATUS_PASS;
	}
	else
	{
		return STATUS_FAIL; 
	}
}
uint32_t drv_uart_getDroppedBytes(drv_uart_config_t* uartConfig)
{
	return uartMemBuf[uartConfig->mem_index].uart_rx_fifo_dropped_bytes;
}
/***********************************************************************************************
 * drv_uart_getline(drv_uart_config_t* uartConfig, char* str, size_t strSize)
 * @brief returns a string that is terminated with a \n (waits indefinetly) 
 * @param uartConfig the configuration structure for the uart
 * @param str the pointer to the buffer where the string will be stored
 * @param strSize the size of the buffer that can be used to store the string
 * @return STATUS_PASS if a string is returned,	STATUS_FAIL if the string found is larger than the buffer. 
 ***********************************************************************************************/	
status_t drv_uart_getline(drv_uart_config_t* uartConfig, char* str, size_t strSize)
{
	status_t result = STATUS_PASS;
	char val;
	int pointer = 0;
	while(1) //TODO add timeout
	{
		result = drv_uart_getChar(uartConfig,&val);
		if(result != STATUS_EOF && val != NULL)
		{
			if(pointer < strSize)
			{
				str[pointer++] = val; //add the result;
				if(val == '\n')
				{
					str[pointer] = NULL; //terminate the string
					result = STATUS_PASS;
					pointer = 0; //reset the pointer.
					break;
				}
			}
			else
			{
				//we overwrote the buffer
				result = STATUS_FAIL;
				str[strSize - 1] = NULL; //terminate what's in the buffer.
				pointer = 0;
				break;
			}
			//vTaskDelay(1);
		}
		else
		{
			//taskYIELD(); 
			vTaskDelay(1); //let the other processes do stuff	
		}
		
	}
	return result; 
}
/***********************************************************************************************
 * drv_uart_getlineTimed(drv_uart_config_t* uartConfig, char* str, size_t strSize, uint32_t maxTime)
 * @brief returns a string that is terminated with a \n (waits indefinetly) 
 * @param uartConfig the configuration structure for the uart
 * @param str the pointer to the buffer where the string will be stored
 * @param strSize the size of the buffer that can be used to store the string
 * @param maxTime the maximum time in ticks the function should wait for the response. 
 * @return STATUS_PASS if a string is returned,	STATUS_FAIL if the string found is larger than the buffer, or timed out
 ***********************************************************************************************/	
status_t drv_uart_getlineTimed(drv_uart_config_t* uartConfig, char* str, size_t strSize, uint32_t maxTime)
{
	status_t result = STATUS_PASS;
	char val;
	int pointer = 0;
	uint32_t startTime = sgSysTickCount;
	while(1) //TODO add timeout
	{
		result = drv_uart_getChar(uartConfig,&val);
		if(result != STATUS_EOF && val != NULL)
		{
			if(pointer < strSize)
			{
				str[pointer++] = val; //add the result;
				if(val == '\n')
				{
					str[pointer] = NULL; //terminate the string
					result = STATUS_PASS;
					pointer = 0; //reset the pointer.
					break;
				}
			}
			else
			{
				//we overwrote the buffer
				result = STATUS_FAIL;
				str[strSize - 1] = NULL; //terminate what's in the buffer.
				pointer = 0;
				break;
			}
		}
		else
		{
			//check if we've timed out yet... 
			if(sgSysTickCount > (startTime + maxTime))
			{
				//return fail, we've timed out. 
				result = STATUS_FAIL; 
				break;
			}
			vTaskDelay(1); //let the other processes do stuff	
		}
		
	}
	return result; 
}
/***********************************************************************************************
 * drv_uart_getPacketTimed(drv_uart_config_t* uartConfig, char* str, size_t strSize, uint32_t maxTime)
 * @brief returns a completed raw packet, waits for maxTime miliseconds
 * @param uartConfig the configuration structure for the uart
 * @param packet the pointer to the raw packet where the it will be stored. 
 * @param maxTime the maximum time in ticks the function should wait for the response. 
 * @return STATUS_PASS if a packet is returned,	STATUS_FAIL if the packet is found or timed out
 ***********************************************************************************************/	
status_t drv_uart_getPacketTimed(drv_uart_config_t* uartConfig, drv_uart_rawPacket_t* packet, uint32_t maxTime)
{
	status_t result = STATUS_PASS;
	char val;
	int pointer = 0;
	uint32_t startTime = sgSysTickCount;
	while(1) 
	{
		result = drv_uart_getRawPacket(uartConfig,packet); 
		if(result != STATUS_EOF)
		{
			//we got a packet, or something is wrong, break the loop.
			break;
		}		
		else
		{
			//check if we've timed out yet... 
			if(sgSysTickCount > (startTime + maxTime))
			{
				//return fail, we've timed out. 
				result = STATUS_FAIL; 
				break;
			}
			vTaskDelay(1); //let the other processes do stuff	
		}
		
	}
	return result; 
}
/***********************************************************************************************
 * drv_uart_getlineTimedSized(drv_uart_config_t* uartConfig, char* str, size_t strSize, uint32_t maxTime)
 * @brief returns a string that is terminated with a \n (waits indefinetly) 
 * @param uartConfig the configuration structure for the uart
 * @param str the pointer to the buffer where the string will be stored
 * @param strSize the size of the buffer that can be used to store the string
 * @param maxTime the maximum time in ticks the function should wait for the response. 
 * @param strLength the size of string received on the UART
 * @return STATUS_PASS if a string is returned,	STATUS_FAIL if the string found is larger than the buffer, or timed out
 ***********************************************************************************************/	
status_t drv_uart_getlineTimedSized(drv_uart_config_t* uartConfig, char* str, size_t strSize, uint32_t maxTime, uint8_t* strLength)
{
	status_t result = STATUS_PASS;
	char val;
	int pointer = 0;
	uint32_t startTime = sgSysTickCount; 
	while(1) //TODO add timeout
	{
		result = drv_uart_getChar(uartConfig,&val);
		if(result != STATUS_EOF && val != NULL)
		{
			if(pointer < strSize)
			{
				str[pointer++] = val; //add the result;
				if(val == '\n')
				{
					str[pointer] = NULL; //terminate the string
					result = STATUS_PASS;
					//pointer = 0; //reset the pointer.
					break;
				}
			}
			else
			{
				//we overwrote the buffer
				result = STATUS_FAIL;
				str[strSize - 1] = NULL; //terminate what's in the buffer.
				//pointer = 0;
				break;
			}
		}
		else
		{
			//check if we've timed out yet... 
			if(sgSysTickCount > (startTime + maxTime))
			{
				//return fail, we've timed out. 
				result = STATUS_FAIL; 
				break;
			}
			vTaskDelay(1); //let the other processes do stuff	
		}
		
	}
	*strLength = pointer;
	return result; 
}

void drv_uart_putString(drv_uart_config_t* uartConfig, char* str)
{
	int i=0;
	int size = strlen(str);

	for (i=0;i<size;)
	{
		if(drv_uart_putChar(uartConfig, str[i]) == STATUS_PASS)
		{
			i++; //increment only if PASS is returned (it means the data has been sent)
		}		
	}
}
void drv_uart_putData(drv_uart_config_t* uartConfig, char* str, size_t length)
{
	int i=0;
	for (i=0;i<length;)
	{
		if(drv_uart_putChar(uartConfig, str[i]) == STATUS_PASS)
		{
			i++; //increment only if PASS is returned (it means the data has been sent)
		}
	}
}
uint32_t drv_uart_getNumBytes(drv_uart_config_t* uartConfig)
{
	//clear the buffer
	if(drv_uart_isInit(uartConfig) == STATUS_PASS)
	{
		return uartMemBuf[uartConfig->mem_index].rx_fifo->num_bytes;
	}
	return 0;
}

void drv_uart_flushRx(drv_uart_config_t* uartConfig)
{
	//clear the buffer
	if(drv_uart_isInit(uartConfig) == STATUS_PASS)
	{	
		usart_disable_interrupt(uartConfig->p_usart, UART_IER_RXRDY);
		//disable the interrupts so we don't fuck up the pointers				
		memset(uartMemBuf[uartConfig->mem_index].rx_fifo->data_buf, 0,FIFO_BUFFER_SIZE);
		uartMemBuf[uartConfig->mem_index].rx_fifo->i_first = 0;
		uartMemBuf[uartConfig->mem_index].rx_fifo->i_last = 0;
		uartMemBuf[uartConfig->mem_index].rx_fifo->num_bytes = 0;
		uartMemBuf[uartConfig->mem_index].uart_rx_fifo_full_flag = 0;
		uartMemBuf[uartConfig->mem_index].uart_rx_fifo_not_empty_flag = 0;
		uartMemBuf[uartConfig->mem_index].uart_rx_fifo_ovf_flag = 0;		
		//flush the TX buffer as well. 
		uartMemBuf[uartConfig->mem_index].tx_fifo->i_first = 0;
		uartMemBuf[uartConfig->mem_index].tx_fifo->i_last = 0;
		uartMemBuf[uartConfig->mem_index].tx_fifo->num_bytes = 0;
		uartMemBuf[uartConfig->mem_index].uart_tx_fifo_full_flag = 0;
		uartMemBuf[uartConfig->mem_index].uart_tx_fifo_not_empty_flag = 0;
		uartMemBuf[uartConfig->mem_index].uart_tx_fifo_ovf_flag = 0;
		//re-enable the interrupts		
		usart_enable_interrupt(uartConfig->p_usart, UART_IER_RXRDY);	
	}
}

// interrupt handlers
void UART0_Handler()
{	
	//if(uartMemBuf[0].isinit) //only handle the interrupt if the driver is initialized. 
	//{
		//uart_process_byte(UART0, &(uartMemBuf[0]));	
	//}
	uint32_t status = uart_get_status(UART0); 
	if(status & UART_SR_RXRDY > 0)
	{	
		if(uartMemBuf[0].isinit) //only handle the interrupt if the driver is initialized.
		{
			uart_process_byte(UART0, &(uartMemBuf[0]));
		}
	}
	
	if(status & UART_SR_TXEMPTY)
	{
		if(uartMemBuf[0].isinit) //only handle the interrupt if the driver is initialized.
		{
			uart_process_tx_byte(UART0, &(uartMemBuf[0])); 	
		}
	}	
}

void UART1_Handler()
{
	uint32_t status = uart_get_status(UART1); 
	if(status & UART_SR_RXRDY > 0)
	{	
		if(uartMemBuf[1].isinit) //only handle the interrupt if the driver is initialized.
		{
			uart_process_byte(UART1, &(uartMemBuf[1]));
			
		}
	}
	
	if(status & UART_SR_TXEMPTY)
	{
		if(uartMemBuf[1].isinit) //only handle the interrupt if the driver is initialized.
		{
			uart_process_tx_byte(UART1, &(uartMemBuf[1])); 	
		}
	}
}

void USART0_Handler()
{
	//if(uartMemBuf[2].isinit) //only handle the interrupt if the driver is initialized.
	//{
		//uart_process_byte(USART0, &(uartMemBuf[2]));
	//}
	uint32_t status = uart_get_status(USART0); 
	if(status & UART_SR_RXRDY > 0)
	{	
		if(uartMemBuf[2].isinit) //only handle the interrupt if the driver is initialized.
		{
			uart_process_byte(USART0, &(uartMemBuf[2]));
		}
	}
	
	if(status & UART_SR_TXEMPTY)
	{
		if(uartMemBuf[2].isinit) //only handle the interrupt if the driver is initialized.
		{
			uart_process_tx_byte(USART0, &(uartMemBuf[2])); 	
		}
	}	
}

void USART1_Handler()
{
	//if(uartMemBuf[3].isinit) //only handle the interrupt if the driver is initialized.
	//{
		//uart_process_byte(USART1, &(uartMemBuf[3]));
	//}
// 	uint32_t status = uart_get_status(USART1); 
// 	if(status & UART_SR_RXRDY > 0)
// 	{	
// 		if(uartMemBuf[3].isinit) //only handle the interrupt if the driver is initialized.
// 		{
// 			uart_process_byte(USART1, &(uartMemBuf[3]));
// 		}
// 	}
// 	
// 	if(status & UART_SR_TXEMPTY)
// 	{
// 		if(uartMemBuf[3].isinit) //only handle the interrupt if the driver is initialized.
// 		{
// 			uart_process_tx_byte(USART1, &(uartMemBuf[3])); 	
// 		}
// 	}	
}

//static functions
static int uart_get_byte(drv_uart_memory_buf_t* memBuf, char* c)
{
	status_t status = STATUS_PASS;
	if(memBuf->rx_fifo->num_bytes == FIFO_BUFFER_SIZE)
	{ // if the sw buffer is full
		memBuf->uart_rx_fifo_full_flag = 0;               // clear the buffer full flag because we are about to make room
	}
	if(memBuf->rx_fifo->num_bytes > 0)
	{
		// if data exists in the sw buffer
		*c = memBuf->rx_fifo->data_buf[memBuf->rx_fifo->i_first]; // grab the oldest element in the buffer
		memBuf->rx_fifo->i_first++;                        // increment the index of the oldest element
		memBuf->rx_fifo->num_bytes--;                      // decrement the bytes counter
	}
	else
	{                                      // RX sw buffer is empty
		memBuf->uart_rx_fifo_not_empty_flag = 0;          // clear the rx flag
		status = STATUS_EOF;
	}
	if(memBuf->rx_fifo->i_first == FIFO_BUFFER_SIZE)
	{   // if the index has reached the end of the buffer,
		memBuf->rx_fifo->i_first = 0;                      // roll over the index counter
	}		
	return status;
}

static status_t uart_getPacket(drv_uart_memory_buf_t* memBuf, drv_uart_rawPacket_t* packet)
{
	status_t status = STATUS_PASS;
	if(memBuf->packet_fifo == NULL)
	{
		return STATUS_FAIL; 
	}
	//clear the full flag
	if(memBuf->packet_fifo->num_packets == MAX_NUMBER_RAW_PACKETS)
	{
		memBuf->uart_rx_fifo_full_flag = 0;
	}
	if(memBuf->packet_fifo->num_packets > 0)
	{
		//copy the packet to the pointer
		memcpy(packet,&(memBuf->packet_fifo->packetArray[memBuf->packet_fifo->i_first++]),sizeof(drv_uart_rawPacket_t));
		memBuf->packet_fifo->num_packets--;
		if(memBuf->packet_fifo->i_first == FIFO_BUFFER_SIZE)
		{   // if the index has reached the end of the buffer,
			memBuf->packet_fifo->i_first = 0;                      // roll over the index counter
		}
	}
	else
	{
		memBuf->uart_rx_fifo_not_empty_flag = 0;          // clear the rx flag
		status = STATUS_EOF;		
	}
	return status;
}

static status_t uart_processPacketByte(uint8_t byte, drv_uart_memory_buf_t* memBuf)
{
	//if byte is start byte
	status_t status = STATUS_EAGAIN;
	int currentIndex = memBuf->packet_fifo->i_last;
	if(memBuf->packet_fifo->num_packets == MAX_NUMBER_RAW_PACKETS)
	{
		//buffers full
		return STATUS_FAIL; 
	}
	if(byte == RAW_PACKET_START_BYTE)
	{
		if(memBuf->packet_fifo->packetArray[memBuf->packet_fifo->i_last].bytesReceived > 0)
		{
			//this means there was an error receiving a packet
			memBuf->uart_rx_fifo_dropped_bytes++;
			//status = STATUS_FAIL;
		}
		//reset the counts and everything for reception of the packet
		memBuf->packet_fifo->packetArray[currentIndex].bytesReceived = 0;
		memBuf->packet_fifo->packetArray[currentIndex].escapeFlag = false;
		memBuf->packet_fifo->packetArray[currentIndex].payloadSize = 0;
		return status;
	}
	//if byte is escape byte
	if(byte == RAW_PACKET_ESCAPE_BYTE)
	{
		//set escape flag, so the next byte is properly offset.
		memBuf->packet_fifo->packetArray[currentIndex].escapeFlag = true;
		return status;
	}
	//if escape byte flag is set
	if(memBuf->packet_fifo->packetArray[currentIndex].escapeFlag == true)
	{
		//un-escape the byte and process it as any other byte.
		byte = byte - RAW_PACKET_ESCAPE_OFFSET;
		//unset the flag
		memBuf->packet_fifo->packetArray[currentIndex].escapeFlag = false;
	}
	
	//if receive count is  0
	if(memBuf->packet_fifo->packetArray[currentIndex].bytesReceived == 0)
	{
		//this is the first byte of the payload size
		//copy byte to LSB of the payload size
		memBuf->packet_fifo->packetArray[currentIndex].payloadSize |= (uint16_t)byte;
		//increment received count
		memBuf->packet_fifo->packetArray[currentIndex].bytesReceived++;
	}
	else if(memBuf->packet_fifo->packetArray[currentIndex].bytesReceived == 1)
	{
		//this is the second byte of the payload size
		//copy byte to MSB of the payload size
		memBuf->packet_fifo->packetArray[currentIndex].payloadSize |= (uint16_t)(byte<<8);
		//increment received count
		memBuf->packet_fifo->packetArray[currentIndex].bytesReceived++;
	}
	else
	{	//copy byte to payload at point receivedBytes - 2
		memBuf->packet_fifo->packetArray[currentIndex].payload[memBuf->packet_fifo->packetArray[currentIndex].bytesReceived - 2] = byte;
		//check if we received the whole packet.
		if(memBuf->packet_fifo->packetArray[currentIndex].bytesReceived-1 == memBuf->packet_fifo->packetArray[currentIndex].payloadSize)
		{
			//set the status to pass, we have the packet!
			status = STATUS_PASS;
			if(memBuf->packetCallback != NULL)
			{
				//transfer the packet
				memBuf->packetCallback(&memBuf->packet_fifo->packetArray[currentIndex]);	
			}			
			//reset bytes received to zero			
			memBuf->packet_fifo->packetArray[currentIndex].bytesReceived = 0;
			//increment the circular packet buffer
			memBuf->packet_fifo->i_last++;                              // increment the index of the most recently added element
			memBuf->packet_fifo->num_packets++;                         // increment the bytes counter
			
			if(memBuf->packet_fifo->num_packets == MAX_NUMBER_RAW_PACKETS)
			{
				memBuf->uart_rx_fifo_full_flag = 1; 	
			}
			if(memBuf->packet_fifo->i_last == MAX_NUMBER_RAW_PACKETS)
			{
				memBuf->packet_fifo->i_last = 0;				
			}
			memBuf->uart_rx_fifo_not_empty_flag = 1;
		}
		else
		{
			memBuf->packet_fifo->packetArray[currentIndex].bytesReceived++;
		}
	}
	return status;
}
static void uart_process_byte(Usart *p_usart, drv_uart_memory_buf_t* memBuf)
{
	uint32_t val = 0;
	if(memBuf->rx_fifo != NULL)
	{	
		if(memBuf->rx_fifo->num_bytes == FIFO_BUFFER_SIZE) // if the sw buffer is full
		{
			memBuf->uart_rx_fifo_ovf_flag = 1;                     // set the overflow flag
			//the buffer is full, but we have to clear the interrupt
			memBuf->uart_rx_fifo_dropped_bytes++; //our data stream will be out of sync now...
			usart_getchar(p_usart,&val);		
		}
		else if(memBuf->rx_fifo->num_bytes < FIFO_BUFFER_SIZE)  // if there's room in the sw buffer
		{		
			if(usart_getchar(p_usart,&val) == STATUS_PASS)
			{		
				memBuf->rx_fifo->data_buf[memBuf->rx_fifo->i_last] = (uint8_t)val;
				memBuf->rx_fifo->i_last++;                              // increment the index of the most recently added element
				memBuf->rx_fifo->num_bytes++;                           // increment the bytes counter
			}
		}
		if(memBuf->rx_fifo->num_bytes == FIFO_BUFFER_SIZE)
		{      // if sw buffer just filled up
			memBuf->uart_rx_fifo_full_flag = 1;                    // set the RX FIFO full flag
		}
		if(memBuf->rx_fifo->i_last == FIFO_BUFFER_SIZE)
		{         // if the index has reached the end of the buffer,
			memBuf->rx_fifo->i_last = 0;                            // roll over the index counter
		}
		memBuf->uart_rx_fifo_not_empty_flag = 1;                 // set received-data flag
	}
	else if(memBuf->packet_fifo != NULL)
	{
		if(usart_getchar(p_usart,&val) == STATUS_PASS)
		{
			uart_processPacketByte((uint8_t)val,memBuf); 
		}
	}
}

static void uart_process_tx_byte(Usart *p_usart, drv_uart_memory_buf_t* memBuf)
{
	if(memBuf->tx_fifo->num_bytes == FIFO_BUFFER_SIZE)
	{ // if the sw buffer is full
		memBuf->uart_tx_fifo_full_flag = 0;               // clear the buffer full flag because we are about to make room
	}
	if(memBuf->tx_fifo->num_bytes > 0)
	{
		// if data exists in the sw buffer
		usart_serial_putchar(p_usart,memBuf->tx_fifo->data_buf[memBuf->tx_fifo->i_first]); // send the next value from buffer
		memBuf->tx_fifo->i_first++;                        // increment the index of the oldest element
		memBuf->tx_fifo->num_bytes--;                      // decrement the bytes counter
	}
	else
	{   // tx sw buffer is empty
		memBuf->uart_tx_fifo_not_empty_flag = 0;          // clear the tx flag
		uart_disable_interrupt(p_usart,UART_IER_TXEMPTY); //the buffer is empty, stop the interrupt. 
	}
	if(memBuf->tx_fifo->i_first == FIFO_BUFFER_SIZE)
	{   // if the index has reached the end of the buffer,
		memBuf->tx_fifo->i_first = 0;                      // roll over the index counter
	}	
};