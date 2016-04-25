/*
 * chrg_chargeMonitor.h
 *
 * Created: 2/29/2016 3:50:29 PM
 *  Author: sean
 */ 


#ifndef CHRG_CHARGEMONITOR_H_
#define CHRG_CHARGEMONITOR_H_
#include "drv_gpio.h"

typedef struct
{
	drv_gpio_pins_t pin_stat1;
	drv_gpio_pins_t pin_stat2;
	drv_gpio_pins_t pin_pg;		
	
}chrg_chargeMonitorConfig_t;

/*
pg = bit 0
stat1 = bit 1
stat2 = bit 2
0x00 Fault
0x01 No Battery
0x02 Charging
0x03 Low Battery
0x04 Charge Complete
0x05 Invalid code
0x06 Shutdown_VIN
0x07 Shutdown_VBAT
*/

typedef enum
{
	CHRG_CHARGER_STATE_FAULT = 0x00,
	CHRG_CHARGER_STATE_NO_BATTERY = 0x01,
	CHRG_CHARGER_STATE_CHARGING = 0x02,
	CHRG_CHARGER_STATE_LOW_BATTERY = 0x03,
	CHRG_CHARGER_STATE_CHARGE_COMPLETE = 0x04,
	CHRG_CHARGER_STATE_INVALID_CODE = 0x05,
	CHRG_CHARGER_STATE_SHUTDOWN_VIN = 0x06,
	CHRG_CHARGER_STATE_SHUTDOWN_VBAT = 0x07
}chrg_chargerState_t;

void chrg_task_chargeMonitor(void *pvParameters);

#endif /* CHRG_CHARGEMONITOR_H_ */