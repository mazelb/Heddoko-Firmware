/*
 * chrg_chargeMonitor.c
 *
 * Created: 2/29/2016 3:50:03 PM
 *  Author: sean
  * charger state
  CHARGE CYCLE STATE 			STAT1 		STAT2 		PG
  Shutdown (VDD = VBAT) 		Hi-Z 		Hi-Z		 Hi-Z   0x07
  Shutdown (VDD = IN) 			Hi-Z 		Hi-Z		 L		0x06
  Shutdown (CE = L) 			Hi-Z 		Hi-Z		 L      0x06 
  Preconditioning 				L			Hi-Z 		 L      0x02
  Constant Current				L 			Hi-Z 		 L		0x02
  Constant Voltage 				L 			Hi-Z 		 L		0x02
  Charge Complete - Standby		Hi-Z 		L 			 L		0x04
  Temperature Fault 			L 			L 			 L		0x00
  Timer Fault					L 			L			 L		0x00
  Low Battery Output			L 			Hi-Z 		 Hi-Z	0x03
  No Battery Present 			Hi-Z		Hi-Z		 L		0x01
  No Input Power Present 		Hi-Z 		Hi-Z 		 Hi-Z	0x00

0x00 Fault
0x01 No Battery
0x02 Charging
0x03 Low Battery
0x04 Charge Complete
0x05 Invalid code
0x06 Shutdown_VIN
0x07 Shutdown_VBAT
 */ 


#include "chrg_chargeMonitor.h"
#include "dat_dataRouter.h"
#include "drv_led.h"
#include "mgr_managerTask.h"
#include "LTC2941-1.h"

volatile chrg_chargerState_t chrg_currentChargerState = CHRG_CHARGER_STATE_INVALID_CODE; 

extern xQueueHandle mgr_eventQueue;
extern slave_twi_config_t ltc2941Config; 
//static function forward declarations
chrg_chargerState_t getChargerState(chrg_chargeMonitorConfig_t* chargerConfig);  
uint32_t powerButtonLowCount = 0;	

#define BATTERY_PERCENT_LOW 15
#define BATTERY_PERCENT_CRITICAL 8
#define BATTERY_PERCENT_FAULT 5

/***********************************************************************************************
 * chrg_chargeMonitor(void *pvParameters)
 * @brief This task will monitor the current charger status and battery charge level.  
 * @param pvParameters, void pointer to structure containing data router configuration. 
 * @return void
 ***********************************************************************************************/
void chrg_task_chargeMonitor(void *pvParameters)
{
	chrg_chargeMonitorConfig_t* chargeMonitorConfig = (chrg_chargeMonitorConfig_t*)pvParameters;  	
	chrg_chargerState_t newChargerState = CHRG_CHARGER_STATE_INVALID_CODE; 
	mgr_eventMessage_t eventMessage; 
	uint16_t chargeLevel = 0, newChargeLevel = 0;
	drv_gpio_pin_state_t usbConnectedState = DRV_GPIO_PIN_STATE_PULLED_HIGH,
		 newUsbConnectedState = DRV_GPIO_PIN_STATE_LOW; 
	drv_gpio_pin_state_t pwrButtonState = DRV_GPIO_PIN_STATE_PULLED_HIGH,
		 newPwrButtonState = DRV_GPIO_PIN_STATE_LOW; 
	
	char tempString[100] = {0}; 	 
	while(1)
	{
		newChargerState = getChargerState(chargeMonitorConfig); 	
		newChargeLevel = getCalculatedPercentage(&ltc2941Config);	
		drv_gpio_getPinState(DRV_GPIO_PIN_USB_DET, &newUsbConnectedState);
		if(newUsbConnectedState != usbConnectedState)
		{
			if(newUsbConnectedState == DRV_GPIO_PIN_STATE_HIGH)
			{
				//send GPIO connected state
				eventMessage.sysEvent = SYS_EVENT_USB_CONNECTED;
			}
			else
			{
				eventMessage.sysEvent = SYS_EVENT_USB_DISCONNECTED;
			}
			if(mgr_eventQueue != NULL)
			{
				if(xQueueSendToBack(mgr_eventQueue,( void * ) &eventMessage,5) != TRUE)
				{
					//this is an error, we should log it.
				}
			}
			usbConnectedState = newUsbConnectedState; 			
		}
		drv_gpio_getPinState(DRV_GPIO_PIN_PWR_BTN, &newPwrButtonState);
		if(newPwrButtonState != pwrButtonState)
		{
			if(newPwrButtonState == DRV_GPIO_PIN_STATE_HIGH)
			{
				powerButtonLowCount = 0;
				dat_sendDebugMsgToDataBoard("PwrBrdMsg:pwr Button high\r\n");
			}
			else
			{				
				dat_sendDebugMsgToDataBoard("PwrBrdMsg:pwr Button low\r\n");
			}
			pwrButtonState = newPwrButtonState;
		}
		if(pwrButtonState == DRV_GPIO_PIN_STATE_LOW)
		{
			powerButtonLowCount++;
		}
		if(powerButtonLowCount == 10) //approximately 3.5 seconds
		{
			
			//should be we reset the power board? or just power off
			eventMessage.sysEvent = SYS_EVENT_POWER_SWITCH;
			if(mgr_eventQueue != NULL)
			{
				if(xQueueSendToBack(mgr_eventQueue,( void * ) &eventMessage,5) != TRUE)
				{
					//this is an error, we should log it.
				}
			}				
		}
				
		//check if the state is new
		if(newChargerState != chrg_currentChargerState)
		{
			switch(newChargerState)
			{
				case CHRG_CHARGER_STATE_SHUTDOWN_VBAT:
					//the charger is shutdown, this means that we're running on battery							
					chargeLevel = 0; //set the current charge level to zero so we update the LED color below. 				
				break;
				case CHRG_CHARGER_STATE_SHUTDOWN_VIN:
					drv_led_set(DRV_LED_YELLOW,DRV_LED_SOLID); //TODO: maybe change this... since it's not actually charging. 
				break;
				case CHRG_CHARGER_STATE_CHARGING:
				{					
					drv_led_set(DRV_LED_YELLOW,DRV_LED_SOLID);	
				}				
				break;
				case CHRG_CHARGER_STATE_CHARGE_COMPLETE:
				{					
					sprintf(tempString,"PwrBrdMsg:Receive Battery Full indication at %d level\r\n",ltc2941GetCharge(&ltc2941Config));
					dat_sendDebugMsgToDataBoard(tempString);
					ltc2941SetChargeComplete(&ltc2941Config);		
								
					drv_led_set(DRV_LED_GREEN,DRV_LED_SOLID);
				}
				break;
				case CHRG_CHARGER_STATE_LOW_BATTERY:
				{
					eventMessage.sysEvent = SYS_EVENT_LOW_BATTERY; 
					if(mgr_eventQueue != NULL)
					{						
						sprintf(tempString,"PwrBrdMsg:Receive Low Battery indication at %d level\r\n",ltc2941GetCharge(&ltc2941Config));
						dat_sendDebugMsgToDataBoard(tempString);
						ltc2941SetCharge(&ltc2941Config, CHARGE_EMPTY_VALUE); //set the gas gauge to zero. 						
						if(xQueueSendToBack(mgr_eventQueue,( void * ) &eventMessage,5) != TRUE)
						{
							//this is an error, we should log it.
						}				
					}
					drv_led_set(DRV_LED_RED,DRV_LED_SOLID);
				}
				break;
				case CHRG_CHARGER_STATE_NO_BATTERY:
				{				
					drv_led_set(DRV_LED_PURPLE,DRV_LED_SOLID);
					dat_sendDebugMsgToDataBoard("PwrBrdMsg:No Battery\r\n");
				}
				break;
				case CHRG_CHARGER_STATE_FAULT:
				case CHRG_CHARGER_STATE_INVALID_CODE:
				default:
				{
					//drv_led_set(DRV_LED_TURQUOISE,DRV_LED_FLASH);	
					//turn off LED for now, figure out what to do with this state later
					dat_sendDebugMsgToDataBoard("PwrBrdMsg:Battery Fault\r\n");
					drv_led_set(DRV_LED_OFF,DRV_LED_SOLID);
				}				
				break;				
			}		
			chrg_currentChargerState = newChargerState; 		
		}
		if(newChargeLevel != chargeLevel)
		{			
			chargeLevel = newChargeLevel; 
			if(chrg_currentChargerState == CHRG_CHARGER_STATE_SHUTDOWN_VBAT)
			{
				//we are being powered by VBAT, determine what the battery level is. 
				if(chargeLevel <= BATTERY_PERCENT_FAULT)
				{
					//the battery is pretty much dead
					drv_led_set(DRV_LED_RED, DRV_LED_FLASH); 
					eventMessage.sysEvent = SYS_EVENT_LOW_BATTERY;
					if(mgr_eventQueue != NULL)
					{
						dat_sendDebugMsgToDataBoard("PwrBrdMsg:Battery Level Empty\r\n");
						//dont send the message 
						//if(xQueueSendToBack(mgr_eventQueue,( void * ) &eventMessage,5) != TRUE)
						//{
							////this is an error, we should log it.
						//}
					}					
				}
				else if(chargeLevel <= BATTERY_PERCENT_CRITICAL)
				{
					drv_led_set(DRV_LED_RED, DRV_LED_FLASH); 
				}				
				else if(chargeLevel <= BATTERY_PERCENT_LOW)
				{
					//battery is low, we need to indicate that to the user
					
					drv_led_set(DRV_LED_RED, DRV_LED_SOLID);
				}
				else if(chargeLevel > BATTERY_PERCENT_LOW)
				{
					//battery level is good. no need to indicate anything. 
					//drv_led_set(DRV_LED_BLUE,DRV_LED_SOLID);					
					drv_led_set(DRV_LED_OFF,DRV_LED_SOLID);
				}				
			}
		}			
		vTaskDelay(250); 
	}	
}


//static functions

//get charge status
chrg_chargerState_t getChargerState(chrg_chargeMonitorConfig_t* chargerConfig)
{
	chrg_chargerState_t chargerState = 0x00; 
	drv_gpio_pin_state_t tempPinState = DRV_GPIO_PIN_STATE_LOW; 
	drv_gpio_getPinState(chargerConfig->pin_pg, &tempPinState); 
	chargerState |= (tempPinState & 0x01); 
	drv_gpio_getPinState(chargerConfig->pin_stat2, &tempPinState);
	chargerState |= ((tempPinState<<1) & 0x02);
	drv_gpio_getPinState(chargerConfig->pin_stat1, &tempPinState);
	chargerState |= ((tempPinState<<2) & 0x04);		
	return chargerState; 		
}