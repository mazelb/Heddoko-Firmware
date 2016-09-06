/*
 * drv_tc.h
 *
 * Created: 2016-06-15 2:22:12 PM
 *  Author: Hriday Mehta
 */ 


#ifndef DRV_TC_H_
#define DRV_TC_H_

#include <asf.h>
#include "common.h"

//#define ENABLE_TC_DEBUG_PRINTS
#define TC_ALL_INTERRUPT_MASK	0xff
#define DRV_TC_TC0				TC0		// pointer to the timer counter 0
#ifdef TC1
#define DRV_TC_TC1				TC1		// pointer to the timer counter 1
#endif
/*	TIO GPIO config	*/
#define PIN_TC0_TIOA0		(PIO_PA0_IDX)
#define PIN_TC0_TIOA0_MUX   (IOPORT_MODE_MUX_B)
#define PIN_TC0_TIOA0_FLAGS (PIO_PERIPH_B | PIO_DEFAULT)

#define PIN_TC0_TIOA1		(PIO_PA15_IDX)
#define PIN_TC0_TIOA1_MUX   (IOPORT_MODE_MUX_B)
#define PIN_TC0_TIOA1_FLAGS (PIO_PERIPH_B | PIO_DEFAULT)

#define PIN_TC0_TIOA1_PIO     PIOA
#define PIN_TC0_TIOA1_MASK  PIO_PA15
#define PIN_TC0_TIOA1_ID      ID_PIOA
#define PIN_TC0_TIOA1_TYPE   PIO_PERIPH_B
#define PIN_TC0_TIOA1_ATTR   PIO_DEFAULT

#define PIN_TC0_TIOA2		(PIO_PA26_IDX)
#define PIN_TC0_TIOA2_MUX   (IOPORT_MODE_MUX_B)
#define PIN_TC0_TIOA2_FLAGS (PIO_PERIPH_B | PIO_DEFAULT)

#define PIN_TC0_TIOA2_PIO     PIOA
#define PIN_TC0_TIOA2_MASK  PIO_PA26
#define PIN_TC0_TIOA2_ID      ID_PIOA
#define PIN_TC0_TIOA2_TYPE   PIO_INPUT
#define PIN_TC0_TIOA2_ATTR   PIO_DEFAULT

#define PIN_TC0_TIOB0			PIO_PA1_IDX
#define PIN_TC0_TIOB0_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)
#define PIN_TC0_TIOB1			PIO_PA16_IDX
#define PIN_TC0_TIOB1_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)
#define PIN_TC0_TIOB2			PIO_PA27_IDX
#define PIN_TC0_TIOB2_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)

#define PIN_TC1_TIOA3			PIO_PC23_IDX
#define PIN_TC1_TIOA3_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)
#define PIN_TC1_TIOA4			PIO_PC26_IDX
#define PIN_TC1_TIOA4_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)
#define PIN_TC1_TIOA5			PIO_PC29_IDX
#define PIN_TC1_TIOA5_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)

#define PIN_TC1_TIOB3			PIO_PC24_IDX
#define PIN_TC1_TIOB3_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)
#define PIN_TC1_TIOB4			PIO_PC27_IDX
#define PIN_TC1_TIOB4_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)
#define PIN_TC1_TIOB5			PIO_PC30_IDX
#define PIN_TC1_TIOB5_FLAGS		(PIO_PERIPH_B | PIO_DEFAULT)

/*	Interrupt sources	*/
#define DRV_TC_COVFS TC_IER_COVFS // Counter Overflow
#define DRV_TC_LOVRS TC_IER_LOVRS // Load Overrun
#define DRV_TC_CPAS  TC_IER_CPAS // RA Compare
#define DRV_TC_CPBS  TC_IER_CPBS // RB Compare
#define DRV_TC_CPCS  TC_IER_CPCS // RC Compare
#define DRV_TC_LDRAS TC_IER_LDRAS // RA Loading
#define DRV_TC_LDRBS TC_IER_LDRBS // RB Loading
#define DRV_TC_ETRGS TC_IER_ETRGS // External Trigger

typedef void (* voidFunction_t)(void);

typedef enum
{
	DRV_TC_CAPTURE = 0,
	DRV_TC_WAVEFORM
}drv_tc_mode_t;

typedef enum
{
	DRV_TC_TC_CH0 = 0x00,
	DRV_TC_TC_CH1 = 0x01,
	DRV_TC_TC_CH2 = 0x02
}drv_tc_channel_t;

typedef struct  
{
	voidFunction_t tc_handler;			// pointer to the interrupt handler
	Tc *p_tc;							// pointer to the Timer/Counter instance
	drv_tc_mode_t tc_mode;				// capture/waveform mode
	drv_tc_channel_t tc_channelNumber;	// channel number relative to the corresponding instance
	uint32_t channel_mode;				// the channel config parameters (use UPDOWN for duty cycle), use TC_CMR_... definitions for configuration
	uint32_t frequency;					// frequency of operation
	uint8_t duty_cycle;					// duty cycle for the wave
	bool enable_interrupt;				// interrupt enabled/disabled
	uint32_t interrupt_sources;			// sources of interrupt.
}drv_tc_config_t;

Status_t drv_tc_init	(drv_tc_config_t *tc_config);
Status_t drv_tc_config	(drv_tc_config_t *tc_config);
Status_t drv_tc_start	(drv_tc_config_t *tc_config);
Status_t drv_tc_stop	(drv_tc_config_t *tc_config);
Status_t drv_tc_changeFrequency		(drv_tc_config_t *tc_config, uint16_t frequency);
Status_t drv_tc_enableInterrupt		(drv_tc_config_t *tc_config);
Status_t drv_tc_disableInterrupt	(drv_tc_config_t *tc_config);
Status_t drv_tc_isInterruptGenerated (drv_tc_config_t *tc_config, uint32_t interruptSource);

#endif /* DRV_TC_H_ */