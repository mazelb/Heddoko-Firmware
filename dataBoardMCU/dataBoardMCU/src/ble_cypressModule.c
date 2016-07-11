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

#define BLE_PORT	USART0_IDX		//assign a default comm bus to the module

/*	extern variables	*/

/*	local variables	*/
rawPacket_t ble_packet;
uint8_t ble_comPort = BLE_PORT;
pkt_packetParserConfiguration_t ble_packetParserConfig =
{
	.transmitDisable = NULL,
	.transmitEnable = NULL,
	.packetReceivedCallback = ble_processPacket,		//redirect to custom packet callback here.
	.packet = NULL
};

/*	local functions	*/

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
	
	//ble_packetParserConfig.uartModule = uartConfig;
	//ble_comPort = drv_uart_getUartIdx(uartConfig);
	//
	//if ((uartConfig->enable_dma_interrupt == TRUE) && (ble_comPort < MAX_NUM_OF_UARTS))
	//{
		//pkt_packetParserInit(&ble_packetParserConfig, ble_comPort);
		//pkt_registerPacketAndCallback(&ble_packetParserConfig, &ble_packet, ble_packetParserConfig.packetReceivedCallback);
	//}
	//else
	//{
		//status |= STATUS_FAIL;
	//}
	
	return status;
}

/* all packets have the format
 <type(1B)><command id(1B)><payload(size dependant on command type)> 
*/
void ble_processPacket(rawPacket_t* packet)		//callback function for the BLE packets
{
	if (packet->payloadSize < 2)
	{
		//if there are less than two bytes then it is not a valid packet.
		return;
	}
	
	if (packet->payload[0] == PACKET_TYPE_BLUETOOTH_MODULE)
	{
		switch (packet->payload[1])
		{
			case PACKET_COMMAND_ID_GPS_DATA_RESP:
			break;
			case PACKET_COMMAND_ID_SSID_DATA_RESP:
			break;
			case PACKET_COMMAND_ID_PASSPHRASE_DATA_RESP:
			break;
			case PACKET_COMMAND_ID_SECURITY_TYPE_DATA_RESP:
			break;
			case PACKET_COMMAND_ID_SEND_RAW_DATA_RESP:
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
		case BLE_WIFI_DATA_SSID:
			commandFrameBuffer[1] = PACKET_COMMAND_ID_SSID_DATA_REQ;
		break;
		case BLE_WIFI_DATA_PASSPHRASE:
			commandFrameBuffer[1] = PACKET_COMMAND_ID_PASSPHRASE_DATA_REQ;
		break;
		case BLE_WIFI_DATA_SECURITY_TYPE:
			commandFrameBuffer[1] = PACKET_COMMAND_ID_SECURITY_TYPE_DATA_REQ;
		break;
		case BLE_WIFI_DATA_ALL:
			//TODO: implement a way to fetch all Wifi data at once.
		break;
		default:
		break;
	}
	
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
	
	commandFrameBuffer[1] = PACKET_COMMAND_ID_SEND_RAW_DATA_REQ;
	
	memcpy(commandFrameBuffer, data, MAX_RAW_DATA_PACKET_SIZE);
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE + MAX_RAW_DATA_PACKET_SIZE, ble_comPort);
}