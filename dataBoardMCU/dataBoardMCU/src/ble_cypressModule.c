/*
 * ble_cypressModule.c
 *
 * Created: 2016-06-16 11:41:21 AM
 *  Author: Hriday Mehta
 */ 

#include "ble_cypressModule.h"

/*
 * @brief: Send request to get most recent GPS data
 */
void ble_send_gpsDataReq()
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
		
	commandFrameBuffer[1] = PACKET_COMMAND_ID_GPS_DATA_REQ;
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE);
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
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE);
}

/*
 * @brief: Send request to most recent raw data
 */
void ble_send_rawDataReq()
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
	
	commandFrameBuffer[1] = PACKET_COMMAND_ID_GET_RAW_DATA_REQ;
	
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE);
}

/*
 * @brief: Send request to most recent raw data
 */
void ble_send_rawData (uint8_t *data)
{
	uint8_t commandFrameBuffer[MAX_BLE_COMMAND_PACKET_SIZE + MAX_RAW_DATA_PACKET_SIZE] = {PACKET_TYPE_MASTER_CONTROL, 0};
	
	commandFrameBuffer[1] = PACKET_COMMAND_ID_SEND_RAW_DATA_REQ;
	
	memcpy(commandFrameBuffer, data, MAX_RAW_DATA_PACKET_SIZE);
	pkt_SendRawPacket(commandFrameBuffer, MAX_BLE_COMMAND_PACKET_SIZE + MAX_RAW_DATA_PACKET_SIZE);
}