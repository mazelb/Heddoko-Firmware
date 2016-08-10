/*
 * brd_dataBoardManager.c
 *
 * Created: 8/1/2016 10:37:15 AM
 *  Author: Hriday Mehta
 */ 

#include <asf.h>
#include <string.h>
#include "common.h"
#include "brd_dataBoardManager.h"
#include "pkt_packetCommandsList.h"
#include "mgr_managerTask.h"
#include "sts_statusHeartbeat.h"
#include "pkt_packetParser.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "LTC2941-1.h"

#define DATA_BOARD_TERMINAL_MSG_LENGTH	200
#define DATA_BOARD_TERMINAL_MSG_FREQ	2	// this controls the size of queue to data router

/*	Static functions forward declaration	*/
static void processPacket(pkt_rawPacket_t *packet);
static status_t setDateTimeFromPacket(pkt_rawPacket_t *packet);
static void sendDateTimeResp(status_t status);
static void sendDateTime();
static uint8_t convertToBcd(uint32_t twoDigitNumber);
static uint32_t convertFromBcd(uint8_t bcdNumber);
static void sendStatus(subp_status_t status);

/*	Extern variables	*/
extern xQueueHandle queue_sensorHandler;
extern xQueueHandle mgr_eventQueue;
extern bool enableStream;
extern uint32_t reqSensorMask;
extern uint8_t dataRate;
extern drv_uart_config_t uart0Config, uart1Config;

/*	Local variables	*/
pkt_rawPacket_t dataBoardPacket;
xQueueHandle queue_dataBoard = NULL;		// queue to pass data to the data router
xSemaphoreHandle semaphore_dataBoardUart = NULL;

static drv_uart_config_t dataBoardPortConfig;

void dat_dataBoardManager(void *pvParameters)
{
	UNUSED(pvParameters);
	pkt_rawPacket_t sensorPacket;
	uint32_t buffer;
	subp_status_t systemStatus;
	uint8_t buff[10] = {0};
	
	// the UART for data board is initialized in the brd_init function.
	
	queue_dataBoard = xQueueCreate(DATA_BOARD_TERMINAL_MSG_FREQ, DATA_BOARD_TERMINAL_MSG_LENGTH);
	if (queue_dataBoard == NULL)
	{
		puts("Failed to create data board terminal message queue\r\n");
	}
	
	semaphore_dataBoardUart = xSemaphoreCreateMutex();
	
	dataBoardPortConfig = uart1Config;
	
	while (1)
	{
		// check for incoming packets from data board
		if (pkt_getPacketTimed(&dataBoardPortConfig, &dataBoardPacket, 100) == STATUS_PASS)
		{
			// process the packet
			processPacket(&dataBoardPacket);
		}
		
		vTaskDelay(100);
	}
}

void processPacket(pkt_rawPacket_t *packet)
{
	status_t status;
	uint16_t chargeLevel;
	subp_status_t systemStatus;
	subp_dateTime_t dateTime;
	mgr_eventMessage_t eventMessage; 
	
	if (packet->payloadSize < 2)
	{
		return;	// a packet should have minimum of two bytes
	}
	
	if (packet->payload[0] == PACKET_TYPE_MASTER_CONTROL)
	{
		switch (packet->payload[1])
		{
			case PACKET_COMMAND_ID_SUBP_GET_STATUS:
				// return current status of Power board
				// use semaphore to access the UART
				sts_getSystemStatus(&systemStatus);
				if (xSemaphoreTake(semaphore_dataBoardUart, 100) == pdTRUE)
				{
					sendStatus(systemStatus);
					xSemaphoreGive(semaphore_dataBoardUart);
				}
			break;
			case PACKET_COMMAND_ID_SUBP_CONFIG:
				// set the received sub processor configuration
				dataRate = packet->payload[2];
				reqSensorMask = packet->payload[3] | (packet->payload[4] << 8);	// we ignore the rest of the packet as the total number of sensors is only 9
			break;
			case PACKET_COMMAND_ID_SUBP_STREAMING:
				// enable / disable sensor streaming
				enableStream = (bool) packet->payload[2];
			break;
			case PACKET_COMMAND_ID_SUBP_OUTPUT_DATA:
				// simply pass this data to daughter board and USB
				// should not access the USB and daughter board UART directly, pass it on a queue and handle the data in dataRouter
				xQueueSendToBack(queue_dataBoard, &packet->payload[2], 10);
			break;
			case PACKET_COMMAND_ID_SUBP_POWER_DOWN_RESP:
				// the data board is now powering down
				eventMessage.sysEvent = SYS_EVENT_POWER_SWITCH;		// TODO: remove the call from the manager task.
				if(mgr_eventQueue != NULL)
				{
					if(xQueueSendToBack(mgr_eventQueue,( void * ) &eventMessage,5) != TRUE)
					{
						//this is an error, we should log it.
					}
				}
			break;
			case PACKET_COMMAND_ID_SUBP_GET_DATE_TIME:
				// the data board has requested current date and time
				sendDateTime();
			break;
			case PACKET_COMMAND_ID_SUBP_SET_DATE_TIME:
				// the data board has sent new date and time
				status = setDateTimeFromPacket(packet);
				sendDateTimeResp(status);
			break;
			default:
			break;
		}
	}
}

static status_t setDateTimeFromPacket(pkt_rawPacket_t *packet)
{
	status_t status = STATUS_PASS;
	subp_dateTime_t dateTime;
	uint32_t year = 0, month = 0, dayOfWeek = 0, date = 0;
	uint32_t hour, minute, second;
	uint32_t startTime = xTaskGetTickCount();
	
	// set to registers directly
	dateTime.time = packet->payload[2] | (packet->payload[3] << 8) | (packet->payload[4] << 16) | (0x00 <<22);	// 0x00 as we are using 24-hour mode
	dateTime.date = packet->payload[6] | (packet->payload[7] << 8) | (((packet->payload[8] & 0x1F) | (packet->payload[8] & 0xE0)) << 16) 
					| (packet->payload[9] << 24);
	
	// Date
	RTC->RTC_CR |= RTC_CR_UPDCAL;
	while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD)
	{
		if (xTaskGetTickCount() - startTime > 1000)		// it can take up to one second to get the ACKUPD bit
		{
			status |= STATUS_FAIL;
			return status;
		}
	}

	RTC->RTC_SCCR = RTC_SCCR_ACKCLR;
	RTC->RTC_CALR = dateTime.date;
	RTC->RTC_CR &= (~RTC_CR_UPDCAL);
	/* Clear SECENV in SCCR */
	RTC->RTC_SCCR |= RTC_SCCR_SECCLR;

	status =  (RTC->RTC_VER & RTC_VER_NVCAL);
	
	// Time
	RTC->RTC_CR |= RTC_CR_UPDTIM;
	while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD)
	{
		if (xTaskGetTickCount() - startTime > 1000)		// it can take up to one second to get the ACKUPD bit
		{
			status |= STATUS_FAIL;
			return status;
		}
	}
	RTC->RTC_SCCR = RTC_SCCR_ACKCLR;
	RTC->RTC_TIMR = dateTime.time;
	RTC->RTC_CR &= (~RTC_CR_UPDTIM);
	RTC->RTC_SCCR |= RTC_SCCR_SECCLR;

	status |= (RTC->RTC_VER & RTC_VER_NVTIM);
	
	/*	return the status	*/
	return status;
}

static void sendDateTimeResp(status_t status)
{
	uint8_t response[3] = {0};
		
	if (status == STATUS_PASS)		// date and time successfully written
	{
		response[0] = PACKET_TYPE_SUB_PROCESSOR;
		response[1] = PACKET_COMMAND_ID_SUBP_SET_DATE_TIME_RESP;
		response[2] = 0x01;
		pkt_sendRawPacket(&dataBoardPortConfig, response, 0x03);
	}
	else							// failed to write date and time
	{
		response[0] = PACKET_TYPE_SUB_PROCESSOR;
		response[1] = PACKET_COMMAND_ID_SUBP_SET_DATE_TIME_RESP;
		response[2] = 0x00;
		pkt_sendRawPacket(&dataBoardPortConfig, response, 0x03);
	}
}

static void sendDateTime()
{
	// fetch current date and time from RTC and send it
	
	uint32_t year, month, dayOfWeek, date;
	uint32_t seconds, minutes, hour;
	subp_dateTime_t dateTime;
	uint8_t response[10] = {0};
	
	rtc_get_date(RTC, &year, &month, &date, &dayOfWeek);
	rtc_get_time(RTC, &hour, &minutes, &seconds);	// returns hour in 24-hour mode
	
	// convert to BCD before sending out
	year = convertToBcd(year/100) | (convertToBcd(year%100) << 8) ;
	month = convertToBcd(month);
	dayOfWeek = convertToBcd(dayOfWeek);
	date = convertToBcd(date);
	hour = convertToBcd(hour);
	minutes = convertToBcd(minutes);
	seconds = convertToBcd(seconds);
	dateTime.time = seconds | (minutes << 8) | (hour << 16) | (0x00 << 22);		// 0x00 as the time is in 24-hour mode
	dateTime.date = year | (month << 16) | (dayOfWeek << 21) | (date << 24);
	
	response[0] = PACKET_TYPE_SUB_PROCESSOR;
	response[1] = PACKET_COMMAND_ID_SUBP_GET_DATE_TIME_RESP;
	memcpy(&response[2], &dateTime, 8);
	pkt_sendRawPacket(&dataBoardPortConfig, response, 0x0A);
}

static uint32_t convertFromBcd(uint8_t bcdNumber)
{
	return (((bcdNumber & 0xF0) >> 4) * 10 + (bcdNumber & 0x0F));
}

static uint8_t convertToBcd(uint32_t twoDigitNumber)
{
	int tens, units;
	tens = twoDigitNumber / 10;
	units = twoDigitNumber % 10;
	return ((tens << 4) | units);
}

static void sendStatus(subp_status_t status)
{
	uint8_t response[11] = {0};
		
	response[0] = PACKET_TYPE_SUB_PROCESSOR;
	response[1] = PACKET_COMMAND_ID_SUBP_GET_STATUS_RESP;
	memcpy(&response[2], &status, 9);
		
	pkt_sendRawPacket(&dataBoardPortConfig, response, sizeof(response));
}