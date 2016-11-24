/*
 * drv_piezo.c
 *
 * Created: 8/31/2016 4:22:00 PM
 *  Author: Hriday Mehta
 */ 

#include "drv_piezo.h"

/*	Static function declarations	*/
static void playNextNote();
void TC_intHandler();
static uint32_t changeDelayToFreq(uint32_t requiredDelay);

/*	Local variables	*/
drv_piezo_config_t* p_PeizoConfig;	// pointer to the external piezo configuration
drv_piezo_noteElement_t *vNotePattern;	// local variable to hold note pattern
uint16_t vPlayedNoteCount;	// count to keep track of number of notes played from note pattern
uint16_t vTotalNoteCount;
static bool enablePiezo = true; 
drv_tc_config_t peizoTcConfig =
{
	.p_tc = DRV_TC_TC0,					// use Timer 0
	.tc_channelNumber = DRV_TC_TC_CH1,	// channel number 1 for timer 0
	.tc_mode = DRV_TC_WAVEFORM,			// Waveform output
	.tc_handler = NULL,					// interrupt handler for the timer counter
	.enable_interrupt = false,
	.interrupt_sources = DRV_TC_CPAS,	// interrupt on RA compare
	.channel_mode = (DRV_TC_AUTO_RELOAD_RC | DRV_TC_ACPC_TOGGLE), // auto reload and toggle TIOA on RC compare,
	.frequency = NULL,
	.duty_cycle = NULL
};
drv_tc_config_t noteLength =
{
	.p_tc = DRV_TC_TC0,					// use Timer 0
	.tc_channelNumber = DRV_TC_TC_CH0,	// channel number 0 for timer 0
	.tc_mode = DRV_TC_WAVEFORM,			// Waveform output
	.tc_handler = TC_intHandler,		// interrupt handler for the timer counter
	.enable_interrupt = true,
	.interrupt_sources = DRV_TC_CPAS,	// interrupt on RA compare
	.channel_mode = (DRV_TC_AUTO_RELOAD_RC), // auto reload timer on RC compare, 
	.frequency = NULL,
	.duty_cycle = NULL
};

/***********************************************************************************************
 * drv_piezo_init(drv_piezo_config_t* peizoConfig)
 * @brief initialize the timer counter for the piezo driver
 * @param drv_piezo_config_t* peizoConfig: pointer to the configuration storing the GPIO mappings 
 * @return void
 ***********************************************************************************************/
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

/***********************************************************************************************
 * drv_piezo_playTone(uint16_t noteFrequency)
 * @brief play a specific tone on the piezo
 * @param uint16_t noteFrequency: desired frequency to be played
 * @return void
 ***********************************************************************************************/
void drv_piezo_playTone(uint16_t noteFrequency)
{
	drv_tc_changeFrequency(&peizoTcConfig, noteFrequency);
}

/***********************************************************************************************
 * drv_piezo_stopPlaying()
 * @brief stop the piezo playback by stopping the timer counters
 * @param void
 * @return void
 ***********************************************************************************************/
void drv_piezo_stopPlaying()
{
	// stop the timer counters.
	drv_tc_disableInterrupt(&noteLength);
	drv_tc_disableInterrupt(&peizoTcConfig);
	drv_tc_stop(&peizoTcConfig);
	drv_tc_stop(&noteLength);
}

/***********************************************************************************************
 * drv_piezo_playPattern(drv_piezo_noteElement_t *notePattern, uint16_t totalElements)
 * @brief Load the pattern into local structure and start playback
 * @param drv_piezo_noteElement_t *notePattern: pointer to the external note pattern to be played
 *			uint16_t totalElements: total number of elements in the note
 * @return void
 ***********************************************************************************************/
void drv_piezo_playPattern(drv_piezo_noteElement_t *notePattern, uint16_t totalElements)
{
	if(enablePiezo == false)
    {
        return; 
    }
    
    if (totalElements == 0)
	{
		return;
	}
	
	// store the note pattern
	vNotePattern = notePattern;
	vTotalNoteCount = totalElements;
	vPlayedNoteCount = 0;
	
	// load the first note
	drv_piezo_playTone(vNotePattern[vPlayedNoteCount].noteFrequency);
	drv_tc_changeFrequency(&noteLength, changeDelayToFreq(vNotePattern[vPlayedNoteCount].postNoteDelay));
	vPlayedNoteCount++;
}

/***********************************************************************************************
 * drv_piezo_togglePiezo(bool piezoState)
 * @brief enable and disable the piezo
 * @param peizoState, = true, will enable the sound, false will disable it. 
 * @return void
 ***********************************************************************************************/
void drv_piezo_togglePiezo(bool piezoState)
{
    enablePiezo = piezoState; 
}

/*	Local functions	*/
/***********************************************************************************************
 * TC_intHandler()
 * @brief interrupt handler for note length timer. Loads the next tone to be played
 * @param void
 * @return void
 ***********************************************************************************************/
void TC_intHandler()
{
	if (drv_tc_isInterruptGenerated(&noteLength, DRV_TC_CPAS) == STATUS_PASS)	// RA Compare Status (cleared on read)
	{
		// load the next note
		playNextNote();
	}
}

/***********************************************************************************************
 * playNextNote()
 * @brief Play the next note from the patteen. Stop playback if all the notes have been played
 * @param void
 * @return void
 ***********************************************************************************************/
static void playNextNote()
{
	if (vPlayedNoteCount >= vTotalNoteCount)
	{
		// stop the timers if all the notes have been played
		drv_piezo_stopPlaying();
		return;
	}
	
	// load the next note
	drv_piezo_playTone(vNotePattern[vPlayedNoteCount].noteFrequency);
	drv_tc_changeFrequency(&noteLength, changeDelayToFreq(vNotePattern[vPlayedNoteCount].postNoteDelay));
	vPlayedNoteCount++;
}

/***********************************************************************************************
 * changeDelayToFreq(uint32_t requiredDelay)
 * @brief Convert delay in milliseconds to frequency
 * @param uint32_t requiredDelay: required delay in ms.
 * @return uint32_t converted frequency
 ***********************************************************************************************/
static uint32_t changeDelayToFreq(uint32_t requiredDelay)
{
	if (requiredDelay > 0)
	{
		return (1000 / requiredDelay);	// convert ms to hertz
	}
	return 0;
}