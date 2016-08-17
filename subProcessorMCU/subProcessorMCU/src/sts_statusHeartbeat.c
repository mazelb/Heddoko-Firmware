/*
 * sts_statusHeartbeat.c
 *
 * Created: 2/29/2016 3:55:28 PM
 *  Author: sean
 */ 

#include "sts_statusHeartbeat.h"
#include "common.h"

static bool usbCommState;	// indicates whether comm is dectected on USB

void sts_getSystemStatus(subp_status_t *sys_status)
{
	// populate the system status structure and return the pointer to data.
	drv_gpio_pin_state_t jcDc1, jcDc2;
	
	sys_status->chargeLevel = (uint8_t) chrg_getBatteryPercentage();
	sys_status->chargerState = chrg_getChargeState();
	sys_status->usbCommState = (uint8_t)(usbCommState);	// should be 1 when connected to a comm port.
	drv_gpio_getPinState(DRV_GPIO_PIN_JC1_DET, &jcDc1);	//DRV_GPIO_PIN_STATE_HIGH
	drv_gpio_getPinState(DRV_GPIO_PIN_JC2_DET, &jcDc2);	//DRV_GPIO_PIN_STATE_HIGH
	sys_status->jackDetectState = (((jcDc1 == DRV_GPIO_PIN_STATE_HIGH ? 1:0) << 1) | (jcDc2 == DRV_GPIO_PIN_STATE_HIGH ? 1:0));
	sys_status->streamState = sen_getSensorState();
	sys_status->sensorMask = sen_getDetectedSensors();
}

void user_callback_suspend_action(void)
{
	usbCommState = false;	// no comm detected on USB
}

void user_callback_resume_action(void)
{
	usbCommState = true;	// comm detected on USB
}