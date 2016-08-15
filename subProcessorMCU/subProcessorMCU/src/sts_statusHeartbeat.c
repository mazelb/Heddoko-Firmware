/*
 * sts_statusHeartbeat.c
 *
 * Created: 2/29/2016 3:55:28 PM
 *  Author: sean
 */ 

#include "sts_statusHeartbeat.h"
#include "common.h"

void sts_getSystemStatus(subp_status_t *sys_status)
{
	// populate the system status structure and return the pointer to data.
	drv_gpio_pin_state_t jcDc1, jcDc2;
	
	sys_status->chargeLevel = (uint8_t) chrg_getBatteryPercentage();
	sys_status->chargerState = chrg_getChargeState();
	sys_status->usbCommState = (uint8_t)0;	// should be 1 when connected, TODO: does not work
	drv_gpio_getPinState(DRV_GPIO_PIN_JC1_DET, &jcDc1);	//DRV_GPIO_PIN_STATE_HIGH
	drv_gpio_getPinState(DRV_GPIO_PIN_JC2_DET, &jcDc2);	//DRV_GPIO_PIN_STATE_HIGH
	sys_status->jackDetectState = (((jcDc1 == DRV_GPIO_PIN_STATE_HIGH ? 1:0) << 1) | (jcDc2 == DRV_GPIO_PIN_STATE_HIGH ? 1:0));	// TODO: needs verification
	sys_status->streamState = sen_getSensorState();
	sys_status->sensorMask = sen_getDetectedSensors();
}