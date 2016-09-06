/*
 * drv_piezo.c
 *
 * Created: 8/31/2016 4:22:00 PM
 *  Author: Hriday Mehta
 */ 

#include "drv_piezo.h"

/*	Local variables	*/
drv_piezo_config_t* p_PeizoConfig;	// pointer to the external piezo configuration
drv_piezo_notePattern_t vNotePattern;	// local variable to hold note pattern
uint16_t vPlayedNoteCount;	// count to keep track of number of notes played from note pattern
uint16_t vTotalNoteCount;

/*	Static function declarations	*/
static void playNextNote();
void TC_intHandler();
static uint32_t changeDelayToFreq(uint32_t requiredDelay);

drv_tc_config_t peizoTcConfig =
{
	.p_tc = DRV_TC_TC0,					// use Timer 0
	.tc_channelNumber = DRV_TC_TC0_CH1,	// channel number 1 for timer 0
	.tc_mode = DRV_TC_WAVEFORM,			// Waveform output
	.tc_handler = NULL,					// interrupt handler for the timer counter
	.enable_interrupt = false,
	.interrupt_sources = DRV_TC_CPAS,	// interrupt on RA compare
	.channel_mode = (TC_CMR_CPCTRG | TC_CMR_ACPC_TOGGLE | TC_CMR_WAVSEL_UP), //use UPDOWN mode for duty cycle
	.frequency = NULL,
	.duty_cycle = NULL
};
drv_tc_config_t noteLength =
{
	.p_tc = DRV_TC_TC0,					// use Timer 0
	.tc_channelNumber = DRV_TC_TC0_CH0,	// channel number 1 for timer 0
	.tc_mode = DRV_TC_WAVEFORM,			// Waveform output
	.tc_handler = TC_intHandler,		// interrupt handler for the timer counter
	.enable_interrupt = true,
	.interrupt_sources = DRV_TC_CPAS,	// interrupt on RA compare
	.channel_mode = (TC_CMR_CPCTRG | TC_CMR_WAVSEL_UP), //use UPDOWN mode for duty cycle
	.frequency = NULL,
	.duty_cycle = NULL
};

void drv_piezo_init(drv_piezo_config_t* peizoConfig)
{
	p_PeizoConfig = peizoConfig;
	drv_tc_init(&peizoTcConfig);
	drv_tc_config(&peizoTcConfig);
	drv_tc_stop(&peizoTcConfig);
	
	// initialize the note length timer
	drv_tc_init(&noteLength);
	drv_tc_config(&noteLength);
	drv_tc_stop(&noteLength);
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
	
	// store the note pattern
	vNotePattern = notePattern;
	vTotalNoteCount = notePattern.totalArrayElements;
	vPlayedNoteCount = 0;
	
	// load the first note
	drv_piezo_playTone(notePattern.p_noteElementArray[vPlayedNoteCount].noteFrequency);
	drv_tc_changeFrequency(&noteLength, changeDelayToFreq(vNotePattern.p_noteElementArray[vPlayedNoteCount].postNoteDelay));
	vPlayedNoteCount++;
}

/*	Local functions	*/
void TC_intHandler()
{
	if (drv_tc_isInterruptGenerated(&noteLength, DRV_TC_CPAS) == STATUS_PASS)	// RA Compare Status (cleared on read)
	{
		// load the next note
		playNextNote();
	}
}

static void playNextNote()
{
	if (vPlayedNoteCount >= vTotalNoteCount)
	{
		// stop the timers if all the notes have been played
		drv_tc_disableInterrupt(&noteLength);
		drv_tc_disableInterrupt(&peizoTcConfig);
		drv_tc_stop(&peizoTcConfig);
		drv_tc_stop(&noteLength);
		return;
	}
	
	// load the next note
	drv_piezo_playTone(vNotePattern.p_noteElementArray[vPlayedNoteCount].noteFrequency);
	drv_tc_changeFrequency(&noteLength, changeDelayToFreq(vNotePattern.p_noteElementArray[vPlayedNoteCount].postNoteDelay));
	vPlayedNoteCount++;
}

static uint32_t changeDelayToFreq(uint32_t requiredDelay)
{
	if (requiredDelay > 0)
	{
		return (1000 / requiredDelay);	// convert ms to hertz
	}
	return 0;
}