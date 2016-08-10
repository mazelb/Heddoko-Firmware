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
#include "drv_gpio.h"
#include "pkt_packetCommandsList.h"
#include "pkt_packetParser.h"

pkt_rawPacket_t dataBoardPacket;
dat_dataRouterConfig_t* dataRouterConfig; 
extern xQueueHandle cmd_queue_commandQueue;
extern xQueueHandle queue_dataBoard;
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
	cmd_commandPacket_t usbPacket;
	//mgr_eventMessage_t eventMessage; 
	//initialize the packets
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
		/*	THE INTERACTION WITH DATA BOARD SHOULD TAKE PLACE IN ITS OWN THERAD. 
		
		//try to read byte from databoard mcu
		
		// TODO: every piece of data exchange between data board and power board should be wrapped
		
		if(drv_uart_getPacketTimed(dataRouterConfig->dataBoardUart, &dataBoardPacket, 10) == STATUS_PASS)
		{
			switch (dataBoardPacket.payload[0])
			{
				case PACKET_COMMAND_ID_SUBP_GET_STATUS:
					// populate the status structure and pass it to data board
				break;
				case PACKET_COMMAND_ID_SUBP_CONFIG:
					// configure the sub processor as requested
				break;
				case PACKET_COMMAND_ID_SUBP_STREAMING:
					// enable / disable sensor streaming
					enableStream = dataBoardPacket.payload[1];
				break;
				case PACKET_COMMAND_ID_SUBP_OUTPUT_DATA:
					// this data is supposed to be passed out on USB and/or daughter board
					
					// unwrap the data before sending it out.
					
					drv_uart_putString(dataRouterConfig->daughterBoard, &dataBoardPacket.payload[2]);
					if (udi_cdc_is_tx_ready() == true)
					{
						//send the data to USB. 
						rxDataLength = dataBoardPacket.payloadSize - 2;
						while (rxDataLength)
						{
							udi_cdc_putc(dataBoardPacket.payload[dataBoardPacket.payloadSize - rxDataLength]);
							rxDataLength--;
						}
					}
				break;
				default:
				break;
			}
		}
		else
		{
			vTaskDelay(1);
		}
		
		if(drv_uart_getChar(dataRouterConfig->dataBoardUart, &receivedByte) == STATUS_PASS)
		{
			if((receivedByte & 0xA0) == 0xA0)
			{
				if(receivedByte == POWER_BOARD_CMD_TOGGLE_JACKS)
				{
					drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_HIGH);
					drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_HIGH);
					vTaskDelay(500);
					drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW);
					drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW);
				}
				else if(receivedByte == POWER_BOARD_CMD_GET_TIME)
				{
					cmd_sendDateTimeCommand();
				}			
			}
			else
			{			
				//if byte exists, pass through to the daughter board and USB (if connected)
				drv_uart_putChar(dataRouterConfig->daughterBoard, receivedByte); 
				if(udi_cdc_is_tx_ready() == true)
				{
					udi_cdc_putc(receivedByte); 
				}
			}
			
		}
		else
		{
			vTaskDelay(1);
		}

		END OF DATA BOARD DIRECT INTERACTION PART		*/
		
		//if (xQueueReceive(queue_dataBoard, dataBoardRxBuffer, 10) == pdPASS)
		//{
			//// data received should be passed to the daughter board and USB
			//drv_uart_putString(dataRouterConfig->daughterBoard, dataBoardRxBuffer);
			//dataLength = strlen(dataBoardRxBuffer);
			//while (dataLength--)
			//{
				//if(udi_cdc_is_tx_ready() == true)
				//{
					//udi_cdc_putc(receivedByte);
				//}
			//}
		//}
						
		//try to read byte from daughter board
		//if(drv_uart_getChar(dataRouterConfig->daughterBoard, &receivedByte) == STATUS_PASS)
		//{
			////if byte exists, pass through to the daughter board and USB (if connected)
			//if(daughterBoardPacket.packetSize < CMD_INCOMING_CMD_SIZE_MAX -1) //check we have room for the command. 
			//{				
				//daughterBoardPacket.packetData[daughterBoardPacket.packetSize++] = receivedByte;				
				//if(receivedByte == '\n')
				//{
					////make sure the packet is null terminated
					//daughterBoardPacket.packetData[daughterBoardPacket.packetSize] = 0x00;
					//if(cmd_queue_commandQueue != NULL)
					//{
						//if(xQueueSendToBack(cmd_queue_commandQueue,( void * ) &daughterBoardPacket,5) != TRUE)
						//{
							////this is an error, we should log it. 
						//}						
					//}
					////clear the packet for the next one. 
					//cmd_initPacketStructure(&daughterBoardPacket);
				//}
			//}
			//else
			//{
				////the packet was too big, we should delete it, possibly log an error
				//cmd_initPacketStructure(&daughterBoardPacket);
			//}
		//}
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
		else
		{
			vTaskDelay(1);
		}
		wdt_restart(WDT);
		//taskYIELD();
		//vTaskDelay(1);
	
	}	
}
//note... for now it must be prepended with "PwrBrdMsg:"
status_t dat_sendDebugMsgToDataBoard(char* debugString)
{
	//possibly add some sort of error handling here.	
	drv_uart_putString(dataRouterConfig->dataBoardUart, debugString);	// TODO: wrap the message 
	return STATUS_PASS;
}

status_t dat_sendPacketToDataBoard(cmd_commandPacket_t* packet)
{ 
	//possibly add some sort of error handling here.
	drv_uart_putData(dataRouterConfig->dataBoardUart, packet->packetData, packet->packetSize);	//TODO: what is this, if important wrap it before sending.
	return STATUS_PASS;	
}

status_t dat_sendStringToUsb(char* str)
{	
	size_t length = strlen(str); 
	udi_cdc_write_buf(str, length); 
	return STATUS_PASS;	
}