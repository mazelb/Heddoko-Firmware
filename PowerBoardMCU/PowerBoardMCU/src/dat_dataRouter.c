/*
 * dat_dataRouter.c
 *
 * Created: 2/29/2016 4:04:56 PM
 *  Author: sean


 */ 

#include <asf.h>
#include <string.h>
#include "dat_dataRouter.h"
#include "cmd_commandProc.h"
#include "udi_cdc.h"


dat_dataRouterConfig_t* dataRouterConfig; 
extern xQueueHandle cmd_queue_commandQueue;
static bool my_flag_autorize_cdc_transfert = false;
int my_callback_cdc_enable(void)
{
	my_flag_autorize_cdc_transfert = true;
	return TRUE;
}
void my_callback_cdc_disable(void)
{
	my_flag_autorize_cdc_transfert = false;
}
void task(void)
{
	if (my_flag_autorize_cdc_transfert) {
		udi_cdc_putc('A');
		
	}
}
/***********************************************************************************************
 * dat_task_dataRouter(void *pvParameters)
 * @brief This is the main task for routing data from the data board to either the USB or 
 *	the daughter board UART 
 * @param pvParameters, void pointer to structure containing data router configuration. 
 * @return void
 ***********************************************************************************************/
void dat_task_dataRouter(void *pvParameters)
{
	dataRouterConfig = (dat_dataRouterConfig_t*)pvParameters; 
	cmd_commandPacket_t daughterBoardPacket, usbPacket;
	//initialize the packets
	cmd_initPacketStructure(&daughterBoardPacket);
	daughterBoardPacket.packetSource = CMD_COMMAND_SOURCE_DAUGHTER; 
	cmd_initPacketStructure(&usbPacket);
	usbPacket.packetSource = CMD_COMMAND_SOURCE_USB;
		
	if(drv_uart_isInit(dataRouterConfig->daughterBoard) != STATUS_PASS)
	{
		//fail!
		return; 
	}
	
	if(drv_uart_isInit(dataRouterConfig->dataBoardUart) != STATUS_PASS)
	{
		//fail!
		return;
	}

	char receivedByte = 0x00; 
	int receivedUsbData = 0x00; 
	while(1)
	{
		//try to read byte from databoard mcu
		if(drv_uart_getChar(dataRouterConfig->dataBoardUart, &receivedByte) == STATUS_PASS)
		{
			//if byte exists, pass through to the daughter board and USB (if connected)
			drv_uart_putChar(dataRouterConfig->daughterBoard, receivedByte); 
			if(udi_cdc_is_tx_ready() == true)
			{
				udi_cdc_putc(receivedByte); 
			} 
			
		}
		else
		{
			vTaskDelay(1);
		}

				
		//try to read byte from daughter board
		if(drv_uart_getChar(dataRouterConfig->daughterBoard, &receivedByte) == STATUS_PASS)
		{
			//if byte exists, pass through to the daughter board and USB (if connected)
			if(daughterBoardPacket.packetSize < CMD_INCOMING_CMD_SIZE_MAX -1) //check we have room for the command. 
			{				
				daughterBoardPacket.packetData[daughterBoardPacket.packetSize++] = receivedByte;				
				if(receivedByte == '\n')
				{
					//make sure the packet is null terminated
					daughterBoardPacket.packetData[daughterBoardPacket.packetSize] = 0x00;
					if(cmd_queue_commandQueue != NULL)
					{
						if(xQueueSendToBack(cmd_queue_commandQueue,( void * ) &daughterBoardPacket,5) != TRUE)
						{
							//this is an error, we should log it. 
						}						
					}
					//clear the packet for the next one. 
					cmd_initPacketStructure(&daughterBoardPacket);
				}
			}
			else
			{
				//the packet was too big, we should delete it, possibly log an error
				cmd_initPacketStructure(&daughterBoardPacket);
			}
		}
		//check if there's any data on the 
		if(udi_cdc_is_rx_ready() == true)
		{
			receivedUsbData = udi_cdc_getc();
			//if byte exists, pass through to the daughter board and USB (if connected)
			if(usbPacket.packetSize < CMD_INCOMING_CMD_SIZE_MAX -1) //check we have room for the command.
			{
				usbPacket.packetData[usbPacket.packetSize++] = (char)receivedUsbData;
				if((char)receivedUsbData == '\n')
				{
					//make sure the packet is null terminated
					usbPacket.packetData[usbPacket.packetSize] = 0x00;
					if(cmd_queue_commandQueue != NULL)
					{
						if(xQueueSendToBack(cmd_queue_commandQueue,( void * ) &usbPacket,5) != TRUE)
						{
							//this is an error, we should log it.
						}
					}
					//clear the packet for the next one.
					cmd_initPacketStructure(&usbPacket);
				}
			}
			else
			{
				//the packet was too big, we should delete it, possibly log an error
				cmd_initPacketStructure(&usbPacket);
			}
		}
		wdt_restart(WDT);
		//taskYIELD();
		//vTaskDelay(1);
				
		
	}	
}

status_t dat_sendPacketToDataBoard(cmd_commandPacket_t* packet)
{
	//possibly add some sort of error handling here.
	drv_uart_putData(dataRouterConfig->dataBoardUart, packet->packetData, packet->packetSize);	
	return STATUS_PASS;	
}

status_t dat_sendStringToUsb(char* str)
{	
	size_t length = strlen(str); 
	udi_cdc_write_buf(str, length); 
	return STATUS_PASS;	
}



