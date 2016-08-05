/*
 * mgr_managerTask.c
 *
 * Created: 2/29/2016 3:49:21 PM
 *  Author: sean
 */ 
#include <string.h>
#include "sam4s2a.h"
#include "drv_gpio.h"
#include "drv_led.h"
#include "brd_board.h"
#include "cmd_commandProc.h"
#include "mgr_managerTask.h"
#include "dat_dataRouter.h"
#include "chrg_chargeMonitor.h"
#include "sen_sensorHandler.h"
#include "brd_dataBoardManager.h"

xQueueHandle mgr_eventQueue = NULL;
xTimerHandle pwrButtonTimer = NULL;
volatile mgr_systemStates_t currentSystemState = SYS_STATE_POWER_OFF; 

//external dependencies

extern drv_uart_config_t uart0Config;
extern drv_uart_config_t uart1Config;
extern xQueueHandle cmd_queue_commandQueue;
extern chrg_chargerState_t chrg_currentChargerState; 
extern uint32_t powerButtonLowCount;


dat_dataRouterConfig_t dataRouterConfiguration = 
{
	.dataBoardUart = &uart1Config,
	.daughterBoard = &uart0Config 
};

chrg_chargeMonitorConfig_t chargeMonitorConfiguration = 
{
	.pin_pg = DRV_GPIO_PIN_CHRG_PG,
	.pin_stat1 = DRV_GPIO_PIN_CHRG_STAT1,
	.pin_stat2 = DRV_GPIO_PIN_CHRG_STAT2
};

//forward static declarations
static void powerButtonHandler_HighEdge();
static void powerButtonHandler_LowEdge();
static void powerButtonTimerCallback();
static void enterSleepMode();
static void PostSleepProcess();
static void PreSleepProcess();
static bool UsbConnected();
static void enterPowerDownChargeState();
static void exitPowerDownChargeState();
static void clearAllEvents(); 
/***********************************************************************************************
 * mgr_managerTask(void *pvParameters)
 * @brief Handles queued events for the power board.  
 * @param pvParameters, void pointer to structure containing data router configuration. 
 * @return void
 ***********************************************************************************************/
void mgr_managerTask(void *pvParameters)
{
	mgr_eventMessage_t msgEvent; 
	//initialize the board
	
	brd_board_init(); 
	printf("startApplication!\r\n");
	//initialize power button listener. 	
	drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_PWR_BTN, DRV_GPIO_INTERRUPT_LOW_EDGE,powerButtonHandler_LowEdge);
	mgr_eventQueue = xQueueCreate( 10, sizeof(mgr_eventMessage_t));
	pwrButtonTimer = xTimerCreate("PowerBnt timer", (SLEEP_ENTRY_WAIT_TIME/portTICK_RATE_MS), pdFALSE, NULL, powerButtonTimerCallback);
	//start all the other tasks
	int retCode = 0;
	retCode = xTaskCreate(chrg_task_chargeMonitor, "CHRG", TASK_CHRG_MON_STACK_SIZE, &chargeMonitorConfiguration, TASK_CHRG_MON_STACK_PRIORITY, NULL);
	if (retCode != pdPASS)
	{
		printf("Failed to create CHRG task code %d\r\n", retCode);
	}
	retCode = xTaskCreate(cmd_task_commandProcesor, "CMD", TASK_COMMAND_PROC_STACK_SIZE, NULL, TASK_COMMAND_PROC_PRIORITY, NULL);
	if (retCode != pdPASS)
	{
		printf("Failed to create CMD task code %d\r\n", retCode);
	}
	retCode = xTaskCreate(dat_task_dataRouter, "DAT", TASK_DATA_ROUTER_STACK_SIZE, &dataRouterConfiguration, TASK_DATA_ROUTER_PRIORITY, NULL);
	if (retCode != pdPASS)
	{
		printf("Failed to create DAT task code %d\r\n", retCode);
	}
	retCode = xTaskCreate(dat_dataBoardManager, "DB", TASK_DATA_BOARD_MANAGER_STACK_SIZE, NULL, TASK_DATA_BOARD_MANAGER_PRIORITY, NULL);
	if (retCode != pdPASS)
	{
		printf("Failed to create DB task code %d\r\n", retCode);
	}
	retCode = xTaskCreate(sen_sensorHandlerTask, "SEN", TASK_SENSOR_HANDLER_STACK_SIZE, NULL, TASK_SENSOR_HANDLER_PRIORITY, NULL);
	if (retCode != pdPASS)
	{
		printf("Failed to create SEN task code %d\r\n", retCode);
	}
	
	drv_led_set(DRV_LED_GREEN, DRV_LED_FLASH);
	//enable power to the data board
	drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW);	
	drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_PULLED_LOW);
	currentSystemState = SYS_STATE_POWER_ON;
	//by default enable fast charging
	drv_gpio_setPinState(DRV_GPIO_PIN_CHRG_SEL, DRV_GPIO_PIN_STATE_HIGH);	
	uint32_t pwrButtonHeldLowCount = 0;
	drv_gpio_pin_state_t pwrButtonState = DRV_GPIO_PIN_STATE_HIGH; 
	while(1)
	{
		//test code for the power board. 
		if(xQueueReceive( mgr_eventQueue, &(msgEvent), 50) == TRUE)
		{	
			switch(msgEvent.sysEvent)
			{
				case SYS_EVENT_POWER_SWITCH:
				{
					dat_sendDebugMsgToDataBoard("PwrBrdMsg:Received PwrSwitch Event\r\n");
					if(currentSystemState == SYS_STATE_POWER_ON)
					{						
						if(UsbConnected() == true)
						{
							//go to power down charge state
							enterPowerDownChargeState();
						}
						else
						{
							//go to power down state.
							enterSleepMode();
						}						
					}
					else if(currentSystemState == SYS_STATE_POWER_OFF_CHARGING)
					{ 
						//we need to power everything back on
						exitPowerDownChargeState();
					}						
				}
				break;
				case SYS_EVENT_POWER_UP_COMPLETE:
				{
					
				}
				break;
				case SYS_EVENT_LOW_BATTERY:
				{
					vTaskDelay(200);
					enterSleepMode();
				}
				break; 
				case SYS_EVENT_JACK_DETECT:
				{
					
				}
				break;
				case SYS_EVENT_USB_CONNECTED:
				{
					dat_sendDebugMsgToDataBoard("PwrBrdMsg:USB Connected\r\n");
					//toggle fast Charge
					drv_gpio_setPinState(DRV_GPIO_PIN_CHRG_SEL, DRV_GPIO_PIN_STATE_LOW);
					vTaskDelay(200);
					drv_gpio_setPinState(DRV_GPIO_PIN_CHRG_SEL, DRV_GPIO_PIN_STATE_HIGH);
					
				}
				break;
				case SYS_EVENT_USB_DISCONNECTED:
				{
					//if we're currently charging with the power off, then go to sleep mode
					if(currentSystemState == SYS_STATE_POWER_OFF_CHARGING)
					{
						enterSleepMode();
					}
					dat_sendDebugMsgToDataBoard("PwrBrdMsg:USB Disconnected\r\n");
				}
				break;
				default:
				{
					
				}
				break;
				
			}
		}
		vTaskDelay(150);
		//drv_gpio_getPinState(DRV_GPIO_PIN_PWR_BTN,&pwrButtonState);
		//if(pwrButtonState == DRV_GPIO_PIN_STATE_LOW)
		//{
			//pwrButtonHeldLowCount++;
		//}
		//else
		//{
			//pwrButtonHeldLowCount = 0;
		//}
		//if(pwrButtonHeldLowCount == 75)
		//{
			////reset the board.
			//rstc_start_software_reset(RSTC);
		//}
	}
}

//static functions
void powerButtonTimerCallback()
{
	drv_gpio_pin_state_t pwSwState = DRV_GPIO_PIN_STATE_HIGH; 
	mgr_eventMessage_t pwrDownEvent = 
	{
		.sysEvent = SYS_EVENT_POWER_SWITCH,
		.data = 0		
	};
	//check if the button is still low
	drv_gpio_getPinState(DRV_GPIO_PIN_PWR_BTN, &pwSwState);	//poll the power switch
	if(pwSwState == DRV_GPIO_PIN_STATE_LOW)
	{	
		////The power button is still low, send the event. 
		//if(xQueueSendToBack(mgr_eventQueue,&pwrDownEvent,0) != TRUE)
		//{
			////this is an error, we should log it.
			//
		//}	
	}
	
}
static void powerButtonHandler_HighEdge(uint32_t ul_id, uint32_t ul_mask)
{
	uint32_t PinMask = pio_get_pin_group_mask(DRV_GPIO_ID_PIN_PWR_BTN);
	pio_disable_interrupt(PIOA, PinMask);
	uint32_t ReadIsr = PIOA->PIO_ISR;
	if (PinMask == ul_mask)
	{
		drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_PWR_BTN, DRV_GPIO_INTERRUPT_LOW_EDGE,powerButtonHandler_LowEdge);
		xTimerStopFromISR(pwrButtonTimer,pdFALSE);
	}
	pio_enable_interrupt(PIOA, PinMask);	

}
volatile uint32_t hasWoken = pdFALSE; 
static void powerButtonHandler_LowEdge(uint32_t ul_id, uint32_t ul_mask)
{
	uint32_t PinMask = pio_get_pin_group_mask(DRV_GPIO_ID_PIN_PWR_BTN);
	pio_disable_interrupt(PIOA, PinMask);
	uint32_t ReadIsr = PIOA->PIO_ISR;
	
	if (PinMask == ul_mask)
	{
		//drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_PWR_BTN, DRV_GPIO_INTERRUPT_HIGH_EDGE,powerButtonHandler_HighEdge);
		xTimerResetFromISR(pwrButtonTimer,&hasWoken);
	}
	pio_enable_interrupt(PIOA, PinMask);	
	if( hasWoken != pdFALSE )
    {
        /* Call the interrupt safe yield function here (actual function
        depends on the FreeRTOS port being used). */
		taskYIELD();
    }
	
}

static void enterSleepMode()
{
	cmd_commandPacket_t packet;
	uint32_t powerOnFlag = FALSE, chargeFlag = FALSE;
	drv_gpio_pin_state_t pwSwState = DRV_GPIO_PIN_STATE_HIGH;  
	drv_gpio_pin_state_t chargingDetect = DRV_GPIO_INTERRUPT_LOW_EDGE; 
	strncpy(packet.packetData,"Power\r\n",CMD_INCOMING_CMD_SIZE_MAX); 
	uint32_t loopCount = 0;
	packet.packetSize = strlen(packet.packetData); 
	//Send power down message to data board
	if(cmd_queue_commandQueue != NULL)
	{
		if(xQueueSendToBack(cmd_queue_commandQueue,( void * ) &packet,5) != TRUE)
		{
			//this is an error, we should log it.
		}
	}	
	//turn off power to both Jacks
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_HIGH);
	//wait for GPIO to go low (indication that data board is ready to sleep)
	loopCount = 0;
	uint32_t startTime = xTaskGetTickCount(); 
	drv_gpio_pin_state_t gpioPinState = DRV_GPIO_PIN_STATE_HIGH;  
	vTaskDelay(2000);
	while(loopCount < 30)
	{		
		drv_gpio_getPinState(DRV_GPIO_PIN_GPIO,&gpioPinState);
		if(gpioPinState == DRV_GPIO_PIN_STATE_LOW)
		{
			//the data board is ready to shutdown, leave the loop. 
			break;
		}
		vTaskDelay(10);
		loopCount++;
	}
	
	//turn off power to the data board
	drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_LOW);
	//wait for the button to go high
	drv_gpio_getPinState(DRV_GPIO_PIN_PWR_BTN, &pwSwState);	//poll the power switch
	loopCount = 0;
	//wait up to 5 seconds for the button to go high before going to sleep. 
	while(loopCount < 50)
	{		
		vTaskDelay(100); 
		drv_gpio_getPinState(DRV_GPIO_PIN_PWR_BTN, &pwSwState);	//poll the power switch
		if(pwSwState == DRV_GPIO_PIN_STATE_HIGH)
		{
			break;
		}
		loopCount++;
	}	
	//go to sleep, and wait for power button press again. 
	PreSleepProcess();
	while ((powerOnFlag == FALSE) && (chargeFlag == FALSE))	//Stay in sleep mode until wakeup
	{
		//cpu_irq_disable();
		//pmc_enable_sleepmode(0);
		uint32_t startupInput = (1<<4 | 1<<14); //WKUP14 and WKUP4 (power button and USB detect) 
		pmc_set_fast_startup_input(startupInput);
		//uint32_t regVal = SUPC->SUPC_SR;  
		pmc_sleep(SAM_PM_SMODE_WAIT);
		//drv_gpio_togglePin(DRV_GPIO_PIN_LED_BLUE);
		//Processor wakes up from sleep
		delay_ms(WAKEUP_DELAY);
		drv_gpio_getPinState(DRV_GPIO_PIN_PWR_BTN, &pwSwState);	//poll the power switch
		drv_gpio_getPinState(DRV_GPIO_ID_PIN_CHRG_PG, &chargingDetect); //
		if(pwSwState == DRV_GPIO_PIN_STATE_LOW)	//check if it is a false wakeup
		{
			//The power button has been held long enough, break the loop and power on. 
			powerOnFlag = TRUE; 
		}
		else
		{
			powerOnFlag = FALSE;
		}
		if(chargingDetect == DRV_GPIO_PIN_STATE_LOW)
		{
			chargeFlag = TRUE; 
		}
		else
		{
			chargeFlag = FALSE; 
		}
	}
	PostSleepProcess();
	if(powerOnFlag == TRUE)
	{	
		//set the GPIO pin to be an input. 
		drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_PULLED_LOW);
		//enable power to the data board
		drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_HIGH);
		//wait for brain mcu to start up
		gpioPinState = DRV_GPIO_PIN_STATE_LOW; 
		loopCount = 0;
		//set the count over the threshold so it needs to be reset by it going high again. 
		powerButtonLowCount = 16;
		while(loopCount < 30)
		{
			drv_gpio_getPinState(DRV_GPIO_PIN_GPIO,&gpioPinState);
			if(gpioPinState == DRV_GPIO_PIN_STATE_HIGH)
			{
				//the data board is powered up, break loop
				break;
			}
			vTaskDelay(50);
			loopCount++;
		}
		//invalidate the current charger state so that it is re-evaluated
		chrg_currentChargerState = CHRG_CHARGER_STATE_INVALID_CODE; 
		//send the date time command to the brain MCU. 	
		vTaskDelay(2000);
		cmd_sendDateTimeCommand(); 
		//enable power to both Jacks
		vTaskDelay(100);
		//TODO add switching auto-enabling to this code. 
		drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW);
		drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW);	
	
		//clear the events
		clearAllEvents();
		currentSystemState = SYS_STATE_POWER_ON; 
	}
	else if(chargeFlag == TRUE)
	{
		//invalidate the current charger state so that it is re-evaluated
		chrg_currentChargerState = CHRG_CHARGER_STATE_INVALID_CODE;
		//clear the events
		clearAllEvents();
		currentSystemState = SYS_STATE_POWER_OFF_CHARGING;		
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
	drv_led_set(DRV_LED_OFF,DRV_LED_SOLID);	
	//supc_disable_brownout_detector(SUPC);	
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;	//disable the systick timer
	brd_deInitAllUarts();
	drv_gpio_disable_interrupt_all();
	//disable the watchdog
	drv_gpio_config_interrupt(DRV_GPIO_PIN_PWR_BTN, DRV_GPIO_INTERRUPT_LOW_EDGE);
	drv_gpio_enable_interrupt(DRV_GPIO_PIN_PWR_BTN);
	//drv_gpio_config_interrupt(DRV_GPIO_PIN_USB_DET, DRV_GPIO_INTERRUPT_HIGH_EDGE);
	//drv_gpio_enable_interrupt(DRV_GPIO_PIN_USB_DET);
	NVIC_DisableIRQ(WDT_IRQn);
	NVIC_ClearPendingIRQ(WDT_IRQn);
	
}

/***********************************************************************************************
 * PostSleepProcess()
 * @brief This does the necessary processing required after waking up the processor from sleep
 * @param void
 * @return void
 ***********************************************************************************************/
static void PostSleepProcess()
{
	drv_gpio_clear_Int(DRV_GPIO_PIN_PWR_BTN);	//Clear the interrupt generated by power switch flag
	drv_gpio_initializeAll();
	drv_gpio_config_interrupt_handler(DRV_GPIO_PIN_PWR_BTN, DRV_GPIO_INTERRUPT_LOW_EDGE,powerButtonHandler_LowEdge);
	brd_initAllUarts();
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;	//enable the systick timer
	
	//pmc_disable_periph_clk(ID_WDT);
	NVIC_EnableIRQ(WDT_IRQn);		
}

static bool UsbConnected()
{
	drv_gpio_pin_state_t usbConnectedState = DRV_GPIO_PIN_STATE_LOW; 
	drv_gpio_getPinState(DRV_GPIO_PIN_USB_DET, &usbConnectedState);
	if(usbConnectedState == DRV_GPIO_PIN_STATE_HIGH)
	{
		return true;
	}
	else
	{
		return false;
	}	
}

static void enterPowerDownChargeState()
{
	cmd_commandPacket_t packet;
	drv_gpio_pin_state_t pwSwState = DRV_GPIO_PIN_STATE_HIGH;
	strncpy(packet.packetData,"Power\r\n",CMD_INCOMING_CMD_SIZE_MAX);
	packet.packetSize = strlen(packet.packetData);
	//Send power down message to data board
	if(cmd_queue_commandQueue != NULL)
	{
		if(xQueueSendToBack(cmd_queue_commandQueue,( void * ) &packet,5) != TRUE)
		{
			//this is an error, we should log it.
		}
	}
	//turn off power to both Jacks
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_HIGH);
	//wait for GPIO to go low (indication that data board is ready to sleep)
	uint32_t startTime = xTaskGetTickCount();
	drv_gpio_pin_state_t gpioPinState = DRV_GPIO_PIN_STATE_HIGH;
	uint32_t loopCount = 0;	
	vTaskDelay(2000);
	while(loopCount < 30)
	{
		drv_gpio_getPinState(DRV_GPIO_PIN_GPIO,&gpioPinState);
		if(gpioPinState == DRV_GPIO_PIN_STATE_LOW)
		{
			//the data board is ready to shutdown, leave the loop.
			break;
		}
		vTaskDelay(100);
		loopCount++;
	}	
	drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_LOW);
	//turn off power to the data board
	drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_LOW);
	
	currentSystemState = SYS_STATE_POWER_OFF_CHARGING; 
		
}
static void exitPowerDownChargeState()
{
	//set the GPIO pin to be an input.
	drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_PULLED_LOW);
	//enable power to the data board
	drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_HIGH);
	//wait for brain mcu to start up
	drv_gpio_pin_state_t gpioPinState = DRV_GPIO_PIN_STATE_LOW;
	uint32_t loopCount = 0;
	while(loopCount < 30)
	{
		drv_gpio_getPinState(DRV_GPIO_PIN_GPIO,&gpioPinState);
		if(gpioPinState == DRV_GPIO_PIN_STATE_HIGH)
		{
			//the data board is ready to shutdown, leave the loop.
			break;
		}
		vTaskDelay(100);
		loopCount++;
	}	
	//invalidate the current charger state so that it is re-evaluated
	chrg_currentChargerState = CHRG_CHARGER_STATE_INVALID_CODE;
	//send the date time command to the brain MCU.
	cmd_sendDateTimeCommand();
	//enable power to both Jacks
	vTaskDelay(100);
	//TODO add switching auto-enabling to this code.
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW);
	currentSystemState = SYS_STATE_POWER_ON; 
	clearAllEvents();
}

void clearAllEvents()
{
	uint32_t numberOfMessages = 0; 
	if(mgr_eventQueue != NULL)
	{
		numberOfMessages = uxQueueMessagesWaiting(mgr_eventQueue); 
	}	
	int i = 0; 
	mgr_eventMessage_t eventMessage;
	if(numberOfMessages > 0)
	{
		for(i=0;i<numberOfMessages;i++)
		{
			xQueueReceive(mgr_eventQueue, &(eventMessage), 10); 						
		}
	}		
}
