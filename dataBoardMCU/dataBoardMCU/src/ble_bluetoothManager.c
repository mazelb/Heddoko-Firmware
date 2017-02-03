/*
 * ble_bluetoothManager.c
 *
 * Created: 8/17/2016 4:40:43 PM
 *  Author: sean
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
static void sendTimeToBleModule();
static void setTimeFromPacket(uint8_t* timeData, uint16_t length);

/*	Local variables	*/
xQueueHandle queue_ble = NULL;
bool newBpStateDataAvailble = false;
subp_status_t *subProcessorStatusData;
uint8_t vRawData[BLE_MAX_RAW_DATA_SIZE];
ble_moduleConfig_t* moduleConfiguration; 
net_wirelessConfig_t receivedWifiConfig; 

drv_uart_config_t usart1Config =
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
struct bpStatus_t
{
	uint8_t batteryLevel;
	uint8_t chargeState;
	uint8_t currentState;
	uint8_t wiFiConnectionState;
	uint8_t sdCardState;
    uint32_t sensorMask; 
}bpStatus;	// status to be passed to BLE module.

struct wifi_data_t
{
    uint8_t ssid[SSID_DATA_SIZE];
    uint8_t passphrase[PASSPHRASE_DATA_SIZE];
    uint8_t securityType;
}wifi_data;	// local config to store wifi data for BLE module.

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


/*		Function definitions	*/
void ble_bluetoothManagerTask(void *pvParameters)
{
	msg_message_t receivedMessage;
	pkt_rawPacket_t rawPacket =
	{
		.bytesReceived = 0,
		.escapeFlag = 0,
		.payloadSize = 0
	};
	
	queue_ble = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_ble != 0)
	{
		msg_registerForMessages(MODULE_BLE, 0xff, queue_ble);
	}
	//initialize the UART packet receiver
	if(drv_uart_init(&usart1Config) != STATUS_PASS)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR,"failed to open USART1 for ble\r\n");
	}
	drv_gpio_setPinState(DRV_GPIO_PIN_BLE_RST, DRV_GPIO_PIN_STATE_HIGH);	
	
    //load the settings
    moduleConfiguration = (ble_moduleConfig_t*)pvParameters;
    vTaskDelay(5000); 
    //send the initial configuration to the BLE module
    sendInitialParameters(moduleConfiguration);   
    vTaskDelay(1000);  
	sendTimeToBleModule();
	//start the main thread where we listen for packets and messages
	while (1)
	{
		if (xQueueReceive(queue_ble, &receivedMessage, 1) == true)
		{
			processMessage(receivedMessage);
		}
		if(pkt_getPacketTimed(&usart1Config,&rawPacket,10) == STATUS_PASS)
		{
			//we have a full packet
			processRawPacket(&rawPacket);
		}
		
		if (newBpStateDataAvailble)
		{
			sendBpStatusData();
			newBpStateDataAvailble = false;
		}
		
		vTaskDelay(10);
	}
}

static void processRawPacket(pkt_rawPacket_t* packet)
{

	//check which type of packet it is.
	//All the packets should be of this type... we're not on a 485 bus.
	int i =0;
	int result = 0;
	if(packet->payload[0] == PACKET_TYPE_BLE_MODULE)
	{
		switch(packet->payload[1])
		{
			case PACKET_COMMAND_ID_GPS_DATA_RESP:
				
			break;
			
			case PACKET_COMMAND_ID_ALL_WIFI_DATA_RESP:
                if(packet->payload[2] == 1) //connect to wifi, only copy the data from the BLE if we are connecting. 
                {
                    strncpy(receivedWifiConfig.ssid,&packet->payload[3],SSID_DATA_SIZE);
                    //memcpy(wifi_data.ssid, (uint8_t *) &packet->payload[2], SSID_DATA_SIZE);
                    strncpy(receivedWifiConfig.passphrase,&packet->payload[35],PASSPHRASE_DATA_SIZE);
                    //memcpy(wifi_data.passphrase, (uint8_t *) &packet->payload[34], PASSPHRASE_DATA_SIZE);
                    receivedWifiConfig.securityType = packet->payload[99];
                    msg_sendMessage(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_WIFI_CONFIG,&receivedWifiConfig);  
                    receivedWifiConfig.channel = 0xFF;                           
                }
                //send the message to connect and disconnect from the wifi network. 
                msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_DEBUG, MSG_TYPE_WIFI_CONTROL,packet->payload[2]);
			break;
			
			case PACKET_COMMAND_ID_SEND_RAW_DATA_TO_MASTER:	// received as notification every time new data is written
			break;
			
			case PACKET_COMMAND_ID_GET_RAW_DATA_RESP:	// received when the master polls for data
			break;
            
            case PACKET_COMMAND_ID_BLE_RECORDING_REQUEST:
                msg_sendMessageSimple(MODULE_SYSTEM_MANAGER, MODULE_BLE, MSG_TYPE_TOGGLE_RECORDING, packet->payload[2]); 
            break;
            case PACKET_COMMAND_ID_BLE_TIME_REQUEST:
                sendTimeToBleModule();
            break;
            case PACKET_COMMAND_ID_BLE_SET_TIME:
                setTimeFromPacket(&(packet->payload[2]), packet->payloadSize -2); 
            break;
            case PACKET_COMMAND_ID_BLE_EVENT:
                if(packet->payload[2] == 1)
                {
                    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Received Pain Event!!\r\n");
                }
                else if(packet->payload[2] == 2)
                {
                    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Received Concern Event!!\r\n");
                }
            break;
			
			default:
			break;
		}
	}
	//dbg_printf(DBG_LOG_LEVEL_DEBUG,"Received a packet!!!!");
}

static void processMessage(msg_message_t message)
{
	switch(message.msgType)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		{
			if (bpStatus.currentState != message.data)
			{
				bpStatus.currentState = message.data;
				newBpStateDataAvailble = true;
			}
            
		}
		break;
		case MSG_TYPE_ERROR:
			
		break;
		case MSG_TYPE_SDCARD_STATE:
		{
			if (bpStatus.sdCardState != message.data)
			{
				bpStatus.sdCardState = message.data;
				newBpStateDataAvailble = true;
			}
		}
		break;
		case MSG_TYPE_WIFI_STATE:
		{
			if (bpStatus.wiFiConnectionState != message.data)
			{
				bpStatus.wiFiConnectionState = message.data;
				newBpStateDataAvailble = true;
			}
		}
		break;
		case MSG_TYPE_SUBP_STATUS:
		{
			subProcessorStatusData = message.parameters;
			if ((bpStatus.batteryLevel != subProcessorStatusData->chargeLevel) || (bpStatus.chargeState != subProcessorStatusData->chargerState))
			{
				bpStatus.batteryLevel =	subProcessorStatusData->chargeLevel;
				bpStatus.chargeState = subProcessorStatusData->chargerState;
                bpStatus.sensorMask = subProcessorStatusData->sensorMask;
				newBpStateDataAvailble = true;
			}
		}
		break;
        case MSG_TYPE_DEBUG_BLE:
            sendInitialParameters(moduleConfiguration);   
            
        break;
		default:
		break;
		
	}
}

static void sendInitialParameters(ble_moduleConfig_t* moduleCfg)
{
    uint8_t outputData[BLE_INIT_PACKET_SIZE + PACKET_HEADER_SIZE] = {0};	//create buffer for message
    ble_pkt_initialData_t initialDataPacket; 
    //clear the structure to all nulls
    memset(&initialDataPacket,0,sizeof(ble_pkt_initialData_t));
    //load the structure for the initial settings packet
    //TODO: use strncpy instead to make safer
    strcpy(initialDataPacket.ssid,moduleCfg->wirelessConfig->ssid);
    strcpy(initialDataPacket.passphrase,moduleCfg->wirelessConfig->passphrase);
    initialDataPacket.securityType = moduleCfg->wirelessConfig->securityType;
    strcpy(initialDataPacket.serialNumber,moduleCfg->serialNumber);
    strcpy(initialDataPacket.fwVersion,moduleCfg->fwVersion);
    strcpy(initialDataPacket.hwRevision,moduleCfg->hwRevision);
    strcpy(initialDataPacket.modelString,moduleCfg->modelString);
    //populate the header packet
    outputData[0] = PACKET_TYPE_MASTER_CONTROL;
    outputData[1] = PACKET_COMMAND_ID_BLE_INITIAL_PARAMETERS;
    //copy over the data to the output buffer
    memcpy(&outputData[2], &initialDataPacket, sizeof(ble_pkt_initialData_t));
    //send the packet
    pkt_sendRawPacket(&usart1Config, outputData, (BLE_INIT_PACKET_SIZE + PACKET_HEADER_SIZE));
}

static void sendBpStatusData()
{
	uint8_t outputData[BLE_BP_STATUS_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header
		
	outputData[0] = PACKET_TYPE_MASTER_CONTROL;
	outputData[1] = PACKET_COMMAND_ID_BP_STATUS;
	memcpy(&outputData[2], &bpStatus, BLE_BP_STATUS_DATA_SIZE);		
	pkt_sendRawPacket(&usart1Config, outputData, (BLE_BP_STATUS_DATA_SIZE + PACKET_HEADER_SIZE));
}

void ble_startFastAdv()
{
	uint8_t outputData[2] = {PACKET_TYPE_MASTER_CONTROL, PACKET_COMMAND_ID_START_FAST_ADV};
	pkt_sendRawPacket(&usart1Config, outputData, sizeof(outputData));
}

void ble_sendWiFiConfig(net_wirelessConfig_t *wiFiConfig)
{
	uint8_t outputData[BLE_WIFI_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header
	
	outputData[0] = PACKET_TYPE_MASTER_CONTROL;
	outputData[1] = PACKET_COMMAND_ID_BLE_INITIAL_PARAMETERS;
	
	memset(&wifi_data.ssid, NULL, SSID_DATA_SIZE);
	strncpy(&wifi_data.ssid, &wiFiConfig->ssid, SSID_DATA_SIZE);
	memset(&wifi_data.passphrase, NULL, PASSPHRASE_DATA_SIZE);
	strncpy(&wifi_data.passphrase, &wiFiConfig->ssid, PASSPHRASE_DATA_SIZE);
	wifi_data.securityType = wiFiConfig->securityType;
	
	memcpy(&outputData[2], (uint8_t *) &wifi_data, BLE_WIFI_DATA_SIZE);
	pkt_sendRawPacket(&usart1Config, outputData, (BLE_WIFI_DATA_SIZE + PACKET_HEADER_SIZE));
}

void ble_sendRawData(uint8_t *data, uint8_t size)
{
	uint8_t outputData[BLE_MAX_RAW_DATA_SIZE + PACKET_HEADER_SIZE] = {0};	// frame has a two byte header
	
	outputData[0] = PACKET_TYPE_MASTER_CONTROL;
	outputData[1] = PACKET_COMMAND_ID_SEND_RAW_DATA_TO_BLE;
	memcpy(&outputData[2], data, (size > BLE_MAX_RAW_DATA_SIZE ? BLE_MAX_RAW_DATA_SIZE : size));
	
	pkt_sendRawPacket(&usart1Config, outputData, (BLE_MAX_RAW_DATA_SIZE + PACKET_HEADER_SIZE));
}

void ble_wifiDataReq()
{
	uint8_t outputData[2] = {PACKET_TYPE_MASTER_CONTROL, PACKET_COMMAND_ID_ALL_WIFI_DATA_REQ};
	pkt_sendRawPacket(&usart1Config, outputData, sizeof(outputData));
}

void ble_rawDataReq()
{
	uint8_t outputData[2] = {PACKET_TYPE_MASTER_CONTROL, PACKET_COMMAND_ID_GET_RAW_DATA_REQ};
	pkt_sendRawPacket(&usart1Config, outputData, sizeof(outputData));
}

static void sendTimeToBleModule()
{
    uint8_t outputData[sizeof(ble_pkt_dateTime_t)+2]; 
    uint32_t hour, minute, second, year, month, day, dow;    
    //cast the packet payload as a date time structure
    ble_pkt_dateTime_t* dateTimePointer = (ble_pkt_dateTime_t*)outputData+2; 
    //setup the packet header
    outputData[0] = PACKET_TYPE_MASTER_CONTROL;
    outputData[1] = PACKET_COMMAND_ID_BLE_INITIAL_TIME;
    rtc_get_time(RTC,&hour,&minute,&second);
    rtc_get_date(RTC,&year,&month,&day,&dow);
     /*
    Date time packet is in the following format
    Byte 0-1:Year
    Byte 2: Month
    Byte 3: Day
    Byte 4: Hour
    Byte 5: Minute
    Byte 6: Second
    Byte 7: Day of week
    */      
    outputData[2] = year & 0xFF; 
    outputData[3] = year >> 8; 
    outputData[4] = (uint8_t)month; 
    outputData[5] = (uint8_t)day; 
    outputData[6] = (uint8_t)hour; 
    outputData[7] = (uint8_t)minute; 
    outputData[8] = (uint8_t)second; 
    outputData[9] = (uint8_t)dow;  
    pkt_sendRawPacket(&usart1Config, outputData, sizeof(outputData));   
    
}

static void setTimeFromPacket(uint8_t* timeData, uint16_t length)
{
    uint32_t hour, minute, second, year, month, day, dow;    


    //make sure the length is good      
    if(length < 8)
    {
        return; 
    }
         /*
    Date time packet is in the following format
    Byte 0-1:Year
    Byte 2: Month
    Byte 3: Day
    Byte 4: Hour
    Byte 5: Minute
    Byte 6: Second
    Byte 7: Day of week
    */ 
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