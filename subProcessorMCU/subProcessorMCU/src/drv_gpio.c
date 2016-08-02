/*
 * drv_gpio.c
 *
 * Created: 9/25/2015 9:55:00 AM
 *  Author: Sean Cloghesy
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 
#include "drv_gpio.h"

/*	Interrupt Handlers Declarations	*/
static void drv_gpio_int_sw0(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_pw(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_sw1(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_sw2(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_oc1(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_oc2(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_dc1(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_dc2(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_en1(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_en2(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_lbo(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_stat(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_cd(uint32_t ul_id, uint32_t ul_mask);
static void drv_gpio_int_usb(uint32_t ul_id, uint32_t ul_mask);

drv_gpio_config_t gpioConfig[] = 
{
	/*  PIN ID					PIN MODE				  INITIAL PIN STATE       INTERRUPT MODE				INTERRUPT HANDLER  PULL UP EN  DEBOUNCE EN	setFlag     current State*/
	{DRV_GPIO_ID_PIN_JC_EN2,	  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_HIGH, DRV_GPIO_INTERRUPT_NONE,		NULL,			TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_HIGH},
	{DRV_GPIO_ID_PIN_JC_EN1,	  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_HIGH, DRV_GPIO_INTERRUPT_NONE,		NULL,			TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_HIGH},
	{DRV_GPIO_ID_PIN_JC1_DET,	  DRV_GPIO_PIN_MODE_INPUT,   DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_LOW_EDGE,  drv_gpio_int_dc1,TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_LOW},
	{DRV_GPIO_ID_PIN_JC2_DET,	  DRV_GPIO_PIN_MODE_INPUT,   DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_LOW_EDGE,  drv_gpio_int_dc2,TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_LOW},
	{DRV_GPIO_ID_PIN_PWR_EN,	  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_NONE,		NULL,			FALSE,	TRUE,			0,	DRV_GPIO_PIN_STATE_LOW},
	{DRV_GPIO_ID_PIN_CHRG_SEL,	  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_NONE,		NULL,			FALSE,	TRUE,			0,	DRV_GPIO_PIN_STATE_LOW},
	{DRV_GPIO_ID_PIN_CHRG_PG,	  DRV_GPIO_PIN_MODE_INPUT,   DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_NONE,		NULL,			TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_LOW},
	{DRV_GPIO_ID_PIN_CHRG_STAT2	, DRV_GPIO_PIN_MODE_INPUT,   DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_NONE	 ,	NULL,			TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_LOW},
	{DRV_GPIO_ID_PIN_CHRG_STAT1	, DRV_GPIO_PIN_MODE_INPUT,   DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_NONE    ,	NULL,			TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_LOW},
	{DRV_GPIO_ID_PIN_GPIO,		  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_LOW, DRV_GPIO_INTERRUPT_NONE	 ,	NULL,			TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_HIGH},
	{DRV_GPIO_ID_PIN_PWR_BTN,	  DRV_GPIO_PIN_MODE_INPUT,   DRV_GPIO_PIN_STATE_HIGH, DRV_GPIO_INTERRUPT_LOW_EDGE,	drv_gpio_int_pw,TRUE,	TRUE,			0,	DRV_GPIO_PIN_STATE_HIGH},
	{DRV_GPIO_ID_PIN_USB_DET,	  DRV_GPIO_PIN_MODE_INPUT,   DRV_GPIO_PIN_STATE_HIGH, DRV_GPIO_INTERRUPT_HIGH_EDGE,	drv_gpio_int_usb,FALSE,	TRUE,			0,	DRV_GPIO_PIN_STATE_HIGH},
	{DRV_GPIO_ID_PIN_LED_BLUE,	  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_HIGH, DRV_GPIO_INTERRUPT_NONE    ,	NULL,			FALSE,	FALSE,			0,	DRV_GPIO_PIN_STATE_HIGH},
	{DRV_GPIO_ID_PIN_LED_GREEN,	  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_HIGH, DRV_GPIO_INTERRUPT_NONE    ,	NULL,			FALSE,	FALSE,			0,	DRV_GPIO_PIN_STATE_HIGH},
	{DRV_GPIO_ID_PIN_LED_RED,	  DRV_GPIO_PIN_MODE_OUTPUT,  DRV_GPIO_PIN_STATE_HIGH, DRV_GPIO_INTERRUPT_NONE    ,	NULL,			FALSE,	FALSE,			0,	DRV_GPIO_PIN_STATE_HIGH}
	
};



/***********************************************************************************************
 * drv_gpio_initializeAll(void)
 * @brief initialize all GPIO
 * @param 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/	
status_t drv_gpio_initializeAll(void)
{
	status_t status = STATUS_PASS; 
	int numGpio = sizeof(gpioConfig)/sizeof(drv_gpio_config_t); 
	int i = 0; 
	for(i = 0; i < numGpio ; i++)
	{
		if(drv_gpio_config(&gpioConfig[i]) != STATUS_PASS)
		{
			status |= STATUS_FAIL;
		}
	}	
	return status; 
}


/***********************************************************************************************
 * drv_gpio_config(drv_gpio_config_t* gpioConfig)
 * @brief Set and configure all GPIOs
 * @param drv_gpio_config_t* gpioConfig
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_config(drv_gpio_config_t* gpioConfig)
{
	status_t status = STATUS_PASS;
	unsigned long PinFlag = 0;	
	if(gpioConfig->pinMode == DRV_GPIO_PIN_MODE_INPUT)	//Check if input or output
	{
		PinFlag |= PIO_TYPE_PIO_INPUT;	//Set as input
		if (gpioConfig->pullUpEnabled == TRUE)
		{
			PinFlag |= PIO_PULLUP;	//enable the pull up
		}
		
		if (gpioConfig->debounceEnabled == TRUE)
		{
			//PinFlag |= PIO_DEBOUNCE;	//enable debounce filter
			Pio *p_pio = pio_get_pin_group(gpioConfig->pinId);
			uint32_t PinMask = pio_get_pin_group_mask(gpioConfig->pinId);	//PinMask
			pio_set_debounce_filter(p_pio, PinMask, DEBOUNCE_PERIOD);
		}
		
		pmc_enable_periph_clk(pio_get_pin_group_id(gpioConfig->pinId));	//pio_get_pin_group_id(gpioConfig->pinId
		pio_configure_pin(gpioConfig->pinId, PinFlag);
		 
		PinFlag = 0;	//Reset the PinFlag to use it to configure interrupt
		switch(gpioConfig->interruptType)	//set interrupt configuration flag
		{
			case DRV_GPIO_INTERRUPT_HIGH_EDGE :
				PinFlag |= PIO_IT_RISE_EDGE | PIO_IT_AIME;
				break;
			case DRV_GPIO_INTERRUPT_LOW_EDGE :
				PinFlag |= PIO_IT_FALL_EDGE | PIO_IT_AIME;
				break;
			case DRV_GPIO_INTERRUPT_HIGH_LVL :
				PinFlag |= PIO_IT_HIGH_LEVEL | PIO_IT_AIME;
				break;
			case DRV_GPIO_INTERRUPT_LOW_LVL :
				PinFlag |= PIO_IT_LOW_LEVEL | PIO_IT_AIME;
				break;
			case DRV_GPIO_INTERRUPT_NONE :
			default:
				break;
		}
		Pio *p_pio = pio_get_pin_group(gpioConfig->pinId);	//peripheral ID
		uint32_t PinMask = pio_get_pin_group_mask(gpioConfig->pinId);	//PinMask
		if (PinFlag & (PIO_IT_RISE_EDGE | PIO_IT_FALL_EDGE | PIO_IT_HIGH_LEVEL | PIO_IT_LOW_LEVEL))	//if set as interrupt, enable and configure it
		{			
			uint32_t PinPio = pio_get_pin_group_id(gpioConfig->pinId);	//Pin ID
			pio_handler_set(p_pio, PinPio, PinMask, PinFlag, gpioConfig->interruptHandler);
			pio_enable_interrupt(p_pio, PinMask);
			if (p_pio == PIOA)
			{
				NVIC_EnableIRQ(PIOA_IRQn);
			}
			else if (p_pio == PIOB)
			{
				NVIC_EnableIRQ(PIOB_IRQn);
			}
		}
		//turn off any pulldown resistors
		p_pio->PIO_PPDDR |= PinMask; 
		
	}
	else	//The pin is output
	{
		if (gpioConfig->initialPinState == DRV_GPIO_PIN_STATE_HIGH)
		{
			PinFlag |= PIO_TYPE_PIO_OUTPUT_1;	//set default pin state
		} 
		else
		{
			PinFlag |= PIO_TYPE_PIO_OUTPUT_0;	//set default pin state
		}
		
		pmc_enable_periph_clk(pio_get_pin_group_id(gpioConfig->pinId)); //pio_get_pin_group_id(
		pio_configure_pin(gpioConfig->pinId, PinFlag);
	}
	
	return status;
}

/***********************************************************************************************
 * drv_gpio_setPinState(drv_gpio_pins_t pin, drv_gpio_pin_state_t state)
 * @brief Set a GPIO to HIGH or LOW
 * @param drv_gpio_pins_t pin, drv_gpio_pin_state_t state
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_setPinState(drv_gpio_pins_t pin, drv_gpio_pin_state_t state)
{
	status_t status = STATUS_PASS;
	uint32_t pinFlag = 0x00; 
	if(gpioConfig[pin].pinMode != DRV_GPIO_PIN_MODE_OUTPUT)
	{
		return STATUS_FAIL;
	}
	if(gpioConfig[pin].currentPinState == DRV_GPIO_PIN_STATE_PULLED_HIGH || gpioConfig[pin].currentPinState == DRV_GPIO_PIN_STATE_PULLED_LOW)
	{
		if(state == DRV_GPIO_PIN_STATE_HIGH )
		{
			//we have to set the pin back to an output
			pinFlag |= PIO_TYPE_PIO_OUTPUT_1;
			pio_configure_pin(gpioConfig->pinId, pinFlag);
		}
		else if(state == DRV_GPIO_PIN_STATE_LOW)
		{
			pinFlag |= PIO_TYPE_PIO_OUTPUT_0;
			pio_configure_pin(gpioConfig->pinId, pinFlag);			
		}
	}
	//the output should be in the right state. 
	
	switch(state)
	{
		case DRV_GPIO_PIN_STATE_HIGH:
			pio_set_pin_high(gpioConfig[pin].pinId);
		break;
		case DRV_GPIO_PIN_STATE_LOW:
			pio_set_pin_low(gpioConfig[pin].pinId);
		break;
		case DRV_GPIO_PIN_STATE_PULLED_HIGH:
			pinFlag |= PIO_TYPE_PIO_INPUT | PIO_PULLUP;	//Set as input with pull up. 
			pio_configure_pin(gpioConfig->pinId, pinFlag);
		break;
		case DRV_GPIO_PIN_STATE_PULLED_LOW:
		break; 
	}
	return status;
}

/***********************************************************************************************
 * drv_gpio_getPinState(drv_gpio_pins_t pin, drv_gpio_pin_state_t* state)
 * @brief Get a GPIO level
 * @param drv_gpio_pins_t pin, drv_gpio_pin_state_t state
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_getPinState(drv_gpio_pins_t pin, drv_gpio_pin_state_t* state)
{
	status_t status = STATUS_PASS;
	bool value = ioport_get_pin_level(gpioConfig[pin].pinId);
	if (value == false)
	{
		*state = DRV_GPIO_PIN_STATE_LOW;
	}
	else
	{
		*state = DRV_GPIO_PIN_STATE_HIGH;
	}
	return status;
}

/***********************************************************************************************
 * drv_gpio_togglePin(drv_gpio_pins_t pin)
 * @brief Toggle a GPIO level
 * @param drv_gpio_pins_t pin
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_togglePin(drv_gpio_pins_t pin)
{
	status_t status = STATUS_PASS;
	pio_toggle_pin(gpioConfig[pin].pinId);
	return status;
}

/***********************************************************************************************
 * drv_gpio_config_interrupt(drv_gpio_pins_t pinId, unsigned long pinFlag)
 * @brief Set Interrupt Configuration for the requested pin
 * @param drv_gpio_pins_t pinId, unsigned long pinFlag
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_config_interrupt(drv_gpio_pins_t pin, drv_gpio_interrupt_t pinInt)
{
	status_t status = STATUS_FAIL;
	unsigned long PinFlag = 0;
	Pio *p_pio = pio_get_pin_group(gpioConfig[pin].pinId);	//peripheral ID
	uint32_t PinMask = pio_get_pin_group_mask(gpioConfig[pin].pinId);	//PinMask
	if (pinInt == DRV_GPIO_INTERRUPT_HIGH_EDGE)
	{
		PinFlag = PIO_IT_RISE_EDGE | PIO_IT_AIME;
		status = STATUS_PASS;
	} 
	else if(pinInt == DRV_GPIO_INTERRUPT_LOW_EDGE)
	{
		PinFlag = PIO_IT_FALL_EDGE | PIO_IT_AIME;
		status = STATUS_PASS;
	}
	else if (pinInt == DRV_GPIO_INTERRUPT_HIGH_LVL)
	{
		PinFlag = PIO_IT_HIGH_LEVEL | PIO_IT_AIME;
		status = STATUS_PASS;
	}
	else if (pinInt == DRV_GPIO_INTERRUPT_LOW_LVL)
	{
		PinFlag = PIO_IT_LOW_LEVEL | PIO_IT_AIME;
		status = STATUS_PASS;
	}
	pio_configure_interrupt(p_pio, PinMask, PinFlag);
	return status;
}

/***********************************************************************************************
 * drv_gpio_config_interrupt_handler(drv_gpio_pins_t pinId, unsigned long pinFlag, void* handler)
 * @brief Set Interrupt Configuration for the requested pin, and also what handler to use
 * @param drv_gpio_pins_t pinId, unsigned long pinFlag, void* handler 
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_config_interrupt_handler(drv_gpio_pins_t pin, drv_gpio_interrupt_t pinInt, void* handler)
{
	status_t status = STATUS_FAIL;
	unsigned long PinFlag = 0;
	Pio *p_pio = pio_get_pin_group(gpioConfig[pin].pinId);	//peripheral ID
	uint32_t PinMask = pio_get_pin_group_mask(gpioConfig[pin].pinId);	//PinMask
	uint32_t PinPio = pio_get_pin_group_id(gpioConfig[pin].pinId);	//Pin ID
	pio_disable_interrupt(PinPio, PinMask);
	if (pinInt == DRV_GPIO_INTERRUPT_HIGH_EDGE)
	{
		PinFlag = PIO_IT_RISE_EDGE | PIO_IT_AIME;
		status = STATUS_PASS;
	} 
	else if(pinInt == DRV_GPIO_INTERRUPT_LOW_EDGE)
	{
		PinFlag = PIO_IT_FALL_EDGE | PIO_IT_AIME;
		status = STATUS_PASS;
	}
	else if (pinInt == DRV_GPIO_INTERRUPT_HIGH_LVL)
	{
		PinFlag = PIO_IT_HIGH_LEVEL | PIO_IT_AIME;
		status = STATUS_PASS;
	}
	else if (pinInt == DRV_GPIO_INTERRUPT_LOW_LVL)
	{
		PinFlag = PIO_IT_LOW_LEVEL | PIO_IT_AIME;
		status = STATUS_PASS;
	}
	pio_handler_set(p_pio, PinPio, PinMask, PinFlag, handler);
	pio_configure_interrupt(p_pio, PinMask, PinFlag);
	pio_enable_interrupt(PinPio, PinMask);
	return status;
}

/***********************************************************************************************
 * drv_gpio_check_Int(drv_gpio_pins_t pin)
 * @brief Check if Interrupt was generated on a Pin or GPIO input
 * @param drv_gpio_pins_t pin
 * @return gpioSetFlag
 ***********************************************************************************************/
bool drv_gpio_check_Int(drv_gpio_pins_t pin)
{
	bool returnVal;
	returnVal = gpioConfig[pin].gpioSetFlag;
	gpioConfig[pin].gpioSetFlag = 0;
	return	returnVal;
}

/***********************************************************************************************
 * drv_gpio_clear_Int(drv_gpio_pins_t pin)
 * @brief Clear the interrupt on a particular pin
 * @param drv_gpio_pins_t pin
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
bool drv_gpio_clear_Int(drv_gpio_pins_t pin)
{
	status_t status = STATUS_PASS;
	gpioConfig[pin].gpioSetFlag = 0;
	return status;
}

/***********************************************************************************************
 * drv_gpio_enable_interrupt(drv_gpio_pins_t pin)
 * @brief Enable the interrupt on a particular pin
 * @param drv_gpio_pins_t pin
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_enable_interrupt(drv_gpio_pins_t pin)
{
	status_t status = STATUS_PASS;
	uint32_t PinMask = pio_get_pin_group_mask(gpioConfig[pin].pinId);
	Pio *p_pio = pio_get_pin_group(gpioConfig[pin].pinId);
	pio_enable_interrupt(p_pio, PinMask);
	if (p_pio == PIOA)
	{
		NVIC_EnableIRQ(PIOA_IRQn);
	}
	else if (p_pio == PIOB)
	{
		NVIC_EnableIRQ(PIOB_IRQn);
	}
	return status;
}

/***********************************************************************************************
 * drv_gpio_save_interrupt_mask_all(void)
 * @brief Save the interrupt mask on all ports
 * @param void
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_save_interrupt_mask_all(void)
{
	status_t status = STATUS_PASS;
	//Save interrupt configuration
	uint32_t PioIntMaskA = pio_get_interrupt_mask(PIOA);
	uint32_t PioIntMaskB = pio_get_interrupt_mask(PIOB);
	return status;
}

/***********************************************************************************************
 * drv_gpio_disable_interrupt_all(void)
 * @brief Disable interrupts on every gpio pin of every port
 * @param drv_gpio_pins_t pin
 * @return STATUS_PASS if successful, STATUS_FAIL if there is an error
 ***********************************************************************************************/
status_t drv_gpio_disable_interrupt_all(void)
{
	status_t status = STATUS_PASS;
	pio_disable_interrupt(PIOA, ALL_INTERRUPT_MASK);
	NVIC_DisableIRQ(PIOA_IRQn);
	pio_disable_interrupt(PIOB, ALL_INTERRUPT_MASK);
	NVIC_DisableIRQ(PIOB_IRQn);
	return status;
}

///*	Interrupt Handlers definitions for GPIOs	*/


/***********************************************************************************************
 * drv_gpio_int_pw(uint32_t ul_id, uint32_t ul_mask)
 * @brief Interrupt routine for Power Switch
 * @param uint32_t ul_id, uint32_t ul_mask
 * @return 
 ***********************************************************************************************/
static void drv_gpio_int_pw(uint32_t ul_id, uint32_t ul_mask)
{
	uint32_t PinMask = pio_get_pin_group_mask(gpioConfig[DRV_GPIO_PIN_PWR_BTN].pinId);
	pio_disable_interrupt(PIOA, PinMask);
	uint32_t ReadIsr = PIOA->PIO_ISR;
	if (PinMask == ul_mask)
	{
		gpioConfig[DRV_GPIO_PIN_PWR_BTN].gpioSetFlag = 1;
	}
	pio_enable_interrupt(PIOA, PinMask);
}


/***********************************************************************************************
 * drv_gpio_int_dc1(uint32_t ul_id, uint32_t ul_mask)
 * @brief Interrupt routine for Jack-1 Detect
 * @param uint32_t ul_id, uint32_t ul_mask
 * @return 
 ***********************************************************************************************/
static void drv_gpio_int_dc1(uint32_t ul_id, uint32_t ul_mask)
{
	uint32_t PinMask = pio_get_pin_group_mask(gpioConfig[DRV_GPIO_PIN_JC1_DET].pinId);
	pio_disable_interrupt(PIOA, PinMask);
	uint32_t ReadIsr = PIOA->PIO_ISR;
	if (PinMask == ul_mask)
	{
		gpioConfig[DRV_GPIO_PIN_JC1_DET].gpioSetFlag = 1;
	}
	pio_enable_interrupt(PIOA, PinMask);
}

/***********************************************************************************************
 * drv_gpio_int_dc2(uint32_t ul_id, uint32_t ul_mask)
 * @brief Interrupt routine for Jack-2 Detect
 * @param uint32_t ul_id, uint32_t ul_mask
 * @return 
 ***********************************************************************************************/
static void drv_gpio_int_dc2(uint32_t ul_id, uint32_t ul_mask)
{
	uint32_t PinMask = pio_get_pin_group_mask(gpioConfig[DRV_GPIO_PIN_JC2_DET].pinId);
	pio_disable_interrupt(PIOA, PinMask);
	uint32_t ReadIsr = PIOA->PIO_ISR;
	if (PinMask == ul_mask)
	{
		gpioConfig[DRV_GPIO_PIN_JC2_DET].gpioSetFlag = 1;
	}
	pio_enable_interrupt(PIOA, PinMask);
}


/***********************************************************************************************
 * drv_gpio_int_usb(uint32_t ul_id, uint32_t ul_mask)
 * @brief Interrupt routine for Jack-2 Detect
 * @param uint32_t ul_id, uint32_t ul_mask
 * @return 
 ***********************************************************************************************/
static void drv_gpio_int_usb(uint32_t ul_id, uint32_t ul_mask)
{
	uint32_t PinMask = pio_get_pin_group_mask(gpioConfig[DRV_GPIO_PIN_USB_DET].pinId);
	pio_disable_interrupt(PIOA, PinMask);
	uint32_t ReadIsr = PIOA->PIO_ISR;
	if (PinMask == ul_mask)
	{
		gpioConfig[DRV_GPIO_PIN_USB_DET].gpioSetFlag = 1;
	}
	pio_enable_interrupt(PIOA, PinMask);
}