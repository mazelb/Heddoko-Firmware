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



xQueueHandle mgr_eventQueue = NULL;
xTimerHandle pwrButtonTimer = NULL;


//external dependancies

extern drv_uart_config_t uart0Config;
extern drv_uart_config_t uart1Config;
extern xQueueHandle cmd_queue_commandQueue;
extern chrg_chargerState_t chrg_currentChargerState; 

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
		printf("Failed to create CMD task code %d\r\n", retCode);
	}
	drv_led_set(DRV_LED_GREEN, DRV_LED_FLASH);
	//enable power to the data board
	drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN1, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_JC_EN2, DRV_GPIO_PIN_STATE_LOW);	
	drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_PULLED_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_CHRG_SEL, DRV_GPIO_PIN_STATE_HIGH);	
	while(1)
	{
		//test code for the power board. 
		//while(1)
		//{
			//drv_led_set(DRV_LED_GREEN, DRV_LED_SOLID);
			//vTaskDelay(500);
			//drv_led_set(DRV_LED_BLUE, DRV_LED_SOLID);
			//vTaskDelay(500);
			//drv_led_set(DRV_LED_RED, DRV_LED_SOLID);
			//vTaskDelay(1000);
			//drv_uart_putString(&uart0Config, "hello from uart0\r\n");
			//drv_uart_putString(&uart1Config, "hello from uart1\r\n");
		//}

		//vTaskDelay(500); 
		if(xQueueReceive( mgr_eventQueue, &(msgEvent), 1000) == TRUE)
		{	
			switch(msgEvent.sysEvent)
			{
				case SYS_EVENT_POWER_SWITCH:
				{
					//go to power down state. 
					enterSleepMode();
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
				default:
				{
					
				}
				break;
				
			}
		}
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
		//The power button is still low, send the event. 
		if(xQueueSendToBack(mgr_eventQueue,&pwrDownEvent,5) != TRUE)
		{
			//this is an error, we should log it.
		}	
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
		xTimerStartFromISR(pwrButtonTimer,&hasWoken);
	}
	pio_enable_interrupt(PIOA, PinMask);	
}

static void enterSleepMode()
{
	cmd_commandPacket_t packet;
	uint32_t powerOnFlag = FALSE;
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
	while((xTaskGetTickCount() - startTime)> 1000)
	{		
		drv_gpio_getPinState(DRV_GPIO_PIN_GPIO,&gpioPinState);
		if(gpioPinState == DRV_GPIO_PIN_STATE_LOW)
		{
			//the data board is ready to shutdown, leave the loop. 
			break;
		}
	}
	//turn off power to the data board
	drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_LOW);

	
	//go to sleep, and wait for power button press again. 
	PreSleepProcess();
	while (powerOnFlag == FALSE)	//Stay in sleep mode until wakeup
	{
		//cpu_irq_disable();
		//pmc_enable_sleepmode(0);
		uint32_t startupInput = (1<<4); 
		pmc_set_fast_startup_input(startupInput);
		pmc_sleep(SAM_PM_SMODE_WAIT);
		//Processor wakes up from sleep
		delay_ms(WAKEUP_DELAY);
		drv_gpio_getPinState(DRV_GPIO_PIN_PWR_BTN, &pwSwState);	//poll the power switch
		//drv_gpio_getPinState(DRV_GPIO_PIN_LBO, &lboState);	//poll Low battery out
		if(pwSwState == DRV_GPIO_PIN_STATE_LOW)	//check if it is a false wakeup
		{
			//The power button has been held long enough, break the loop and power on. 
			powerOnFlag = TRUE; 
		}
		else
		{
			powerOnFlag = FALSE;
		}
	}
	PostSleepProcess();
	//set the GPIO pin to be an input. 
	drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_PULLED_LOW);
	//enable power to the data board
	drv_gpio_setPinState(DRV_GPIO_PIN_PWR_EN, DRV_GPIO_PIN_STATE_HIGH);
	//wait for brain mcu to start up
	gpioPinState = DRV_GPIO_PIN_STATE_LOW; 
	while((xTaskGetTickCount() - startTime)> 3000)
	{
		drv_gpio_getPinState(DRV_GPIO_PIN_GPIO,&gpioPinState);
		if(gpioPinState == DRV_GPIO_PIN_STATE_HIGH)
		{
			//the data board is powered up, break loop
			break;
		}
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