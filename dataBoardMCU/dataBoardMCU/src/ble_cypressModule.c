/*
 * ble_cypressModule.c
 *
 * Created: 2016-06-16 11:41:21 AM
 *  Author: Hriday Mehta
 */ 

/*
 * @note: only use this module in the iterrupt based DMA mode.
 */

#include "ble_cypressModule.h"
#include "drv_uart.h"
#include "cmd_commandProcessor.h"
#include "msg_messenger.h"

#define BLE_PORT	USART0_IDX		//assign a default comm bus to the module
#define WIFI_SSID_DATA_SIZE				32
#define WIFI_PASSPHRASE_DATA_SIZE		64
#define WIFI_SECURITY_TYPE_DATA_SIZE	1

/*	extern variables	*/

/*	Extern functions	*/

/*	Static functions forward declarations	*/
static status_t ble_init(drv_uart_config_t* uartConfig);
static void ble_processPacket(rawPacket_t* packet);
static void processEvent(msg_message_t *message);
static void ble_startFastAdv();
static void ble_enableMsgs(bool enable);
static void ble_send_wifiConfig();
static void saveWifiConfig();
static void loadWifiConfig();

/*	local variables	*/
static xQueueHandle msg_queue_bleTask = NULL;
rawPacket_t ble_packet;
uint8_t ble_comPort = BLE_PORT;
static struct  
{
	//float64_t latitude;
	//float64_t longitude;
	//float64_t altitude;
	//float64_t time;
}ble_gps_data;

static struct  
{
	uint8_t ssid[WIFI_SSID_DATA_SIZE];
	uint8_t passphrase[WIFI_PASSPHRASE_DATA_SIZE];
	uint8_t securityType;
}ble_wifi_data;

static uint8_t securityType;

drv_uart_config_t usart0Config =
{
	.p_usart = USART0,
	.mem_index = 2,
	.init_as_DMA = FALSE,
	.enable_dma_interrupt = false,
	.dma_bufferDepth = FIFO_BUFFER_SIZE,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	},
	.pktConfig = 
	{
		.transmitDisable = NULL,
		.transmitEnable = NULL,
		.packetReceivedCallback = ble_processPacket,
		.packet = 
		{
			.bytesReceived = NULL,
			.escapeFlag = NULL,
			.payloadSize = NULL,
			.payload = {NULL}
		}
	}
};


/*	local functions	*/

/*
 * @brief: Main ble processor task
 * 
 */
void ble_bleModuleTask(void *pvParameters)
{
	UNUSED(pvParameters);
	msg_message_t eventData;
	
	//register for message queue
	msg_queue_bleTask = xQueueCreate(10, sizeof(msg_message_t));
	if (msg_queue_bleTask != NULL)
	{
		msg_registerForMessages(MODULE_BLE_MANAGER, 0xff, msg_queue_bleTask);
	}
	
	// initialize the module
	ble_init(&usart0Config);
	
	while (1)
	{
		// check for received messages and take necessary actions
		if (xQueueReceive(msg_queue_bleTask, &eventData, 1) == TRUE)
		{
			processEvent(&eventData);
		}
		
		vTaskDelay(10);
	}
}

static void processEvent(msg_message_t *message)
{
	switch (message->type)
	{
		case MSG_TYPE_ENTERING_NEW_STATE:
		break;
		case MSG_TYPE_READY:
		break;
		case MSG_TYPE_ERROR:
		break;
		case MSG_TYPE_SDCARD_STATE:
		break;
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		break;
		case MSG_TYPE_SDCARD_SETTINGS:
		break;
		case MSG_TYPE_WIFI_STATE:
		break;
		case MSG_TYPE_WIFI_SETTINGS:
		break;
		case MSG_TYPE_USB_CONNECTED:
		break;
		case MSG_TYPE_CHARGER_EVENT:
		break;
		default:
		break;
	}
}

/*
 * @brief: initialize the BLE module and register for the callback functions
 * @note: this function only functions for the interrupt based DMA mode.
 */
status_t ble_init(drv_uart_config_t* uartConfig)
{
	status_t status = STATUS_PASS;
	
	if (uartConfig == NULL)
	{
		return STATUS_FAIL;
	}
	
	drv_uart_init(uartConfig);	// initialize the UART driver
	ble_startFastAdv();			// start BLE in fast advertisement mode
	
	// grab the WIFI config from the NVM and send it over
	loadWifiConfig();
	ble_send_wifiConfig();
	
	return status;
}

/* all packets have the format
 <type(1B)><command id(1B)><payload(size dependant on command type)> 
*/
static void ble_processPacket(rawPacket_t* packet)		//callback function for the BLE packets
{
	if (packet->payloadSize < 2)
	{
		//if there are less than two bytes then it is not a valid packet.
		return;
	}
	
	// don't scrutinize packets, just enqueue to local queue
	
	if (packet->payload[0] == PACKET_TYPE_BLUETOOTH_MODULE)
	{
		switch (packet->payload[1])
		{
			case PACKET_COMMAND_ID_GPS_DATA_RESP:
			break;
			case PACKET_COMMAND_ID_ALL_WIFI_DATA_RESP:
				//the data will be in the order of <SSID><PASSPHRASE><SEC_TYPE>
				memcpy(ble_wifi_data.ssid, &packet->payload[2], WIFI_SSID_DATA_SIZE);
				memcpy(ble_wifi_data.passphrase, &packet->payload[34], WIFI_PASSPHRASE_DATA_SIZE);
				ble_wifi_data.securityType = packet->payload[98];
				saveWifiConfig();
			break;
			case PACKET_COMMAND_ID_SEND_RAW_DATA_TO_MASTER:
			break;
			case PACKET_COMMAND_ID_GET_RAW_DATA_RESP:
			break;
			default:
			break;
		}
	}
}

/*
 * @brief: Send request to get most recent GPS data
 */
void ble_send_gpsDataReq()
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
		
	commandFrameBuffer[1] = PACKET_COMMAND_ID_GPS_DATA_REQ;
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE, ble_comPort);
}

/*
 * @brief: Send request to most recent Wifi data
 */
void ble_send_wifiDataReq(ble_wifi_data_categories_t dataCategory)
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
	
	switch (dataCategory)
	{
		case BLE_WIFI_DATA_ALL:
			commandFrameBuffer[1] = PACKET_COMMAND_ID_ALL_WIFI_DATA_REQ;
		break;
		default:
		break;
	}
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE, ble_comPort);
}

static void ble_send_wifiConfig()
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
	
	commandFrameBuffer[1] = PACKET_COMMAND_ID_DEFAULT_WIFI_DATA;
	
	memcpy(&commandFrameBuffer[2], ble_wifi_data.ssid, WIFI_SSID_DATA_SIZE);
	memcpy(&commandFrameBuffer[34], ble_wifi_data.passphrase, WIFI_PASSPHRASE_DATA_SIZE);
	commandFrameBuffer[98] = ble_wifi_data.securityType;
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE, ble_comPort);
}

/*
 * @brief: Send request to most recent raw data
 */
void ble_send_rawDataReq()
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
	
	commandFrameBuffer[1] = PACKET_COMMAND_ID_GET_RAW_DATA_REQ;
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE, ble_comPort);
}

/*
 * @brief: Send request to most recent raw data
 */
void ble_send_rawData (uint8_t *data)
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE + MAX_RAW_DATA_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
	
	commandFrameBuffer[1] = PACKET_COMMAND_ID_SEND_RAW_DATA_TO_BLE;
	
	memcpy(commandFrameBuffer, data, MAX_RAW_DATA_PACKET_SIZE);
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE + MAX_RAW_DATA_PACKET_SIZE, ble_comPort);
}

/*
 * @brief: Switch the BLE module to fast advertising mode
 * @param: void
 */
static void ble_startFastAdv()
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
	
	commandFrameBuffer[1] = PACKET_COMMAND_ID_START_FAST_ADV;
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE, ble_comPort);
}

static void loadWifiConfig()
{
	flash_read_user_signature(&ble_wifi_data, sizeof(ble_wifi_data));
}

static void saveWifiConfig()
{
	flash_erase_user_signature();	// erase is mandatory before wirting
	
	if (flash_write_user_signature(&ble_wifi_data, sizeof(ble_wifi_data)) == TRUE)
	{
		puts("saved the new wifi settings\r");
	}
	else
	{
		puts("failed to save the wifi settings\r");
	}
}