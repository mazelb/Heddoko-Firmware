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
 * @brief This is the main task for routing data from the data board to either the USB
 * @param pvParameters, void pointer to structure containing data router configuration. 
 * @return void
 ***********************************************************************************************/
void dat_task_dataRouter(void *pvParameters)
{
	dataRouterConfig = (dat_dataRouterConfig_t*)pvParameters;
	uint8_t dataBoardRxBuffer[RAW_PACKET_MAX_SIZE] = {0}; // maximum size of data in raw packet structure.
	uint8_t dataLength;
	cmd_commandPacket_t usbPacket;
	//mgr_eventMessage_t eventMessage; 
	//initialize the packets
	cmd_initPacketStructure(&usbPacket);
	usbPacket.packetSource = CMD_COMMAND_SOURCE_USB;
	
	if(drv_uart_isInit(dataRouterConfig->dataBoardUart) != STATUS_PASS)
	{
		//fail!
		return;
	}
	
	char receivedByte = 0x00; 
	int receivedUsbData = 0x00; 
	
	while(1)
	{	
		if (queue_dataBoard != NULL)
		{
			if (xQueueReceive(queue_dataBoard, dataBoardRxBuffer, 10) == pdPASS)
			{
				// data received should be passed to the USB
				dat_sendStringToUsb(dataBoardRxBuffer);	// TODO: can we verify that it is a string
			}
		}
						
		////check if there's any data on the 
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
	uint16_t length = strlen(debugString);
	uint8_t outputBuf[100] = {0};
	outputBuf[0] = PACKET_TYPE_SUB_PROCESSOR;
	outputBuf[1] = PACKET_COMMAND_ID_SUBP_OUTPUT_DATA;
	memcpy(&outputBuf[2], debugString, length);
    #ifndef CHARGER_TEST_MODE 
    pkt_sendRawPacket(dataRouterConfig->dataBoardUart, outputBuf, length + 2);	// 2 bytes for the header
    #endif
	
	return STATUS_PASS;
}

status_t dat_sendPacketToDataBoard(cmd_commandPacket_t* packet)	// send the command packet to the data board
{ 
	//possibly add some sort of error handling here.
	// TODO: for now it is sent as an output data, if required assign a category to it
	
	uint8_t outputBuf[110] = {0};	// the maximum size of data is 100
	outputBuf[0] = PACKET_TYPE_SUB_PROCESSOR;
	outputBuf[1] = PACKET_COMMAND_ID_SUBP_OUTPUT_DATA;
	memcpy(&outputBuf[2], packet->packetData, packet->packetSize);
	pkt_sendRawPacket(dataRouterConfig->dataBoardUart, outputBuf, packet->packetSize  + 2);	// 2 bytes for the header
	//drv_uart_putData(dataRouterConfig->dataBoardUart, packet->packetData, packet->packetSize);	//TODO: what is this, if important wrap it before sending.
	return STATUS_PASS;	
}

status_t dat_sendStringToUsb(char* str)
{	
	size_t length = strlen(str); 
	udi_cdc_write_buf(str, length); 
	return STATUS_PASS;	
}