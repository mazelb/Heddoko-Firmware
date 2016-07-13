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
#include <stdarg.h>
#include "sys_systemManager.h"
#include "common.h"
#include "string.h"
#include "msg_messenger.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "task_SensorHandler.h"
#include "subp_subProcessor.h"
#include "dat_dataManager.h"

/*	Local static functions	*/
static void initAllTasks();									// initialize all the other tasks
static void initAllUarts();									// initialize all uarts
static void deInitAllUarts();								// de-initialize all uarts
static void peripheralInit();								// initialize all peripherals
static void processEvent(msg_message_t message);			// process the received event
static void initModule(modules_t module);					// initialize an individual or all modules
static void deInitModule(modules_t module);					// de-initialize an individual or all modules
static void assertError(modules_t module);					// assert Error to a module
static void assertReady(modules_t module);					// assert Ready to a module
static bool isModuleReady(modules_t module);				// check if a module is in Ready state
static void checkGpioInt();									// check for GPIO interrupts
static void openDebugLog();									// opens debug Log, swaps the log if necessary
static void sysEnterNewState(sys_manager_systemState_t newState);	// enter new state
void reInitAllUarts();										// re-initialize all uarts
void configureBusSpeed(drv_uart_config_t* uartConfig, uint32_t newBaud);		// configure new bus speed for the UART and reinitialize it

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
uint32_t expectedReadyMask = 0;

sdc_file_t dataLogFile =
{
	.bufferIndexA = 0,
	.bufferIndexB = 0,
	.bufferPointerA = dataLogBufferA,
	.bufferPointerB = dataLogBufferB,
	.bufferSize = DATALOG_MAX_BUFFER_SIZE,
	.fileObj = NULL,
	.fileName = "datalog-new",
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
	.enable_dma_interrupt = FALSE,
	.dma_bufferDepth = FIFO_BUFFER_SIZE,
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
	.enable_dma_interrupt = false,
	.dma_bufferDepth = FIFO_BUFFER_SIZE,
	.uart_options =
	{
		.baudrate   = 921600,
		.charlength = CONF_CHARLENGTH,
		.paritytype = CONF_PARITY,
		.stopbits   = CONF_STOPBITS
	}
};

// Assign Com ports to each module.
system_port_config_t sys_comPorts = 
{
	.sensor_port = NULL,
	.ble_port = NULL,
	.wifi_port = NULL,
	.consoleUart = &uart1Config,
	.dataOutUart = &uart0Config
};

/*	Function definitions	*/
void configure_console()
{
	//stdio_serial_init(consoleUart->p_usart, &consoleUart->uart_options);
	stdio_serial_init(sys_comPorts.consoleUart->p_usart, &sys_comPorts.consoleUart->uart_options);
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
	
	initAllTasks();
	
	vTaskDelay(10);		//wait for all modules to initialize message queues
	sysEnterNewState(SYSTEM_STATE_INIT);
	
	systemStatus.modulesReadyMask |= (TRUE << MODULE_SYSTEM_MANAGER);
	expectedReadyMask |= (TRUE << MODULE_SYSTEM_MANAGER);
	
	while (1)
	{		
		//if (isModuleReady(MODULE_SDCARD) == FALSE)
		//{
			//if (systemStatus.sdCardState == SD_CARD_INITIALIZED)
			//{
				//status =  sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);
				//status |= sdc_openFile(&debugLogFile, debugLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG);
				//if (status == STATUS_PASS)
				//{
					//assertReady(MODULE_SDCARD);
				//}
				//else
				//{
					//assertError(MODULE_SDCARD);
				//}
			//}
			//else
			//{
				//deInitModule(MODULE_SENSOR_HANDLER);
			//}
		//}
		
		//if (isModuleReady(MODULE_SDCARD) == TRUE)
		//{
			//initModule(MODULE_SENSOR_HANDLER);
		//}
		
		if (ioport_get_pin_level(DRV_GPIO_ID_PIN_PW_SW) == FALSE)
		{
			while(ioport_get_pin_level(DRV_GPIO_ID_PIN_PW_SW) == FALSE){;}
				sd_val = !sd_val;
			if (sd_val == TRUE)
			{
				sysEnterNewState(SYSTEM_STATE_RECORDING);
			}
			else
			{
				sysEnterNewState(SYSTEM_STATE_IDLE);
			}
			
		}
		
		if (enable)
		{
			snprintf(data, 21, "Data count: %06d\r\n", count);
			drv_uart_DMA_putData(sys_comPorts.consoleUart, data, 21);
			drv_uart_DMA_wait_endTx(sys_comPorts.consoleUart);
			//sdc_writeToFile(&dataLogFile, (void*)data, sizeof(data));
			count++;
		}
		
		if(xQueueReceive(queue_systemManager, &(eventMessage), 1) == true)
		{
			processEvent(eventMessage);
		}
		
		//checkGpioInt();
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
					//status =  sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);
					//status |= sdc_openFile(&debugLogFile, debugLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG);
					//if (status == STATUS_PASS)
					//{
						assertReady(MODULE_SDCARD);
					//}
					//else
					//{
						//assertError(MODULE_SDCARD);
					//}
				}
			}
			else if (message.source == MODULE_SENSOR_HANDLER)
			{
				assertReady(MODULE_SENSOR_HANDLER);
			}
			
			if (expectedReadyMask == systemStatus.modulesReadyMask)
			{
				sysEnterNewState(SYSTEM_STATE_IDLE);
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
					sysEnterNewState(SYSTEM_STATE_ERROR);
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

static void sysEnterNewState(sys_manager_systemState_t newState)
{
	status_t status = STATUS_PASS;
	msg_sys_manager_t *sys_eventData, *sys_eventData1;
	msg_message_t broadcastMessage;
	
	if (systemStatus.sysState == newState)
	{
		return;
	}
	
	systemStatus.sysState = newState;
	switch (newState)
	{
		case SYSTEM_STATE_SLEEP:
		
		break;
		
		case SYSTEM_STATE_INIT:
			initModule(MODULE_NUMBER_OF_MODULES);		//pass initialization command to all modules
			//openDebugLog();		// open the debugLog file
		break;
		
		case SYSTEM_STATE_IDLE:
			//sys_eventData = malloc(sizeof(msg_sys_manager_t));
			//sys_eventData->systemState	= SYSTEM_STATE_IDLE;
			//status |= msg_sendMessage(MODULE_SENSOR_HANDLER, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData);
			//sys_eventData1 = malloc(sizeof(msg_sys_manager_t));
			//sys_eventData1->systemState	= SYSTEM_STATE_IDLE;
			//status |= msg_sendMessage(MODULE_SDCARD, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData1);
			broadcastMessage.broadcastData = (uint32_t) SYSTEM_STATE_IDLE;
			broadcastMessage.source = MODULE_SYSTEM_MANAGER;
			broadcastMessage.type = MSG_TYPE_ENTERING_NEW_STATE;
			status |= msg_sendBroadcastMessage(&broadcastMessage);
			puts("System State: IDLE\r");
		break;
		
		case SYSTEM_STATE_ERROR:
			deInitModule(MODULE_SENSOR_HANDLER);
			deInitModule(MODULE_SDCARD);
		break;
		
		case SYSTEM_STATE_RECORDING:
			status =  sdc_openFile(&dataLogFile, dataLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DATA_LOG);		// TODO: only for temporary purposes, different module will open this file
			status |= sdc_openFile(&debugLogFile, debugLogFile.fileName, SDC_FILE_OPEN_READ_WRITE_DEBUG_LOG);
			if (status == STATUS_PASS)
			{
				sys_eventData = malloc(sizeof(msg_sys_manager_t));
				sys_eventData->systemState	= SYSTEM_STATE_RECORDING;
				status |= msg_sendMessage(MODULE_SENSOR_HANDLER, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData);
			}
			else
			{
				systemStatus.sysState = SYSTEM_STATE_IDLE;
			}
		break;
		
		default:
		break;
		
	}
}

static void checkGpioInt()
{
	//check for the GPIO interrupts here.
}

void initModule(modules_t module)
{
	status_t status = STATUS_PASS;
	msg_sys_manager_t* sys_eventData_toSD;
	msg_sys_manager_t* sys_eventData_toSensor;
	uint32_t startTime = xTaskGetTickCount();
	modules_t firstModuleIdx = 0, lastModuleIdx = 0;
	
	if (module >= MODULE_NUMBER_OF_MODULES)		//initialize all modules
	{
		firstModuleIdx = 0;
		lastModuleIdx = MODULE_NUMBER_OF_MODULES;
	}
	else										//initialize a particular module.
	{
		firstModuleIdx = module;
		lastModuleIdx = module + 1;
	}
	
	for (uint8_t moduleId = firstModuleIdx; moduleId < lastModuleIdx; moduleId++)		//this loop controls the initialization of modules.
	{
		if (isModuleReady(moduleId) == TRUE)
		{
			return;
		}
		
		switch (moduleId)
		{
			case MODULE_SDCARD:
			do		//NOTE: this do while is obsolete as the vTaskDelay() is called before initModule.
			{
				sys_eventData_toSD = malloc(sizeof(msg_sys_manager_t));
				sys_eventData_toSD->systemState = SYSTEM_STATE_INIT;
				status = msg_sendMessage(MODULE_SDCARD, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData_toSD);
				expectedReadyMask |= (TRUE << MODULE_SDCARD);
				vTaskDelay(10);
			}while ((status != STATUS_PASS) && (xTaskGetTickCount() - startTime < 200));	//wait for the queue to initialize, keep passing command until initialized.
			break;
			
			case MODULE_SENSOR_HANDLER:
			sys_eventData_toSensor = malloc(sizeof(msg_sys_manager_t));
			sys_eventData_toSensor->systemState = SYSTEM_STATE_INIT;
			status |= msg_sendMessage(MODULE_SENSOR_HANDLER, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData_toSensor);
			expectedReadyMask |= (TRUE << MODULE_SENSOR_HANDLER);
			break;
			
			default:
			break;
		}
	}
}

static void deInitModule(modules_t module)
{
	status_t status = STATUS_PASS;
	msg_sys_manager_t* sys_eventData_toSD;
	msg_sys_manager_t* sys_eventData_toSensor;
	uint32_t startTime = xTaskGetTickCount();
	modules_t firstModuleIdx = 0, lastModuleIdx = 0;
	
	if (module >= MODULE_NUMBER_OF_MODULES)		//de-initialize all modules
	{
		firstModuleIdx = 0;
		lastModuleIdx = MODULE_NUMBER_OF_MODULES;
	}
	else										//de-initialize a particular module.
	{
		firstModuleIdx = module;
		lastModuleIdx = module + 1;
	}
	
	for (uint8_t moduleId = firstModuleIdx; moduleId < lastModuleIdx; moduleId++)		//this loop controls the de-initialization of modules.
	{
		if (isModuleReady(moduleId) == FALSE)
		{
			return;
		}
		
		switch (moduleId)
		{
			case MODULE_SDCARD:
			sys_eventData_toSD = malloc(sizeof(msg_sys_manager_t));
			sys_eventData_toSD->systemState	= SYSTEM_STATE_SLEEP;
			status = msg_sendMessage(MODULE_SDCARD, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData_toSD);
			expectedReadyMask |= (TRUE << MODULE_SDCARD);
			break;
			case MODULE_SENSOR_HANDLER:
			sys_eventData_toSensor = malloc(sizeof(msg_sys_manager_t));
			sys_eventData_toSensor->systemState	= SYSTEM_STATE_SLEEP;
			status |= msg_sendMessage(MODULE_SENSOR_HANDLER, MODULE_SYSTEM_MANAGER, MSG_TYPE_ENTERING_NEW_STATE, (void*)sys_eventData_toSensor);
			expectedReadyMask &= ~(TRUE << MODULE_SENSOR_HANDLER);
			systemStatus.modulesReadyMask &= ~(TRUE << MODULE_SENSOR_HANDLER);
			break;
			default:
			break;
		}
	}
}

static void initAllTasks()
{
	if (xTaskCreate(sdc_sdCardTask, "SD", TASK_SD_CARD_STACK_SIZE, NULL, TASK_SD_CARD_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create SD-card task\r");
	}
	//if (xTaskCreate(task_SensorHandler, "SH", TASK_SENSOR_HANDLER_STACK_SIZE, NULL, TASK_SENSOR_HANDLER_PRIORITY, NULL) != pdPASS)
	//{
		//puts("Failed to create sensor handler task\r");
	//}
	//if (xTaskCreate(pkt_ParserTask, "PS", (1024/sizeof(portSTACK_TYPE)), NULL, (tskIDLE_PRIORITY + 3), NULL) != pdPASS)
	//{
	//puts("Failed to create packet parser task\r");
	//}
	if (xTaskCreate(subp_subProcessorTask, "SP", TASK_SUB_PROCESS_MANAGER_STACK_SIZE, NULL, TASK_SUB_PROCESS_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create sub process handler task\r");
	}
	if (xTaskCreate(dat_dataManagerTask, "DM", TASK_DATA_MANAGER_STACK_SIZE, NULL, TASK_DATA_MANAGER_PRIORITY, NULL) != pdPASS)
	{
		puts("Failed to create data manager handler task\r");
	}
}

static void peripheralInit()
{
	NVIC_EnableIRQ(SysTick_IRQn);
	board_init();
	configure_console();
	drv_gpio_initializeAll();
	//initAllUarts();		// the modules will initialize their respective UARTs
	drv_uart_init(sys_comPorts.consoleUart);
	puts("System Initialized.\r");
}

static void initAllUarts()
{
	//drv_uart_init(&uart0Config);
	//drv_uart_init(&uart1Config);
	//drv_uart_init(&usart0Config);
	//drv_uart_init(&usart1Config);
}

static void deInitAllUarts()
{
	//drv_uart_deInit(&uart0Config);
	//drv_uart_deInit(&uart1Config);
	//drv_uart_deInit(&usart0Config);
	//drv_uart_deInit(&usart1Config);
}

void reInitAllUarts()
{
	deInitAllUarts();
	initAllUarts();
}

void configureBusSpeed(drv_uart_config_t* uartConfig, uint32_t newBaud)
{
	uartConfig->uart_options.baudrate = newBaud;
	reInitAllUarts();
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

static void openDebugLog()
{
	status_t status = STATUS_PASS;
	
	// check default file's size
	
	// decide if the file size exceeds, delete old file if exists and change the new file name to old file
	
	// open debugLog file.
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
			if(sys_comPorts.consoleUart != NULL)
			{
				drv_uart_putData(sys_comPorts.consoleUart, timeStampedStr, length);
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
			if(sys_comPorts.consoleUart != NULL)
			{
				drv_uart_putData(sys_comPorts.consoleUart, timeStampedStr, length);
			}
		}
		sdc_writeToFile(&debugLogFile, timeStampedStr, length);
	}
}

//static void putStr(char * str)
//{
	//debugPrintString(str);
//}