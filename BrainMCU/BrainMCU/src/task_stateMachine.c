/*
 * task_stateMachine.c
 *
 * Created: 10/19/2015 9:09:03 AM
 *  Author: Sean Cloghesy
 */ 

#include <asf.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task_stateMachine.h"
#include "task_quinticInterface.h"
#include "task_commandProc.h"
#include "task_fabricSense.h"
#include "task_dataProcessor.h"
#include "task_sdCardWrite.h"
#include "task_emInterface.h"
#include "task_main.h"
#include "drv_gpio.h"
#include "Board_Init.h"
#include "drv_led.h"
#include "settings.h"

const char* systemEventNameString[] = {	
	"Received SYS_EVENT_POWER_SWITCH\r\n",
	"Received SYS_EVENT_RESET_SWITCH\r\n",
	"Received SYS_EVENT_RECORD_SWITCH\r\n",
	"Received SYS_EVENT_OVER_CURRENT\r\n",
	"Received SYS_EVENT_SD_CARD_DETECT\r\n",
	"Received SYS_EVENT_SD_FILE_ERROR\r\n",
	"Received SYS_EVENT_IMU_DISCONNECT\r\n",
	"Received SYS_EVENT_BLE_ERROR\r\n",
	"Received SYS_EVENT_JACK_DETECT\r\n",
	"Received SYS_EVENT_LOW_BATTERY\r\n",
	"Received SYS_EVENT_RESET_COMPLETE\r\n",
	"Received SYS_EVENT_RESET_FAILED\r\n",
	"Received SYS_EVENT_POWER_UP_COMPLETE\r\n",
	"Received SYS_EVENT_GET_ACCEL_DATA_COMPLETE\r\n",
};

const char* systemStateNameString[] = {
	"Current System state: SYS_STATE_OFF\r\n",  
	"Current System state: SYS_STATE_POWER_DOWN\r\n",
	"Current System state: SYS_STATE_RESET\r\n",
	"Current System state: SYS_STATE_IDLE\r\n",
	"Current System state: SYS_STATE_RECORDING\r\n",
	"Current System state: SYS_STATE_ERROR\r\n",
	"Current System state: SYS_STATE_GET_ACCEL_DATA\r\n"
	};

//queue to store events
xQueueHandle queue_stateMachineEvents = NULL;
systemStates_t currentSystemState = SYS_STATE_OFF;
//external variables
#ifdef ENABLE_EM_SENSORS
extern drv_twi_config_t twiConfig[];
extern slave_twi_config_t em7180Config[];
extern task_em_config_t task_em_config[];
#endif
extern quinticConfiguration_t quinticConfig[];
extern fabricSenseConfig_t fsConfig;
extern unsigned long sgSysTickCount;
uint8_t ResetStatus = 0; //for Quintic init task
uint8_t vExpectedResetStatus = 0;	//Standard expected response for Quintic init task
uint8_t QResetCount = 0;
bool firstBoot = true; //flag that indicates to the 
extern drv_uart_config_t uart0Config;
extern brainSettings_t brainSettings;
//Reset task handle
xTaskHandle ResetHandle = NULL;

//static function forward declarations
void processEvent(eventMessage_t eventMsg);
void stateEntry_PowerDown();
void stateEntry_Reset();
void stateExit_Reset();
void stateEntry_Idle();
void stateExit_Idle();
void stateEntry_Recording();
void stateExit_Recording();
void stateEntry_Error();
void stateExit_Error();
void stateEntry_GetAccelData();
void stateExit_GetAccelData();
static void CheckInitQuintic();
static void PreSleepProcess();
static void PostSleepProcess();
status_t reloadConfigSettings();
static void setCurrentSystemState(systemStates_t state);
void lowBatteryBlink();

xTimerHandle TimeOutTimer = NULL, sdTimeOutTimer = NULL;
volatile bool sdInsertWaitTimeoutFlag = FALSE;

void vTimeOutTimerCallback( xTimerHandle xTimer )
{
	//if auto off is enabled. 
	if(brainSettings.autoTurnOff)
	{
		#ifdef USES_NEW_POWER_BOARD 
		//if we use the new power board, set the PB_GPIO low to indicate that we need to power down
		drv_gpio_setPinState(DRV_GPIO_PIN_PB_GPIO, DRV_GPIO_PIN_STATE_LOW); 
		#else
		 task_stateMachine_EnqueueEvent(SYS_EVENT_POWER_SWITCH, 0x00);
		#endif
		
	}
}

void vSdTimeOutTimerCallback( xTimerHandle xTimer )
{
	sdInsertWaitTimeoutFlag = TRUE;
}

//task to handle the events
void task_stateMachineHandler(void *pvParameters)
{
	//Start Idle-time out timer 
	TimeOutTimer = xTimerCreate("Time Out Timer", (MAX_IDLE_TIMEOUT/portTICK_RATE_MS), pdFALSE, NULL, vTimeOutTimerCallback);
	if (TimeOutTimer == NULL)
	{
		printf("Failed to create timer task code %d\r\n", TimeOutTimer);
	}	
	queue_stateMachineEvents = xQueueCreate( 10, sizeof(eventMessage_t));
	if(queue_stateMachineEvents == 0)
	{
		// Queue was not created this is an error!
		debugPrintString("an error has occurred, state machine queue creation failed. \r\n");
		return;
	}	
	eventMessage_t eventMessage = {.sysEvent = SYS_EVENT_POWER_SWITCH, .data = 0x0000}; 
	//from start immediately put the system into power down mode. 
	xQueueSendToBack( queue_stateMachineEvents,( void * ) &eventMessage,5); 
	while(1)
	{
		if(xQueueReceive(queue_stateMachineEvents, &(eventMessage), 100) == TRUE)
		{
			processEvent(eventMessage); 
		}
		vTaskDelay(50); 	
	}		
}

status_t task_stateMachine_EnqueueEvent(systemEvents_t eventType, uint16_t data)
{
	status_t status = STATUS_PASS; 
	eventMessage_t msg; 
	msg.sysEvent = eventType; 
	msg.data = data; 
	debugPrintString(systemEventNameString[eventType]);
	if(queue_stateMachineEvents != NULL)
	{
		if(xQueueSendToBack( queue_stateMachineEvents,( void * ) &msg,5) != TRUE)
		{
			status = STATUS_FAIL; 
		}
	}
	return status; 
}
systemStates_t getCurrentState()
{
	return currentSystemState;
}
//static functions
//event processor
void processEvent(eventMessage_t eventMsg)
{
	switch(eventMsg.sysEvent)
	{
		case SYS_EVENT_POWER_SWITCH:
		{
			if(currentSystemState == SYS_STATE_RECORDING)
			{
				//stop recording, then go to the off state. 
				stateExit_Recording(); 
			}
			else if (currentSystemState == SYS_STATE_GET_ACCEL_DATA)
			{
				stateExit_GetAccelData(); 
				stateExit_Recording(); 
			}			
			else if (currentSystemState == SYS_STATE_RESET)
			{				
				stateExit_Reset();
			}
			else if (currentSystemState == SYS_STATE_IDLE)
			{
				stateExit_Idle();
			}
			else if (currentSystemState == SYS_STATE_ERROR)
			{
				stateExit_Error();
			}
			else if(currentSystemState == SYS_STATE_POWER_DOWN)
			{
				//we should already be off, ignore
				break; 
			}
			//go to the power down state. 
			stateEntry_PowerDown(); 	
		}
		break;
		case SYS_EVENT_RECORD_SWITCH:
		{
			if(currentSystemState == SYS_STATE_IDLE)
			{
				//start recording
				stateExit_Idle();
				//go to get accel data first. 
				stateEntry_GetAccelData();
				//go to recording state
			}
			else if(currentSystemState == SYS_STATE_RECORDING)
			{
				//stop recording
				stateExit_Recording();
				//go to IDLE state
				stateEntry_Idle(); 
			}
		}
		break;
		case SYS_EVENT_RESET_SWITCH:
		{
			if(currentSystemState == SYS_STATE_RECORDING)
			{
				//stop recording
				stateExit_Recording();
			}
			else if (currentSystemState == SYS_STATE_GET_ACCEL_DATA)
			{
				stateExit_GetAccelData();
				stateExit_Recording();
			}
			else if(currentSystemState == SYS_STATE_RESET)
			{

				//Delete the on going Reset task to re-spawn again.
				if (ResetHandle != NULL)
				{
					vTaskDelete(ResetHandle);
				}
			}
			else if (currentSystemState == SYS_STATE_IDLE)
			{
				stateExit_Idle();
			}
			else if (currentSystemState == SYS_STATE_ERROR)
			{
				stateExit_Error();
			}
						 
			//start the reset process... 
			if (brainSettings.isLoaded == 1)
			{
				stateEntry_Reset();
			}
			else
			{
				stateEntry_Error();
			}
			 
		}
		break;
		case SYS_EVENT_IMU_DISCONNECT:
		{		
			if(currentSystemState == SYS_STATE_POWER_DOWN)
			{
				//do nothing, this is expected
				break;
			}
			if (currentSystemState == SYS_STATE_ERROR)
			{
				break;
			}
		}		
		case SYS_EVENT_OVER_CURRENT:
		case SYS_EVENT_BLE_ERROR:
		case SYS_EVENT_JACK_DETECT:
		if (currentSystemState == SYS_STATE_RESET)
		{
			stateExit_Reset();
		}
		if(currentSystemState == SYS_STATE_RECORDING)
		{
			//stop recording
			stateExit_Recording();
		}
		else if (currentSystemState == SYS_STATE_GET_ACCEL_DATA)
		{
			stateExit_GetAccelData();
			stateExit_Recording();
		}		
		stateEntry_Error(); 
		break;
		case SYS_EVENT_RESET_FAILED:
		{
			if(currentSystemState == SYS_STATE_RECORDING)
			{
				//stop recording.
				stateExit_Recording();
			}
			stateEntry_Error(); 			
		}
		break;
		case SYS_EVENT_SD_FILE_ERROR:
		{
			if (currentSystemState == SYS_STATE_RESET)
			{
				stateExit_Reset();
			}
			if (currentSystemState == SYS_STATE_RECORDING)
			{
				stateExit_Recording();
			}
			else if (currentSystemState == SYS_STATE_GET_ACCEL_DATA)
			{
				stateExit_GetAccelData();
				stateExit_Recording();
			}
			//SD card was removed clear all file open / loaded flags
			brainSettings.isLoaded = 0;
			//get the SD card task to actually close the files. 
			task_sdCard_CloseFile();
			task_debugLog_CloseFile(); 
			vTaskDelay(200); //wait for the files to get closed in the SD card task. 
			f_mount(LUN_ID_SD_MMC_0_MEM, NULL);
			stateEntry_Error(); 
		}
		break;
		case SYS_EVENT_LOW_BATTERY:
		{
			if(currentSystemState == SYS_STATE_RECORDING)
			{
				//stop recording
				stateExit_Recording();
			}
			else if (currentSystemState == SYS_STATE_GET_ACCEL_DATA)
			{
				stateExit_GetAccelData();
				stateExit_Recording();
			}
			else if (currentSystemState == SYS_STATE_IDLE)
			{
				stateExit_Idle();
			}
			else if (currentSystemState == SYS_STATE_ERROR)
			{
				stateExit_Error();
			}
			else if (currentSystemState == SYS_STATE_RESET)
			{
				stateExit_Reset();
			}
			//go to the power down state. 
			lowBatteryBlink();	
			stateEntry_PowerDown(); 		
		}
		break;
		case SYS_EVENT_RESET_COMPLETE:
		{
			if(currentSystemState != SYS_STATE_RESET)
			{
				//do nothing, this is weird, should not get here. 
				break;
			}
			//QResetCount++;	//Check if all three Quintics are past the initialization process
			int z;
			for (z=0; z<3; z++)
			{
				if (eventMsg.data == z)	//check for which Quintic is successfully initialized
				{
					ResetStatus |= (1u<<(z));	//Save the result to a result flag
				}
			}
			//go to the idle state
			if (QResetCount < 3)	//If not all Quintic were initialized,
			{
				CheckInitQuintic();	// pass the init command to the next one
			}
			else
			{
				if (ResetStatus == vExpectedResetStatus)	//Check if all of them were initialized
				{
					//go to the idle state
					stateEntry_Idle();
				}
				else
				{
					task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0);	//Assert Reset failed as one or more failed to initialize
				}
			}			
		}
		break;
		case SYS_EVENT_SD_CARD_DETECT:
		{
			if(reloadConfigSettings() == STATUS_PASS)
			{
				//perform reset only if loading the settings was successful
				stateEntry_Reset();
			}
			else
			{
				stateEntry_Error();
			}
		}
		break;
		case SYS_EVENT_POWER_UP_COMPLETE:
		{
			if(currentSystemState != SYS_STATE_POWER_DOWN)
			{
				//we should not have gotten this event here
				break;
			}
			//first thing to do after the power up is reload config settings 
			if(reloadConfigSettings() == STATUS_PASS)
			{	
				if(task_debugLog_OpenFile() == STATUS_PASS)
				{
					//perform reset only if loading the settings was successful
					stateEntry_Reset();
				}
				else
				{
					stateEntry_Error();
				}
			}
			else
			{
				stateEntry_Error();
			}
		}		
		break; 		
		case SYS_EVENT_GET_ACCEL_DATA_COMPLETE:
		{
			if(currentSystemState == SYS_STATE_GET_ACCEL_DATA)
			{
				stateExit_GetAccelData(); 
				stateEntry_Recording(); 				
			}
			
		}
		break;
		default:
		//do nothing, should never reach here
		break; 										
	}
}

/***********************************************************************************************
 * stateEntry_PowerDown()
 * @brief Put the processor to sleep and wait for interrupt on Power pin
 * @param void
 * @return void
 ***********************************************************************************************/
//entry and exit functions
//power down function (handles entry and exit)
void stateEntry_PowerDown()
{
	drv_gpio_pin_state_t pwSwState = DRV_GPIO_PIN_STATE_HIGH, lboState = DRV_GPIO_PIN_STATE_HIGH;
	bool pwrSwFlag = FALSE; 
	setCurrentSystemState(SYS_STATE_POWER_DOWN);	
	drv_led_set(DRV_LED_OFF, DRV_LED_SOLID);
	//disable the interrupts, except for the power button
	//it is assumed that the button has already been held for 5 seconds

	DisconnectImus(&quinticConfig[0]);
	DisconnectImus(&quinticConfig[1]);
	DisconnectImus(&quinticConfig[2]);
	task_fabSense_stop(&fsConfig);
	task_debugLog_CloseFile();
	
	//clear the settings loaded bit
	brainSettings.isLoaded = 0;
	
	//turn off the JACK power supplies (they're negatively asserted) 
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_HIGH);
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_HIGH);
	toggleJackEnables(DRV_GPIO_PIN_STATE_HIGH);
	//Put the BLE's in reset. 
	drv_gpio_setPinState(quinticConfig[0].resetPin, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(quinticConfig[1].resetPin, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(quinticConfig[2].resetPin, DRV_GPIO_PIN_STATE_LOW);	
	drv_gpio_setPinState(DRV_GPIO_PIN_BT_PWR_EN, DRV_GPIO_PIN_STATE_LOW);
	/* Put the processor to sleep, in this context with the systick timer
	*  dead, we will never leave, so initialization has to be done here too. 
	*   
	*/	
	debugPrintString("Sleep mode enabled\r\n");
	PreSleepProcess();
	//
	
	while (pwrSwFlag == FALSE)	//Stay in sleep mode until wakeup
	{
		//if first boot, don't go to sleep, wake up. 
		if(firstBoot == true)
		{			
			firstBoot = false; 
			#ifdef USES_NEW_POWER_BOARD
			drv_gpio_setPinState(DRV_GPIO_PIN_PB_GPIO, DRV_GPIO_PIN_STATE_HIGH);
			#endif
			break;
		}
		else
		{
			#ifdef USES_NEW_POWER_BOARD			
			drv_gpio_setPinState(DRV_GPIO_PIN_PB_GPIO, DRV_GPIO_PIN_STATE_LOW);
			while(1); //loop here forever... wait for power board to turn us off
			#endif
		}
		
		#ifndef USES_NEW_POWER_BOARD
		
		//cpu_irq_disable();	
		pmc_enable_sleepmode(0);	
		//Processor wakes up from sleep		
		delay_ms(WAKEUP_DELAY);
		drv_gpio_getPinState(DRV_GPIO_PIN_PW_SW, &pwSwState);	//poll the power switch
		drv_gpio_getPinState(DRV_GPIO_PIN_LBO, &lboState);	//poll Low battery out
		if(pwSwState == DRV_GPIO_PIN_STATE_LOW)	//check if it is a false wakeup
		{
			if (lboState == DRV_GPIO_PIN_STATE_HIGH)
			{
				pwrSwFlag = TRUE;
			}
			else
			{
				lowBatteryBlink();	//the battery is low, blink to indicate
				pwrSwFlag = FALSE;	//move back to Sleep state
			}
		}
		else
		{
			pwrSwFlag = FALSE;
		}
		#endif
	}
	PostSleepProcess();
	//blink BLUE to indicate wake up
	drv_led_activate_timer();
	drv_led_set(DRV_LED_BLUE, DRV_LED_SOLID);
	//enable the jacks
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW);
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW);
	toggleJackEnables(DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_BT_PWR_EN, DRV_GPIO_PIN_STATE_HIGH);
	//clear the queue of any messages
	uint32_t numberOfMessages = 0; 
	if(queue_stateMachineEvents != NULL)
	{
		numberOfMessages = uxQueueMessagesWaiting(queue_stateMachineEvents); 
	}	
	int i = 0; 
	eventMessage_t eventMessage;
	if(numberOfMessages > 0)
	{
		for(i=0;i<numberOfMessages;i++)
		{
			xQueueReceive(queue_stateMachineEvents, &(eventMessage), 10); 						
		}
	}
	//TODO check which jacks are connected to determine which IMUs are there	
	
	//send power up complete event
	eventMessage_t msg = {.sysEvent = SYS_EVENT_POWER_UP_COMPLETE, .data = 0};
	if(queue_stateMachineEvents != NULL)
	{
		xQueueSendToBack(queue_stateMachineEvents, &msg,5);
	}
		
}

/***********************************************************************************************
 * stateEntry_Reset()
 * @brief This initializes the Entry to Reset state
 * @param void
 * @return void
 ***********************************************************************************************/
//reset entry
void stateEntry_Reset()
{
	status_t status = STATUS_PASS; 
	ResetStatus = 0;
	eventMessage_t msg = {.sysEvent = SYS_EVENT_RESET_COMPLETE, .data = 0};
	//set current state to reset.
	setCurrentSystemState(SYS_STATE_RESET);
	//set LED to blue
	drv_led_set(DRV_LED_BLUE, DRV_LED_FLASH);
	QResetCount = 0;
	#ifdef USE_Q1_Q2
	//reset NOD power with JACK EN (only for stretch sense capable module)
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_HIGH); 
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_HIGH);
	toggleJackEnables(DRV_GPIO_PIN_STATE_HIGH);
	vTaskDelay(100); 
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW); 
	//drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW); 
	toggleJackEnables(DRV_GPIO_PIN_STATE_LOW);
	vTaskDelay(100); 
	#endif
	
	//Reset/init Qn
	CheckInitQuintic();
	
	//initialize fabric sense module
	status |= task_fabSense_init(&fsConfig); 
	if(status != STATUS_PASS)
	{
		debugPrintString("FabSense initialization failed\r\n");
		task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0x00);  		
	}
}

/***********************************************************************************************
 * stateExit_Reset()
 * @brief This initializes the Exit from Reset state
 * @param void
 * @return void
 ***********************************************************************************************/
void stateExit_Reset()
{
	if (ResetHandle != NULL)
	{
		vTaskDelete(ResetHandle);
	}
}

/***********************************************************************************************
 * stateEntry_Recording()
 * @brief This initializes the entry to recording state and sends the start command
 * @param void
 * @return void
 ***********************************************************************************************/
//recording entry
void stateEntry_Recording()
{
	status_t status;
	setCurrentSystemState(SYS_STATE_RECORDING);
	task_dataProcessor_startRecording();
	//send start command to quintics and fabric sense
	task_quintic_startRecording(&quinticConfig[0]);
	task_quintic_startRecording(&quinticConfig[1]);
	task_quintic_startRecording(&quinticConfig[2]);
	task_fabSense_start(&fsConfig);				
}

/***********************************************************************************************
 * stateExit_Recording()
 * @brief This initializes the exit from recording state and sends the stop command
 * @param void
 * @return void
 ***********************************************************************************************/
//recording exit
void stateExit_Recording()
{
	//send stop command to quintics and fabric sense
	task_quintic_stopRecording(&quinticConfig[0]);
	task_quintic_stopRecording(&quinticConfig[1]);
	task_quintic_stopRecording(&quinticConfig[2]);
	task_fabSense_stop(&fsConfig);	
	//wait for a bit for the data to be processed.
	vTaskDelay(100);
	//close the data file for the current recording
	task_sdCard_CloseFile();	
}

/***********************************************************************************************
 * stateEntry_Idle()
 * @brief This initializes the entry to idle state
 * @param void
 * @return void
 ***********************************************************************************************/
//idle entry
void stateEntry_Idle()
{
	setCurrentSystemState(SYS_STATE_IDLE);
	xTimerReset(TimeOutTimer, 0); 
	drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);
}

/***********************************************************************************************
 * stateExit_Idle()
 * @brief This initializes the exit from idle state
 * @param void
 * @return void
 ***********************************************************************************************/
//idle exit
void stateExit_Idle()
{
	xTimerStop(TimeOutTimer, 0); 
}

/***********************************************************************************************
 * stateEntry_Error()
 * @brief This initializes the entry to error state
 * @param void
 * @return void
 ***********************************************************************************************/
//Error state entry
void stateEntry_Error()
{
	setCurrentSystemState(SYS_STATE_ERROR);
	debugPrintString(systemStateNameString[currentSystemState]);	
	xTimerReset(TimeOutTimer, 0);
	drv_led_set(DRV_LED_YELLOW, DRV_LED_FLASH);
}

/***********************************************************************************************
 * stateExit_Error()
 * @brief This initializes the exit from error state
 * @param void
 * @return void
 ***********************************************************************************************/
//Error state exit
void stateExit_Error()
{
	xTimerStop(TimeOutTimer, 0);
}

/***********************************************************************************************
 * stateEntry_GetAccelData()
 * @brief This initializes the Entry to the get acceleration data state
 * @param void
 * @return void
 ***********************************************************************************************/
void stateEntry_GetAccelData()
{
	task_quintic_sendConnectMsg(&quinticConfig[0]);
	task_quintic_sendConnectMsg(&quinticConfig[1]);
	task_quintic_sendConnectMsg(&quinticConfig[2]);
	//wait for a bit.
	vTaskDelay(100);
	setCurrentSystemState(SYS_STATE_GET_ACCEL_DATA);
	drv_led_set(DRV_LED_RED, DRV_LED_FLASH);
	//open new log file
	if(task_sdCard_OpenNewFile() != STATUS_PASS)
	{
		//this is an error, we should probably do something
		debugPrintString("Cannot open new file to write records\r\n");
		task_stateMachine_EnqueueEvent(SYS_EVENT_SD_FILE_ERROR, 0);
		return;
	}
	//wait for user to get into position
	vTaskDelay(2500);
	if(brainSettings.numberOfAccelFrames > 0)
	{
		task_quintic_startGetAccelData(&quinticConfig[0]);
		task_quintic_startGetAccelData(&quinticConfig[1]);
		task_quintic_startGetAccelData(&quinticConfig[2]);
		task_dataProcessor_startGetAccelData(brainSettings.numberOfAccelFrames); 
	}
	else
	{
		task_stateMachine_EnqueueEvent(SYS_EVENT_GET_ACCEL_DATA_COMPLETE, 0x00); 
	}	
}

/***********************************************************************************************
 * stateExit_GetAccelData()
 * @brief This initializes the exit from the acceleration data state
 * @param void
 * @return void
 ***********************************************************************************************/
void stateExit_GetAccelData()
{
	task_quintic_stopGetAccelData(&quinticConfig[0]);
	task_quintic_stopGetAccelData(&quinticConfig[1]);
	task_quintic_stopGetAccelData(&quinticConfig[2]);	
}

/***********************************************************************************************
 * CheckInitQuintic()
 * @brief This creates quintic initializing task for the next  quintic
 * @param void
 * @return void
 ***********************************************************************************************/
static void CheckInitQuintic()
{
	switch(QResetCount)
	{
		//Check if the specific Quintic task is spawned and any IMUs are assigned to it.
		case 0:
			if((quinticConfig[0].isinit) && (quinticConfig[0].expectedNumberOfNods > 0))
			{
				//status |= task_quintic_initializeImus(&quinticConfig[i]);
				int retCode = xTaskCreate(task_quintic_initializeImus, "Qi", TASK_IMU_INIT_STACK_SIZE, (void*)&quinticConfig[0], TASK_IMU_INIT_PRIORITY, &ResetHandle );
				if (retCode != pdPASS)
				{
					debugPrintString("Failed to create Q0 init task \r\n");
					task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0x00);
				}
			}
			else
			{
				//the quintic is absent in the settings file, send an event 
				task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_COMPLETE, 0xff);
			}
			QResetCount = 1;	//the init task has been spawned, move to next quintic.
		break;
		case 1:
			if ((quinticConfig[1].isinit) && (quinticConfig[1].expectedNumberOfNods > 0))
			{
				int retCode = xTaskCreate(task_quintic_initializeImus, "Qi", TASK_IMU_INIT_STACK_SIZE, (void*)&quinticConfig[1], TASK_IMU_INIT_PRIORITY, &ResetHandle );
				if (retCode != pdPASS)
				{
					debugPrintStringInt("Failed to create Q1 init task \r\n", retCode);
					task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0x00);
				}
			}
			else
			{
				//the quintic is absent in the settings file, send an event
				task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_COMPLETE, 0xff);
			}
			QResetCount = 2;	//the init task has been spawned, move to next quintic.
		break;
		case 2:
			if ((quinticConfig[2].isinit) && (quinticConfig[2].expectedNumberOfNods > 0))
			{
				int retCode = xTaskCreate(task_quintic_initializeImus, "Qi", TASK_IMU_INIT_STACK_SIZE, (void*)&quinticConfig[2], TASK_IMU_INIT_PRIORITY, &ResetHandle );
				if (retCode != pdPASS)
				{
					debugPrintString("Failed to create Q2 init task \r\n");
					task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_FAILED, 0x00);
				}
			}
			else
			{
				//the quintic is absent in the settings file, send an event
				task_stateMachine_EnqueueEvent(SYS_EVENT_RESET_COMPLETE, 0xff);
			}
			QResetCount = 3;	//the init task has been spawned, move to next quintic.
		break;
		default:
		// do nothing, should never reach here.
		break;
	}

}

/***********************************************************************************************
 * PreSleepProcess()
 * @brief This does the necessary processing before putting the processor to sleep
 * @param void
 * @return void
 ***********************************************************************************************/
static void PreSleepProcess()
{
	//supc_disable_brownout_detector(SUPC);	
	//unmount the SD card. 
	static FRESULT res;
	res = f_mount(LUN_ID_SD_MMC_0_MEM, NULL);
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;	//disable the systick timer
	deInitAllUarts();
	drv_gpio_disable_interrupt_all();
	NVIC_DisableIRQ(WDT_IRQn);
	NVIC_ClearPendingIRQ(WDT_IRQn);	
	drv_gpio_config_interrupt(DRV_GPIO_PIN_PW_SW, DRV_GPIO_INTERRUPT_LOW_EDGE);
	drv_gpio_enable_interrupt(DRV_GPIO_PIN_PW_SW); 		
}

/***********************************************************************************************
 * PostSleepProcess()
 * @brief This does the necessary processing required after waking up the processor from sleep
 * @param void
 * @return void
 ***********************************************************************************************/
static void PostSleepProcess()
{
	drv_gpio_clear_Int(DRV_GPIO_PIN_PW_SW);	//Clear the interrupt generated by power switch flag
	drv_gpio_initializeAll();
	initAllUarts();
	debugPrintString("Exit Sleep mode\r\n");
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;	//enable the systick timer
	NVIC_EnableIRQ(WDT_IRQn);		
}

/***********************************************************************************************
 * reloadConfigSettings()
 * @brief Reload the config settings from SD card 
 * @param void
 * @return void
 ***********************************************************************************************/
status_t reloadConfigSettings()
{
	static FATFS fs;
	static FATFS* fs1;	//pointer to FATFS structure used to check free space
	static FRESULT res;
	static DWORD freeClusters, freeSectors, totalSectors;
	status_t result = STATUS_FAIL;
	Ctrl_status status; 
	drv_gpio_pin_state_t sdCdPinState;
	int i;
	
	drv_gpio_getPinState(DRV_GPIO_PIN_SD_CD, &sdCdPinState);
	//check the pin value to see if the card is detected. 
	if (sdCdPinState != SD_MMC_0_CD_DETECT_VALUE)
	{
		//card has not been detected.
		sdInsertWaitTimeoutFlag = FALSE;	//clear the flag for resuse
		return result;
	}
	//SD-Card present, reconfigure the interrupt to use it for detecting removal
	drv_gpio_config_interrupt(DRV_GPIO_PIN_SD_CD, DRV_GPIO_INTERRUPT_LOW_EDGE);
	//initialize the sd card... assuming that the card has not been initialized? 
	sd_mmc_init();
	sdTimeOutTimer = xTimerCreate("SD insert tmr", (SD_INSERT_WAIT_TIMEOUT/portTICK_RATE_MS), pdFALSE, NULL, vSdTimeOutTimerCallback);
	if (sdTimeOutTimer == NULL)
	{
		debugPrintString("Failed to create SD card timer\r\n");
	}
	xTimerStart(sdTimeOutTimer, 0);
	do
	{
		status = sd_mmc_test_unit_ready(0);
		if (CTRL_FAIL == status)
		{
			debugPrintString("Card install FAIL\n\r");
			debugPrintString("Please unplug and re-plug the card.\n\r");
			while ((CTRL_NO_PRESENT != sd_mmc_check(0)) && (sdInsertWaitTimeoutFlag == FALSE))
			{
			}
		}
	} while ((CTRL_GOOD != status) && (sdInsertWaitTimeoutFlag == FALSE));
	
	sdInsertWaitTimeoutFlag = FALSE;	//clear the flag for reuse
	xTimerStop(sdTimeOutTimer, 0);
	xTimerDelete(sdTimeOutTimer, 0);
	
	/*	Mount the SD card	*/
	if(status == CTRL_GOOD)
	{
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		if (res == FR_INVALID_DRIVE)
		{
			debugPrintString("Error: Invalid Drive\r\n");
			return result;
		}
		
		//Check the free space on card
		result = f_getfree("0:", &freeClusters, &fs1);
		if (result != FR_OK)
		{
			result = STATUS_FAIL;
			debugPrintString("Error: Cannot calculate free space\r\n");
			return result;
		}
		totalSectors = (fs1->n_fatent -2) * fs1->csize;
		freeSectors = freeClusters * fs1->csize;	
		if ((totalSectors/2) - (freeSectors/2) > SD_DISK_SPACE_LOW_THRESHOLD)	
		{
			result = STATUS_FAIL;
			debugPrintString("Error: Low disk space on SD-card\r\n");
			return result;
		}
		
		//prevent system to go in reset state on button press event after a failed config load
		result = STATUS_PASS;
		if(loadSettings(SETTINGS_FILENAME) != STATUS_PASS)
		{
			result = STATUS_FAIL;
			debugPrintString("failed to get read settings\r\n");
			return result;
		}
		brainSettings.isLoaded = 1;
		for (i=0; i<3; i++)	//Update the vExpectedResetStatus value according to new settings file
		{
			if (quinticConfig[i].expectedNumberOfNods > 0)
			{
				vExpectedResetStatus |= (1u<<i);
			}
		}
	}
	return result;
}

/***********************************************************************************************
 * setCurrentSystemState(systemStates_t state)
 * @brief Sets the current system state 
 * @param systemStates_t state
 * @return void
 ***********************************************************************************************/
static void setCurrentSystemState(systemStates_t state)
{
	currentSystemState = state; 
	debugPrintString(systemStateNameString[currentSystemState]);	
}

/***********************************************************************************************
 * lowBatteryBlink()
 * @brief Blink the LED to indicate low battery 
 * @param void
 * @return void
 ***********************************************************************************************/
void lowBatteryBlink()
{
	//Blink the LED connected to STAT pin to indicate battery low
	#ifndef USES_NEW_POWER_BOARD //if we use the new power board don't blink the LED
	for (int i = 0; i < 3; i++)
	{
		drv_gpio_setPinState(DRV_GPIO_PIN_PB_GPIO, DRV_GPIO_PIN_STATE_HIGH);
		delay_ms(LED_BLINK_RATE);
		drv_gpio_setPinState(DRV_GPIO_PIN_PB_GPIO, DRV_GPIO_PIN_STATE_LOW);
		delay_ms(LED_BLINK_RATE);
	}
	#endif
}