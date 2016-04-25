/*
 * cmd_commandProc.c
 *
 * Created: 3/1/2016 9:00:18 AM
 *  Author: sean
 */ 
#include <asf.h>
#include <string.h>
#include <assert.h>
#include "cmd_commandProc.h"
#include "dat_dataRouter.h"
#include "common.h"
#include "LTC2941-1.h"
xQueueHandle cmd_queue_commandQueue = NULL;
extern slave_twi_config_t ltc2941Config; 
extern dat_dataRouterConfig_t dataRouterConfiguration;
//static function forward declarations
static void setTimeFromString(char* dateTime);

char tempString[255] = {0}; 

/***********************************************************************************************
 * cmd_task_commandProcesor(void *pvParameters)
 * @brief This task receives all incoming commands to the brain pack, and responds to the ones it needs to.   
 * @param pvParameters, void pointer to structure containing configuration
 * @return void
 ***********************************************************************************************/
void cmd_task_commandProcesor(void *pvParameters)
{
	cmd_queue_commandQueue = xQueueCreate( 10, sizeof(cmd_commandPacket_t));
	cmd_commandPacket_t packet; 
	cmd_initPacketStructure(&packet);
	uint16_t chargeLevel = 0; 
	uint32_t chargeRegValue = 0; 
	bool forwardCommand = true; 
	if(cmd_queue_commandQueue == 0)
	{
		// Queue was not created this is an error!		
		return;
	}
	while(1)
	{	
		if(xQueueReceive( cmd_queue_commandQueue, &(packet), 1000) == TRUE)
		{
			//only a small subset of commands are handled on the power board
			//send the rest to the data board for processing.
			if(packet.packetSize > 0)
			{		
				forwardCommand = true; //by default always forward the command to the databoard
				if(strncmp(packet.packetData,"setTime",7)==0)
				{
					//handle the set time command. 
					if(packet.packetSource != CMD_COMMAND_SOURCE_LOCAL)
					{					
						if(strlen(packet.packetData) >= 24)
						{
							setTimeFromString(packet.packetData+7);
						}
					}
				}
				else if(strncmp(packet.packetData,"getRawCharge",9)==0)
				{
					//handle the set time command. 					
					chargeLevel = ltc2941GetCharge(&ltc2941Config);
					sprintf(tempString,"raw charge Level: %d\r\n",chargeLevel);
					if(packet.packetSource == CMD_COMMAND_SOURCE_DAUGHTER)
					{
						drv_uart_putString(dataRouterConfiguration.daughterBoard, tempString);
					}
					else if(packet.packetSource == CMD_COMMAND_SOURCE_USB)
					{
						dat_sendStringToUsb(tempString);
					}
					//don't forward this message on. 	
					forwardCommand = false; 						
				}		
				else if(strncmp(packet.packetData,"getCharge",9)==0)
				{
					//handle the set time command. 					
					chargeLevel = getCalculatedPercentage(&ltc2941Config);
					sprintf(tempString,"charge Level: %d\r\n",chargeLevel);
					if(packet.packetSource == CMD_COMMAND_SOURCE_DAUGHTER)
					{
						drv_uart_putString(dataRouterConfiguration.daughterBoard, tempString);
					}
					else if(packet.packetSource == CMD_COMMAND_SOURCE_USB)
					{
						dat_sendStringToUsb(tempString);
					}
					//don't forward this message on. 	
					forwardCommand = false; 						
				}		
				else if(strncmp(packet.packetData,"setChargeLow",12)==0)
				{					
					chargeRegValue = getRegValueForPercent(14);					
					ltc2941SetCharge(&ltc2941Config, chargeRegValue);
					forwardCommand = false; 	 
				}						
				else if(strncmp(packet.packetData,"setChargeCritical",17)==0)
				{
					chargeRegValue = getRegValueForPercent(7);					
					ltc2941SetCharge(&ltc2941Config,chargeRegValue);
					forwardCommand = false; 	 
				}										
				else if(strncmp(packet.packetData,"setChargeFault",14)==0)
				{
					chargeRegValue = getRegValueForPercent(4);					
					ltc2941SetCharge(&ltc2941Config, chargeRegValue);
					forwardCommand = false; 	 
				}
				else if(strncmp(packet.packetData,"setChargeFull",13)==0)
				{										
					ltc2941SetCharge(&ltc2941Config, 0xFFFF);
					forwardCommand = false; 	 
				}									
				else if(strncmp(packet.packetData,"getChrgStatus",13)==0)
				{
					//handle the set time command. 
					sprintf(tempString,"charger status: %x\r\n",ltc2941GetStatus(&ltc2941Config));
					if(packet.packetSource == CMD_COMMAND_SOURCE_DAUGHTER)
					{
						drv_uart_putString(dataRouterConfiguration.dataBoardUart, tempString);
					}
					else if(packet.packetSource == CMD_COMMAND_SOURCE_USB)
					{
						dat_sendStringToUsb(tempString);
					}						
					//don't forward this message on. 						
					forwardCommand = false; 						
				}		
				else if (strncmp(packet.packetData,"crashSystem",11)==0)
				{
					//sprintf(1234213,"crashity crash crash!%s\r\n",NULL);
					//assert(false);
					//*((unsigned int*)0) = 0xDEAD;
					//uint32_t* deadPointer = malloc(10000000000);
					//strncpy(deadPointer+4, deadPointer+6, 10000);
					memcpy(0x20000000, packet.packetData, 1000000);
										
				}
				if(forwardCommand == true)
				{
					//forward the command to the data board. 
					dat_sendPacketToDataBoard(&packet); 	
				}				
			}
		}
	}
		
}

void cmd_initPacketStructure(cmd_commandPacket_t* packet)
{
	memset(packet->packetData,0,CMD_INCOMING_CMD_SIZE_MAX);
	packet->packetSize = 0;
}

char debugMessage[100] = {0};
status_t cmd_sendDebugMsgToDataBoard(char* errorMessage)
{
	cmd_commandPacket_t packet; 
	//add prepended message to databoard packet
	snprintf(packet.packetData,CMD_INCOMING_CMD_SIZE_MAX,"PwrBrdMsg:%s", errorMessage);	
	packet.packetSize = strlen(packet.packetData); 
	packet.packetSource = CMD_COMMAND_SOURCE_LOCAL;
	if(cmd_queue_commandQueue != NULL)
	{
		if(xQueueSendToBack(cmd_queue_commandQueue,( void * ) &packet,5) != TRUE)
		{
			//this is an error, we should log it.
		}	
	}
}
status_t cmd_sendDateTimeCommand()
{
	cmd_commandPacket_t packet;
	packet.packetSource = CMD_COMMAND_SOURCE_LOCAL; 
	uint32_t hour, minute, second, year, month, day, dow; 
	rtc_get_date(RTC,&year,&month,&day, &dow); 
	rtc_get_time(RTC,&hour,&minute,&second); 
	sprintf(packet.packetData,"SetTime%d-%d-%d-%d-%d:%d:%d\r\n", year, month, day, dow, hour, minute, second ); 	
	packet.packetSize = strlen(packet.packetData); 
	if(cmd_queue_commandQueue != NULL)
	{
		if(xQueueSendToBack(cmd_queue_commandQueue,( void * ) &packet,5) != TRUE)
		{
			//this is an error, we should log it.
			return STATUS_FAIL;
		}	
	}
	return STATUS_PASS;	
	
} 
//static functions


static void setTimeFromString(char* dateTime)
{
	uint32_t year, month, day, dow; //dow is day of week (1-7)
	uint32_t hour, minute, second;
	if(sscanf(dateTime,"%d-%d-%d-%d-%d:%d:%d\r\n", &year, &month, &day, &dow, &hour, &minute, &second ) == 7)
	{
		// we successfully parsed the data, set the time and date
		rtc_set_time(RTC,hour,minute,second);
		rtc_set_date(RTC,year,month,day,dow);
	}
}
