/*
 * drv_piezo.c
 *
 * Created: 8/31/2016 4:22:00 PM
 *  Author: Hriday Mehta
 */ 

#include "drv_piezo.h"

drv_piezo_config_t* p_PeizoConfig;	// pointer to the external piezo configuration
drv_tc_config_t peizoTcConfig =
{
	.p_tc = TC0,	// use Timer 0
	.tc_channelId = ID_TC1,	// channel 1 ID of timer 0
	.tc_channelNumber = 1,	// channel number 1 for timer 0
	.tc_mode = DRV_TC_WAVEFORM,	// Waveform output
	.tc_handler = TC1_Handler,	// interrupt handler for the timer counter
	.enable_interrupt = true,
	.interrupt_sources = TC_IER_CPAS,	// interrupt on RA compare
	.channel_mode = (TC_CMR_CPCTRG | TC_CMR_ACPC_TOGGLE | TC_CMR_WAVSEL_UP), //use UPDOWN mode for duty cycle
	.frequency = 5000,
	.duty_cycle = NULL
};

void TC1_Handler()
{
	if ((tc_get_status(TC, TC_CHANNEL_WAVEFORM) & TC_SR_CPAS) == TC_SR_CPAS)	// RA Compare Status (cleared on read)
	{
		ioport_toggle_pin_level(p_PeizoConfig->gpioPin);
	}
	
	if ((tc_get_status(TC, TC_CHANNEL_WAVEFORM) & TC_SR_CPCS) == TC_SR_CPCS)	// RC Compare Status (cleared on read)
	{
		// 
	}
	
	if ((tc_get_status(TC, TC_CHANNEL_WAVEFORM) & TC_SR_LOVRS) == TC_SR_LOVRS)	// Load Overrun Status (cleared on read)
	{
		//
	}
}

void drv_piezo_init(drv_piezo_config_t* peizoConfig)
{
	p_PeizoConfig = peizoConfig;
	drv_tc_init(&peizoTcConfig);
	drv_tc_config(&peizoTcConfig);
}

void drv_piezo_playTone(uint16_t noteFrequency)
{
	drv_tc_changeFrequency(&peizoTcConfig, noteFrequency);
}

void drv_piezo_playPattern(drv_piezo_notePattern_t notePattern)
{
	if (notePattern.totalArrayElements == 0)
	{
		return;
	}
	
	// play the pattern
	for (int i = 0; i < notePattern.totalArrayElements; i++)
	{
		drv_piezo_playTone(notePattern.p_noteElementArray[i].noteFrequency);
		vTaskDelay(notePattern.p_noteElementArray[i].postNoteDelay);
	}
}