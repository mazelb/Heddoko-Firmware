/**
* @file ble_bluetoothManager.c
* @brief This file contains all code pertaining to the functionality with the on board BLE
* Module. The interface with this module should be done explicitly through the messenger 
* module. 
* @author Sean Cloghesy (sean@heddoko.com)
* @date August 2016
* Copyright Heddoko(TM) 2016, all rights reserved
*/
#include "ble_bluetoothManager.h"
#include "common.h"
#include "drv_gpio.h"
#include "msg_messenger.h"
#include "pkt_packetParser.h"
#include "dbg_debugManager.h"
#include "pkt_packetCommandsList.h"
#include "subp_subProcessor.h"
#include "nvm_nvMemInterface.h"

/*	Local defines	*/
#define BLE_BP_STATUS_DATA_SIZE	9 
#define BLE_MAX_RAW_DATA_SIZE	20
#define SSID_DATA_SIZE			32
#define PASSPHRASE_DATA_SIZE	64
#define BLE_WIFI_DATA_SIZE		(SSID_DATA_SIZE + PASSPHRASE_DATA_SIZE + 1)	// one byte for security type enum
#define PACKET_HEADER_SIZE		2
#define BLE_INIT_PACKET_SIZE    BLE_WIFI_DATA_SIZE + 50

/*	Static function forward declarations	*/
static void processMessage(msg_message_t message);
static void processRawPacket(pkt_rawPacket_t* packet);
static void sendBpStatusData();
static void sendInitialParameters(ble_moduleConfig_t* moduleCfg);
static void sendStartFastAdv();
static void sendRawDataPacket(uint8_t *data, uint8_t size);
static void sendTimeToBleModule();
static void setTimeFromPacket(uint8_t* timeData, uint16_t length);


typedef struct 
{
    uint8_t batteryLevel;
    uint8_t chargeState;
    uint8_t sgCurrentState;
    uint8_t wiFiConnectionState;
    uint8_t sdCardState;
    uint32_t sensorMask;
}bpStatus_t;// status to be passed to BLE module.

typedef struct
{
    uint8_t ssid[SSID_DATA_SIZE];
    uint8_t passphrase[PASSPHRASE_DATA_SIZE];
    uint8_t securityType;
    uint8_t serialNumber[NVM_MAX_SUIT_NAME_SIZE];
    uint8_t fwVersion[10]; //update this to define
    uint8_t hwRevision[10];
    uint8_t modelString[20]; //the model of the brainpack
}ble_pkt_initialData_t;

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t dayOfWeek;
}ble_pkt_dateTime_t;


/*	Local variables	*/
static xQueueHandle sgQueue_ble = NULL;
static bool vNewBpStateDataAvailble = false;
static subp_status_t *sgpSubProcessorStatusData;
static uint8_t sgaRawData[BLE_MAX_RAW_DATA_SIZE];
static ble_moduleConfig_t* sgpModuleConfiguration; 
static net_wirelessConfig_t sgReceivedWifiConfig; 
static bpStatus_t sgBrainpackStatus;
drv_uart_config_t sgBleUartConfig =
{
	.p_usart = USART1,
	.mem_index = 0,
	.uart_options =
	{
		.baudrate   = 115200,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
	.mode = DRV_UART_MODE_INTERRUPT
};



/*		Function definitions	*/

/**
* @brief The Bluetooth manager task. Is executed by the free rtos
* main task and loops while the brainpack is functional. 
* Processes packets from the BLE module, and any messages from the
* other free rtos tasks. 
* @param pvParameters, pointer to the initialization variables for the task
* @return void
*/
void ble_bluetoothManagerTask(void *pvParameters)
{
	msg_message_t vReceivedMessage;
	pkt_rawPacket_t vRawPacket =
	{
		.bytesReceived = 0,
		.escapeFlag = 0,
		.payloadSize = 0
	};
	//create the message queue
	sgQueue_ble = xQueueCreate(10, sizeof(msg_message_t));
	if (sgQueue_ble != 0)
	{
		msg_registerForMessages(MODULE_BLE, 0xff, sgQueue_ble);
	}
	//initialize the UART packet receiver
	if(drv_uart_init(&sgBleUartConfig) != STATUS_PASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"failed to open USART1 for ble\r\n");
	}
    //set the reset line of the BLE module high
	drv_gpio_setPinState(DRV_GPIO_PIN_BLE_RST, DRV_GPIO_PIN_STATE_HIGH);	
	
    //load the settings
    sgpModuleConfiguration = (ble_moduleConfig_t*)pvParameters;
    vTaskDelay(5000); 
    //send the initial configuration to the BLE module
    sendInitialParameters(sgpModuleConfiguration);   
    vTaskDelay(1000);  
	sendTimeToBleModule();
	//start the main thread where we listen for packets and messages
	while (1)
	{
		if (xQueueReceive(sgQueue_ble, &vReceivedMessage, 1) == true)
		{
			processMessage(vReceivedMessage);
		}
		if(pkt_getPacketTimed(&sgBleUartConfig,&vRawPacket,10) == STATUS_PASS)
		{
			//we have a full packet
			processRawPacket(&vRawPacket);
		}
		
		if (vNewBpStateDataAvailble)
		{
			sendBpStatusData();
			vNewBpStateDataAvailble = false;
		}
		
		vTaskDelay(10);
	}
}
/**
* @brief Processes a raw packet from the BLE module. Is called from the BLE task.
* @param vPacket, pointer to the packet that must be processed
* @return void
*/
static void processRawPacket(pkt_rawPacket_t* vPacket)
{

	//check which type of packet it is.
	//All the packets should be of this type... we're not on a 485 bus.
	int i =0;
	if(vPacket->payload[0] == PACKET_TYPE_BLE_MODULE)
	{
		switch(vPacket->payload[1])
		{
			case PACKET_COMMAND_ID_GPS_DATA_RESP:
				
			break;
			
			case PACKET_COMMAND_ID_ALL_WIFI_DATA_RESP:
                if(vPacket->payload[2] == 1) //connect to wifi, only copy the data from the BLE if we are connecting. 
                {
                    strncpy(sgReceivedWifiConfig.ssid,&vPacket->payload[3],SSID_DATA_SIZE);
                    strncpy(sgReceivedWifiConfig.passphrase,&vPacket->payload[35],PASSPHRASE_DATA_SIZE);
                    sgReceivedWifiConfig.securityType = vPacket->payload[99];
                    msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_WIFI_CONFIG,&sgReceivedWifiConfig);  
                    //set the default channel to all channels
                    sgReceivedWifiConfig.channel = 0xFF;                           
                }
                //send the message to connect/disconnect from the wifi network. 
                msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_WIFI_CONTROL,vPacket->payload[2]);
			break;
			
			case PACKET_COMMAND_ID_SEND_RAW_DATA_TO_MASTER:	// received as notification every time new data is written
			break;
			
			case PACKET_COMMAND_ID_GET_RAW_DATA_RESP:	// received when the master polls for data
			break;
            
            case PACKET_COMMAND_ID_BLE_RECORDING_REQUEST:
                msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_BLE, MSG_TYPE_TOGGLE_RECORDING, vPacket->payload[2]); 
            break;
            case PACKET_COMMAND_ID_BLE_TIME_REQUEST:
                sendTimeToBleModule();
            break;
            case PACKET_COMMAND_ID_BLE_SET_TIME:
                setTimeFromPacket(&(vPacket->payload[2]), vPacket->payloadSize -2); 
            break;
            case PACKET_COMMAND_ID_BLE_EVENT:
                if(vPacket->payload[2] == 1)
                {
                    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Received Pain Event!!\r\n");
                }
                else if(vPacket->payload[2] == 2)
                {
                    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Received Concern Event!!\r\n");
                }
            break;
			
			default:
			break;
		}
	}
}
/**
* @brief Processes messages received from other modules. Is called from BLE task.  
* @param vMessage, message that was received
* @return void
*/
static void processMessage(msg_message_t vMessage)
{
	switch(vMessage.msgType)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		{
			//check if the state has changed, if so 
            if (sgBrainpackStatus.sgCurrentState != vMessage.data)
			{
				sgBrainpackStatus.sgCurrentState = vMessage.data;
				vNewBpStateDataAvailble = true;
			}
            
		}
		break;
		case MSG_TYPE_ERROR:
			
		break;
		case MSG_TYPE_SDCARD_STATE:
		{
			if (sgBrainpackStatus.sdCardState != vMessage.data)
			{
				sgBrainpackStatus.sdCardState = vMessage.data;
				vNewBpStateDataAvailble = true;
			}
		}
		break;
		case MSG_TYPE_WIFI_STATE:
		{
			if (sgBrainpackStatus.wiFiConnectionState != vMessage.data)
			{
				sgBrainpackStatus.wiFiConnectionState = vMessage.data;
				vNewBpStateDataAvailble = true;
			}
		}
		break;
		case MSG_TYPE_SUBP_STATUS:
		{
			sgpSubProcessorStatusData = vMessage.parameters;
			if ((sgBrainpackStatus.batteryLevel != sgpSubProcessorStatusData->chargeLevel) || (sgBrainpackStatus.chargeState != sgpSubProcessorStatusData->chargerState))
			{
				sgBrainpackStatus.batteryLevel =	sgpSubProcessorStatusData->chargeLevel;
				sgBrainpackStatus.chargeState = sgpSubProcessorStatusData->chargerState;
                sgBrainpackStatus.sensorMask = sgpSubProcessorStatusData->sensorMask;
				vNewBpStateDataAvailble = true;
			}
		}
		break;
        case MSG_TYPE_DEBUG_BLE:
            sendInitialParameters(sgpModuleConfiguration);   
            
        break;
		default:
		break;
		
	}
}
/**
* @brief sends the initial parameters to the BLE module.
* @param vpModuleCfg, pointer to structure containing all BLE configuration
* @return void
*/
static void sendInitialParameters(ble_moduleConfig_t* vpModuleCfg)
{
    uint8_t vaOutputData[BLE_INIT_PACKET_SIZE + PACKET_HEADER_SIZE] = {0};	//create buffer for message
    ble_pkt_initialData_t vInitialDataPacket; 
    //clear the structure to all nulls
    memset(&vInitialDataPacket,0,sizeof(ble_pkt_initialData_t));
    //load the structure for the initial settings packet
    //TODO: use strncpy instead to make safer
    strcpy(vInitialDataPacket.ssid,vpModuleCfg->wirelessConfig->ssid);
    strcpy(vInitialDataPacket.passphrase,vpModuleCfg->wirelessConfig->passphrase);
    vInitialDataPacket.securityType = vpModuleCfg->wirelessConfig->securityType;
    strcpy(vInitialDataPacket.serialNumber,vpModuleCfg->serialNumber);
    strcpy(vInitialDataPacket.fwVersion,vpModuleCfg->fwVersion);
    strcpy(vInitialDataPacket.hwRevision,vpModuleCfg->hwRevision);
    strcpy(vInitialDataPacket.modelString,vpModuleCfg->modelString);
    //populate the header packet
    vaOutputData[0] = PACKET_TYPE_MASTER_CONTROL;
    vaOutputData[1] = PACKET_COMMAND_ID_BLE_INITIAL_PARAMETERS;
    //copy over the data to the output buffer
    memcpy(&vaOutputData[2], &vInitialDataPacket, sizeof(ble_pkt_initialData_t));
    //send the packet
    pkt_sendRawPacket(&sgBleUartConfig, vaOutputData, (BLE_INIT_PACKET_SIZE + PACKET_HEADER_SIZE));
}
/**
* @brief sends the current brainpack status packet to the BLE module
* @return void
*/
static void sendBpStatusData()
{
	uint8_t vaOutputData[BLE_BP_STATUS_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header
		
	vaOutputData[0] = PACKET_TYPE_MASTER_CONTROL;
	vaOutputData[1] = PACKET_COMMAND_ID_BP_STATUS;
	memcpy(&vaOutputData[2], &sgBrainpackStatus, BLE_BP_STATUS_DATA_SIZE);		
	pkt_sendRawPacket(&sgBleUartConfig, vaOutputData, (BLE_BP_STATUS_DATA_SIZE + PACKET_HEADER_SIZE));
}
/**
* @brief sends the start fast advertise command to BLE module 
* @return void
*/
static void sendStartFastAdv()
{
	uint8_t vaOutputData[2] = {PACKET_TYPE_MASTER_CONTROL, PACKET_COMMAND_ID_START_FAST_ADV};
	pkt_sendRawPacket(&sgBleUartConfig, vaOutputData, sizeof(vaOutputData));
}
/**
* @brief sends a raw data packet to the BLE module
* @param vpData, pointer to raw data
* @param vLength, length of raw data to send
* @return void
*/
static void sendRawDataPacket(uint8_t *vpData, uint8_t vLength)
{
	uint8_t outputData[BLE_MAX_RAW_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header	
	outputData[0] = PACKET_TYPE_MASTER_CONTROL;
	outputData[1] = PACKET_COMMAND_ID_SEND_RAW_DATA_TO_BLE;
	memcpy(&outputData[2], vpData, (vLength > BLE_MAX_RAW_DATA_SIZE ? BLE_MAX_RAW_DATA_SIZE : vLength));	
	pkt_sendRawPacket(&sgBleUartConfig, outputData, (BLE_MAX_RAW_DATA_SIZE + PACKET_HEADER_SIZE));
}

/**
* @brief sends a packet containing the current system time to the BLE module
    Date time packet is in the following format
    Byte 0: 0x01 (master control ID)
    Byte 1: PACKET_COMMAND_ID_BLE_INITIAL_TIME (command ID)
    Byte 2-3:Year
    Byte 4: Month
    Byte 5: Day
    Byte 6: Hour
    Byte 7: Minute
    Byte 8: Second
    Byte 9: Day of week   
* @return void
*/
static void sendTimeToBleModule()
{
    uint8_t vaOutputData[10]; //the packet length is 10 TODO: consider adding define
    uint32_t hour, minute, second, year, month, day, dow;    
    //setup the packet header
    vaOutputData[0] = PACKET_TYPE_MASTER_CONTROL;
    vaOutputData[1] = PACKET_COMMAND_ID_BLE_INITIAL_TIME;
    rtc_get_time(RTC,&hour,&minute,&second);
    rtc_get_date(RTC,&year,&month,&day,&dow);
  
    vaOutputData[2] = year & 0xFF; 
    vaOutputData[3] = year >> 8; 
    vaOutputData[4] = (uint8_t)month; 
    vaOutputData[5] = (uint8_t)day; 
    vaOutputData[6] = (uint8_t)hour; 
    vaOutputData[7] = (uint8_t)minute; 
    vaOutputData[8] = (uint8_t)second; 
    vaOutputData[9] = (uint8_t)dow;  
    pkt_sendRawPacket(&sgBleUartConfig, vaOutputData, sizeof(vaOutputData));   
    
}
/**
* @brief sets the the current brainpack time from the payload of a set time packet.
    Date time packet is in the following format
    Byte 0-1:Year
    Byte 2: Month
    Byte 3: Day
    Byte 4: Hour
    Byte 5: Minute
    Byte 6: Second
    Byte 7: Day of week
* @return void
*/
static void setTimeFromPacket(uint8_t* timeData, uint16_t length)
{
    uint32_t hour, minute, second, year, month, day, dow;   
    //make sure the length is good      
    if(length < 8)
    {
        return; 
    }
    year = timeData[0];
    year += (uint32_t)timeData[1]<<8;     
    month = timeData[2]; 
    day = timeData[3]; 
    hour = timeData[4]; 
    minute = timeData[5]; 
    second = timeData[6]; 
    dow = timeData[7];     
    rtc_set_date(RTC,year,month,day,dow);
    rtc_set_time(RTC,hour,minute,second);  
    dbg_printf(DBG_LOG_LEVEL_DEBUG,"Time set to: %d:%d:%d\r\n",hour,minute,second);
    //need to add command to send the new time to the sub_processor. 
    
}