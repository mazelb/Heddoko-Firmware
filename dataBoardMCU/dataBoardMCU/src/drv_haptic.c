/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
/*
 * drv_haptic.c
 *
 * Created: 9/7/2016 9:48:59 AM
 *  Author: Hriday Mehta
 */ 

#include "drv_haptic.h"
#include "dbg_debugManager.h"

/*	Local variables	*/
xTimerHandle hapticTimer;							//timer handle for the haptic timer
drv_haptic_config_t *vHapticConfig;					// local pointer to point to external haptic configuration
drv_haptic_patternElement_t *vPatternElementArray;	// local variable to hold the pointer to external pattern array
uint32_t vTotalArrayElements = 0;					// variable to hold the total number of elements in the requested array
uint32_t vPlayedElementCount = 0;					// dictates the number of elements that are played from the pattern
uint32_t vPlayedOnTimeElementsCount = 0;			// indicates the number of onTime elements played from the array.

/*	Static functions declarations	*/
static void vHapticTimerCallBack();
static uint32_t getTickFromMs(uint32_t milliSeconds);	// converts milliseconds to ticks

/***********************************************************************************************
 * drv_haptic_init(drv_haptic_config_t *hapticConfig)
 * @brief initialize haptic driver. Store the config and create a timer
 * @param hapticConfig, the configuration structure for haptic motor, contains gpio mappings.  
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_haptic_init(drv_haptic_config_t *hapticConfig)
{
	if (hapticConfig != NULL)
	{
		vHapticConfig = hapticConfig;
	}
	
	// initialize the timer pattern playback
	hapticTimer = xTimerCreate("HT", getTickFromMs(DRV_HAPTIC_BUZZ_RATE), pdTRUE, NULL, vHapticTimerCallBack);	// dormant when created
	if (hapticTimer == NULL)
	{
		dbg_printString(DBG_LOG_LEVEL_ERROR, "Failed to initialize Haptic Driver\r\n");
		return STATUS_FAIL;
	}
	return STATUS_PASS;
}

/***********************************************************************************************
 * drv_haptic_playPattern(drv_haptic_patternElement_t *patternElementArray, uint32_t totalElements)
 * @brief Store the playback pattern in local structure and begin playback
 * @param drv_haptic_patternElement_t *patternElementArray: pointer to the pattern array
 *			uint32_t totalElements: Total number of elements in the array
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_haptic_playPattern(drv_haptic_patternElement_t *patternElementArray, uint32_t totalElements)
{
	portBASE_TYPE result;
	
	if (totalElements == 0)
	{
		return STATUS_FAIL;
	}
	
	// make sure the timer is dormant
	drv_haptic_stopPlayback();
	
	// store the data to local variables
	vPatternElementArray = patternElementArray;
	vTotalArrayElements = totalElements;
	vPlayedElementCount = 0;
	vPlayedOnTimeElementsCount = 0;
	
	if (vPatternElementArray[vPlayedElementCount].onTime != NULL)	// check if the value is not a null
	{
		// load the first element
		result = xTimerChangePeriod(hapticTimer, getTickFromMs(vPatternElementArray[vPlayedElementCount].onTime), NULL);
		vPlayedOnTimeElementsCount++;
	
		// set the GPIO - turn on the motor
		drv_gpio_setPinState(vHapticConfig->hapticGpio, vHapticConfig->onState);
	}
}

/***********************************************************************************************
 * drv_haptic_stopPlayback()
 * @brief Stop the playback of the haptic pattern
 * @param void
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_haptic_stopPlayback()
{
	// stop the timers and turn off motor
	if (xTimerIsTimerActive(hapticTimer) == pdTRUE)
	{
		xTimerStop(hapticTimer, NULL);
	}
	drv_gpio_setPinState(vHapticConfig->hapticGpio, vHapticConfig->offState);
	return STATUS_PASS;
}

/*	Local static function definitions	*/

/***********************************************************************************************
 * vHapticTimerCallBack()
 * @brief Timer callback function used to play the next pattern element
 * @param void
 * @return void
 ***********************************************************************************************/
static void vHapticTimerCallBack()
{
	portBASE_TYPE result;
	
	if (xTimerIsTimerActive(hapticTimer) == pdTRUE)
	{
		// as the timer is still active, stop it before reconfiguring
		xTimerStop(hapticTimer, NULL);
	}
	if (vPlayedElementCount == vTotalArrayElements)
	{
		// all elements have been played, stop the playBack
		drv_haptic_stopPlayback();
		return;
	}	
	
	// check if the next element should be onTime or offTime
	if (vPlayedElementCount < vPlayedOnTimeElementsCount)	// offTime is next
	{
		// offTime part of current element is yet to be played
		result = xTimerChangePeriod(hapticTimer, getTickFromMs(vPatternElementArray[vPlayedElementCount].offTime), NULL);
		vPlayedElementCount++;	// both the onTime and offTime have been played, increment the count
		// set the GPIO - turn off the motor
		drv_gpio_setPinState(vHapticConfig->hapticGpio, vHapticConfig->offState);
	}
	
	else if (vPlayedElementCount == vPlayedOnTimeElementsCount)	// onTime is next
	{
		// a complete element has been played, load the next one from array
		result = xTimerChangePeriod(hapticTimer, getTickFromMs(vPatternElementArray[vPlayedElementCount].onTime), NULL);
		vPlayedOnTimeElementsCount++;	// increment the count to indicate the next element is offTime
		// set the GPIO - turn on the motor
		drv_gpio_setPinState(vHapticConfig->hapticGpio, vHapticConfig->onState);
	}
}

/***********************************************************************************************
 * getTickFromMs(uint32_t milliSeconds)
 * @brief Convert milliseconds to ticks
 * @param uint32_t milliSeconds
 * @return uint31_t converted ticks
 ***********************************************************************************************/
static uint32_t getTickFromMs(uint32_t milliSeconds)
{
	if (milliSeconds == 0)
	{
		milliSeconds++;	// make sure to not pass nulls. 1ms is as good as 0ms
	}
	return (milliSeconds / portTICK_RATE_MS);
}