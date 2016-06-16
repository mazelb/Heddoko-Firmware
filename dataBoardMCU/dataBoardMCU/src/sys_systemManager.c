/*
 * sys_systemManager.c
 *
 * Created: 5/24/2016 3:37:09 PM
 *  Author: sean
 */ 
/*
 * @brief: A free running task which initializes the peripherals on boot-up,
 *			listens to the other modules and sends commands, starts other task threads.
 */

#include <asf.h>
#include "sys_systemManager.h"
#include "common.h"
#include "string.h"
#include "msg_messenger.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "task_SensorHandler.h"
#include <stdarg.h>

/*	Local static functions	*/
static void initAllUarts();							//initialize all uarts
static void deInitAllUarts();						//de-initialize all uarts
void reInitAllUarts();								//re-initialize all uarts
static void peripheralInit();						//initialize all peripherals
static void processEvent(msg_message_t message);	//process the received event
static void initModule(modules_t module);
static void deInitModule(modules_t module);
static void assertError(modules_t module);
static void assertReady(modules_t module);
static bool isModuleReady(modules_t module);

/*	Extern functions	*/
extern bool sdCardPresent();

/*	Extern variables	*/
extern char debugLogNewFileName[], debugLogOldFileName[];
extern bool enableStreaming;

/*	Local & Global variables	*/
system_status_t systemStatus = {0};
volatile uint8_t dataLogBufferA[DATALOG_MAX_BUFFER_SIZE] = {0} , dataLogBufferB[DATALOG_MAX_BUFFER_SIZE] = {0};
volatile uint8_t debugLogBufferA[DEBUGLOG_MAX_BUFFER_SIZE] = {0} , debugLogBufferB[DEBUGLOG_MAX_BUFFER_SIZE] = {0};
xQueueHandle queue_systemManager = NULL;
drv_uart_config_t *consoleUart = NULL, *dataOutUart = NULL;
uint32_t expectedReadyMask = 0;

sdc_file_t dataLogFile =
{
	.bufferIndexA = 0,
	.bufferIndexB = 0,
	.bufferPointerA = dataLogBufferA,
	.bufferPointerB = dataLogBufferB,
	.bufferSize = DATALOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileName = "datalog",
	.fileOpen = false,
	.activeBuffer = 0
};

sdc_file_t debugLogFile =
{
	.bufferIndexA = 0,
	.bufferIndexB = 0,
	.bufferPointerA = debugLogBufferA,
	.bufferPointerB = debugLogBufferB,
	.bufferSize = DEBUGLOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileName = "sysHdk",
	.fileOpen = false,
	.activeBuffer = 0
};

drv_uart_config_t uart0Config =
{
	.p_usart = UART0,
	.mem_index = 0,
	.init_as_DMA = TRUE,
	.uart_options =
	{
		.baudrate   = 921600,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};
drv_uart_config_t uart1Config =
{
	.p_usart = UART1,
	.mem_index = 1,
	.init_as_DMA = TRUE,
	.uart_options =
	{
		.baudrate   = 921600,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};
drv_uart_config_t usart0Config =
{
	.p_usart = USART0,
	.mem_index = 2,
	.init_as_DMA = FALSE,
	.uart_options =
	{
		.baudrate   = CONF_BAUDRATE,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};
drv_uart_config_t usart1Config =
{
	.p_usart = USART1,
	.mem_index = 3,
	.init_as_DMA = TRUE,
	.uart_options =
	{
		.baudrate   = 460800,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};

/*	Function definitions	*/
void configure_console()
{
	stdio_serial_init(consoleUart->p_usart, &consoleUart->uart_options);
	setbuf(stdout, NULL);
}

void systemManager(void* pvParameters)
{
	bool sd_val, sd_oldVal, enable = false;
	uint8_t data[20] = {0};
	uint16_t count = 0;
	status_t status = STATUS_PASS;
	msg_message_t eventMessage;
	
	/*	Initialize the peripherals	*/
	peripheralInit();
	
	queue_systemManager = xQueueCreate(10, sizeof(msg_message_t));
	if (queue_systemManager != 0)
	{
		msg_registerForMessages(MODULE_SYSTEM_MANAGER, 0xff, queue_systemManager);
	}
	
	initModule(MODULE_SDCARD);
	
	systemStatus.modulesReadyMask |= (TRUE << MODULE_SYSTEM_MANAGER);
	while (1)
	{		
		if (isModuleReady(MODULE_SDCARD) == FALSE)
		{
			if (systemStatus.sdCardState == SD_CARD_INITIALIZED)
			{
				status =  sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);
				status |= sdc_openFile(&debugLogFile, debugLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG);
				if (status == STATUS_PASS)
				{
					assertReady(MODULE_SDCARD);
				}
				else
				{
					assertError(MODULE_SDCARD);
				}
			}
			else
			{
				deInitModule(MODULE_SENSOR_HANDLER);
			}
		}
		
		if (isModuleReady(MODULE_SDCARD) == TRUE)
		{
			initModule(MODULE_SENSOR_HANDLER);
		}
		
		if (enable)
		{
			snprintf(data, 21, "Data count: %06d\r\n", count);
			drv_uart_DMA_putData(consoleUart, data, 21);
			drv_uart_DMA_wait_endTx(consoleUart);
			//sdc_writeToFile(&dataLogFile, (void*)data, sizeof(data));
			count++;
		}
		
		if(xQueueReceive(queue_systemManager, &(eventMessage), 1) == true)
		{
			processEvent(eventMessage);
		}
		vTaskDelay(100);
	}
}

static void processEvent(msg_message_t message)
{
	status_t status = STATUS_PASS;
	msg_sd_card_state_t* sd_eventData;
	msg_sensor_state_t* sensor_eventData;
	sd_eventData = (msg_sd_card_state_t *) message.parameters;
	sensor_eventData = (msg_sensor_state_t*) message.parameters;
	
	switch (message.type)
	{
		case MSG_TYPE_SDCARD_STATE:
			if (message.source == MODULE_SDCARD)
			{
				if (sd_eventData->message == SD_CARD_INSERTED)
				{
					puts("SD-card inserted\r");
					systemStatus.sdCardState = sd_eventData->message;
				}
			}
		break;
		case MSG_TYPE_READY:
			if (message.source == MODULE_SDCARD)
			{
				if (sd_eventData->message == SD_CARD_INITIALIZED)
				{
					puts("SD-card initialized\r");
					systemStatus.sdCardState = sd_eventData->message;
					//open new files
					status =  sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);
					status |= sdc_openFile(&debugLogFile, debugLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG);
					if (status == STATUS_PASS)
					{
						assertReady(MODULE_SDCARD);
					}
					else
					{
						assertError(MODULE_SDCARD);
					}
				}
			}
			else if (message.source == MODULE_SENSOR_HANDLER)
			{
				assertReady(MODULE_SENSOR_HANDLER);
			}
		break;
		case MSG_TYPE_ERROR:
			if (message.source == MODULE_SENSOR_HANDLER)
			{
				if (sensor_eventData->errorCode == SENSOR_DISCONNECTED)
				{
					puts("No sensor\r");
					assertError(MODULE_SENSOR_HANDLER);
				}
				else if (sensor_eventData->errorCode == SENSOR_PACKET_INCOMPLETE)
				{
					puts("Sensor Error: incomplete packet\r");
					assertError(MODULE_SENSOR_HANDLER);
				}
			}
			else if (message.source == MODULE_SDCARD)
			{
				if (sd_eventData->message == SD_CARD_REMOVED)
				{
					puts("SD-Card removed\r");
					systemStatus.sdCardState = sd_eventData->message;
					assertError(MODULE_SDCARD);
				}
			}
		break;
		case MSG_TYPE_ENTERING_NEW_STATE:
		case MSG_TYPE_SDCARD_SETTINGS:
		case MSG_TYPE_WIFI_STATE:
		case MSG_TYPE_WIFI_SETTINGS:
		case MSG_TYPE_USB_CONNECTED:
		case MSG_TYPE_CHARGER_EVENT:
		case MSG_TYPE_COMMAND_PACKET_RECEIVED:
		default:
		break;
	}
	
	free(message.parameters);
}

static void initAllUarts()
{
	drv_uart_init(&uart0Config);
	drv_uart_init(&uart1Config);
	drv_uart_init(&usart0Config);
	drv_uart_init(&usart1Config);
}

static void deInitAllUarts()
{
	drv_uart_deInit(&uart0Config);
	drv_uart_deInit(&uart1Config);
	drv_uart_deInit(&usart0Config);
	drv_uart_deInit(&usart1Config);
}

void reInitAllUarts()
{
	deInitAllUarts();
	usart1Config.uart_options.baudrate = 2000000;
	initAllUarts();
}

static void peripheralInit()
{
	consoleUart = &uart1Config;
	dataOutUart = &uart0Config;
	NVIC_EnableIRQ(SysTick_IRQn);
	board_init();
	configure_console();
	drv_gpio_initializeAll();
	initAllUarts();
	puts("System Initialized.\r");
}

void initModule(modules_t module)
{
	status_t status = STATUS_PASS;
	msg_sys_manager_t* sys_eventData;
	msg_sys_manager_t* sys_eventData1;
	uint32_t startTime = xTaskGetTickCount();
	
	if (isModuleReady(module) == TRUE)
	{
		return;
	}
	
	switch (module)
	{
		case MODULE_SDCARD:
			do
			{
				sys_eventData = malloc(sizeof(msg_sys_manager_t));
				sys_eventData->mountSD = true;
				sys_eventData->unmountSD = false;
				sys_eventData->enableSensorStream = false;
				status = msg_sendMessage(MODULE_SDCARD, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData);
				expectedReadyMask |= (TRUE << MODULE_SDCARD);
				vTaskDelay(10);
			}while ((status != STATUS_PASS) && (xTaskGetTickCount() - startTime < 200));	//wait for the queue to initialize
		break;
		
		case MODULE_SENSOR_HANDLER:
			sys_eventData1 = malloc(sizeof(msg_sys_manager_t));
			sys_eventData1->mountSD = false;
			sys_eventData1->unmountSD = false;
			sys_eventData1->enableSensorStream = true;
			status |= msg_sendMessage(MODULE_SENSOR_HANDLER, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData1);
			expectedReadyMask |= (TRUE << MODULE_SENSOR_HANDLER);
		break;
		
		default:
		break;
	}
}

static void deInitModule(modules_t module)
{
	status_t status = STATUS_PASS;
	msg_sys_manager_t* sys_eventData;
	msg_sys_manager_t* sys_eventData1;
	uint32_t startTime = xTaskGetTickCount();
	
	if (isModuleReady(module) == FALSE)
	{
		return;
	}
	
	switch (module)
	{
		case MODULE_SENSOR_HANDLER:
			sys_eventData1 = malloc(sizeof(msg_sys_manager_t));
			sys_eventData1->mountSD = false;
			sys_eventData1->unmountSD = false;
			sys_eventData1->enableSensorStream = false;
			status |= msg_sendMessage(MODULE_SENSOR_HANDLER, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData1);
			expectedReadyMask &= ~(TRUE << MODULE_SENSOR_HANDLER);
			systemStatus.modulesReadyMask &= ~(TRUE << MODULE_SENSOR_HANDLER);
		break;
		default:
		break;
	}
}

static void assertReady(modules_t module)
{
	systemStatus.modulesReadyMask |= (TRUE << module);
	systemStatus.modulesErrorMask &= ~(TRUE << module);
}

static void assertError(modules_t module)
{
	systemStatus.modulesErrorMask |= (TRUE << module);
	systemStatus.modulesReadyMask &= ~(TRUE << module);
}

static bool isModuleReady(modules_t module)
{
	if ((systemStatus.modulesReadyMask & (TRUE << module)) != NULL)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void __attribute__((optimize("O0"))) debugPrintStringInt(char* str, int number)
{
	size_t length = 0;
	char timeStampedStr[MAX_DEBUG_STRING_LENGTH];
	int len = itoa(xTaskGetTickCount(), timeStampedStr, 10);
	timeStampedStr[len++] = ',';
	length = len;
	len = itoa(number, timeStampedStr+length, 10);
	timeStampedStr[length+len] = ',';
	length += len + 1;
	strncpy(timeStampedStr+length, str, MAX_DEBUG_STRING_LENGTH-length);
	length = strlen(timeStampedStr);
	//length = snprintf(timeStampedStr,200,"%08d,%s %d\r\n",sgSysTickCount,str,number);
	if(length > 0)
	{
		if(systemStatus.debugPrintsEnabled)
		{
			if(consoleUart != NULL)
			{
				drv_uart_putData(consoleUart, timeStampedStr, length);
			}
		}
		sdc_writeToFile(&debugLogFile, timeStampedStr, length);
	}
}

void __attribute__((optimize("O0"))) debugPrintString(char* str)
{
	size_t length = 0;
	char timeStampedStr[MAX_DEBUG_STRING_LENGTH];
	int len = itoa(xTaskGetTickCount(), timeStampedStr, 10);
	timeStampedStr[len++] = ',';
	strncpy(timeStampedStr+len, str, MAX_DEBUG_STRING_LENGTH-len);
	length = strlen(timeStampedStr);
	if(length > 0)
	{
		if(systemStatus.debugPrintsEnabled)
		{
			if(consoleUart != NULL)
			{
				drv_uart_putData(consoleUart, timeStampedStr, length);
			}
		}
		sdc_writeToFile(&debugLogFile, timeStampedStr, length);
	}
}

//static void putStr(char * str)
//{
	//debugPrintString(str);
//}