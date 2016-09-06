/*
 * drv_tc.c
 *
 * Created: 2016-06-15 2:22:23 PM
 *  Author: Hriday Mehta
 */ 

#include "drv_tc.h"

voidFunction_t vCallbackTable[6] = {NULL};

void configureTio(drv_tc_config_t* tc_config)
{
	uint32_t channelB_present = 0, channelA_present = 0;
	uint32_t checkGpioRequired = (TC_CMR_ACPA_Msk | TC_CMR_ACPC_Msk | TC_CMR_AEEVT_Msk | TC_CMR_ASWTRG_Msk |
									TC_CMR_BCPB_Msk | TC_CMR_BCPC_Msk | TC_CMR_BEEVT_Msk | TC_CMR_BSWTRG_Msk);
	
	if ((tc_config->channel_mode & checkGpioRequired) == NULL)
	{
		//channel not configured to use GPIO.
		return;
	}
	
	channelA_present = tc_config->channel_mode & (TC_CMR_ACPA_Msk | TC_CMR_ACPC_Msk | TC_CMR_AEEVT_Msk | TC_CMR_ASWTRG_Msk);
	channelB_present = tc_config->channel_mode & (TC_CMR_BCPB_Msk | TC_CMR_BCPC_Msk | TC_CMR_BEEVT_Msk | TC_CMR_BSWTRG_Msk);
	
	if (tc_config->p_tc == DRV_TC_TC0)
	{
		switch (tc_config->tc_channelNumber)
		{
			case DRV_TC_TC_CH0:
				if (channelA_present != NULL)
					gpio_configure_pin(PIN_TC0_TIOA0, PIN_TC0_TIOA0_FLAGS);
				if (channelB_present != NULL)
					gpio_configure_pin(PIN_TC0_TIOB0, PIN_TC0_TIOB0_FLAGS);
			break;
		
			case DRV_TC_TC_CH1:
				if (channelA_present != NULL)
					gpio_configure_pin(PIN_TC0_TIOA1, PIN_TC0_TIOA1_FLAGS);
				if (channelB_present != NULL)
					gpio_configure_pin(PIN_TC0_TIOB1, PIN_TC0_TIOB1_FLAGS);
			break;
		
			case DRV_TC_TC_CH2:
				if (channelA_present != NULL)
					gpio_configure_pin(PIN_TC0_TIOA2, PIN_TC0_TIOA2_FLAGS);
				if (channelB_present != NULL)
					gpio_configure_pin(PIN_TC0_TIOB2, PIN_TC0_TIOB2_FLAGS);
			break;
			default:
			break;
		}
	}
	else
	{
		switch (tc_config->tc_channelNumber)
		{
			#ifdef ID_TC3
			case DRV_TC_TC_CH0:	// TODO: possibly cover it with the ifdef of the the same (ID_TC3).
				if (channelA_present != NULL)
				gpio_configure_pin(PIN_TC1_TIOA3, PIN_TC1_TIOA3_FLAGS);
				if (channelB_present != NULL)
				gpio_configure_pin(PIN_TC1_TIOB3, PIN_TC1_TIOB3_FLAGS);
			break;
			#endif
		
			#ifdef ID_TC4
			case DRV_TC_TC_CH1:
				if (channelA_present != NULL)
				gpio_configure_pin(PIN_TC1_TIOA4, PIN_TC1_TIOA4_FLAGS);
				if (channelB_present != NULL)
				gpio_configure_pin(PIN_TC1_TIOB4, PIN_TC1_TIOB4_FLAGS);
			break;
			#endif
		
			#ifdef ID_TC5
			case DRV_TC_TC_CH2:
				if (channelA_present != NULL)
				gpio_configure_pin(PIN_TC1_TIOA5, PIN_TC1_TIOA5_FLAGS);
				if (channelB_present != NULL)
				gpio_configure_pin(PIN_TC1_TIOB5, PIN_TC1_TIOB5_FLAGS);
			break;
			#endif
		
			default:
			break;
		}
	}
}

void assign_interruptHandler(drv_tc_config_t *tc_config)
{
	if (tc_config->p_tc == DRV_TC_TC0)
	{
		switch (tc_config->tc_channelNumber)
		{
			case DRV_TC_TC_CH0:
				vCallbackTable[0] = tc_config->tc_handler;
			break;
		
			case DRV_TC_TC_CH1:
				vCallbackTable[1] = tc_config->tc_handler;
			break;
		
			case DRV_TC_TC_CH2:
				vCallbackTable[2] = tc_config->tc_handler;
			break;
			default:
			break;
		}
	}
	else
	{
		switch (tc_config->tc_channelNumber)
		{
			#ifdef ID_TC3
			case DRV_TC_TC_CH0:
				vCallbackTable[3] = tc_config->tc_handler;
			break;
			#endif
		
			#ifdef ID_TC4
			case DRV_TC_TC_CH1:
				vCallbackTable[4] = tc_config->tc_handler;
			break;
			#endif
		
			#ifdef ID_TC5
			case DRV_TC_TC_CH2:
				vCallbackTable[5] = tc_config->tc_handler;
			break;
			#endif
		
			default:
			break;
		}
	}
}

void enablePeripheralClock(drv_tc_config_t *tc_config)
{
	if (tc_config->p_tc == DRV_TC_TC0)
	{
		switch (tc_config->tc_channelNumber)
		{
			case DRV_TC_TC_CH0:
				sysclk_enable_peripheral_clock(ID_TC0);
			break;
		
			case DRV_TC_TC_CH1:
				sysclk_enable_peripheral_clock(ID_TC1);
			break;
		
			case DRV_TC_TC_CH2:
				sysclk_enable_peripheral_clock(ID_TC2);
			break;
			default:
			break;
		}
	}
	else
	{
		switch (tc_config->tc_channelNumber)
		{
			#ifdef ID_TC3
			case DRV_TC_TC_CH0:
				sysclk_enable_peripheral_clock(ID_TC3);
			break;
			#endif
	
			#ifdef ID_TC4
			case DRV_TC_TC_CH1:
				sysclk_enable_peripheral_clock(ID_TC4);
			break;
			#endif
	
			#ifdef ID_TC5
			case DRV_TC_TC_CH2:
				sysclk_enable_peripheral_clock(ID_TC5);
			break;
			#endif
	
			default:
			break;
		}
	}
}

Status_t drv_tc_init(drv_tc_config_t *tc_config)
{
	//stop the timer first
	tc_stop(tc_config->p_tc, tc_config->tc_channelNumber);
	tc_disable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, TC_ALL_INTERRUPT_MASK);
	
	//now configure the module
	enablePeripheralClock(tc_config);
	configureTio(tc_config);
	if ((tc_config->enable_interrupt) && (tc_config->tc_handler != NULL))
	{
		assign_interruptHandler(tc_config);
	}
	return STATUS_PASS;
}

Status_t drv_tc_config(drv_tc_config_t *tc_config)
{
	uint32_t ra = 0, rb = 0, rc = 0;
	uint32_t clk_divisor = 0, clk_index = 0, timerClock = 0;
	uint32_t status = 0;
	
	if (tc_config->tc_mode == DRV_TC_WAVEFORM)
	{
		tc_stop(tc_config->p_tc, tc_config->tc_channelNumber);
		tc_disable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, TC_ALL_INTERRUPT_MASK);
		
		status = tc_find_mck_divisor(tc_config->frequency, sysclk_get_cpu_hz(), &clk_divisor, &clk_index, sysclk_get_cpu_hz());
		#ifdef ENABLE_TC_DEBUG_PRINTS
		printf("Frequency: %d\r\n Divisor: %d\r\n Clock index: %d\r\n", tc_config->frequency, clk_divisor, clk_index);
		#endif
		
		if (status == 1)
		{
			tc_init(tc_config->p_tc, tc_config->tc_channelNumber, (clk_index << 0) | TC_CMR_WAVE | tc_config->channel_mode);
			
			rc = ((sysclk_get_peripheral_bus_hz(tc_config->p_tc) - 1) / (2*clk_divisor) /tc_config->frequency);			#ifdef ENABLE_TC_DEBUG_PRINTS			printf("Rc value set to be: %d\r\n", rc);			#endif			tc_write_rc(tc_config->p_tc, tc_config->tc_channelNumber, rc);
			
			ra = ((100 - tc_config->duty_cycle)*rc)/100;
			#ifdef ENABLE_TC_DEBUG_PRINTS
			printf("Ra value set to be: %d\r\n", ra);
			#endif
			tc_write_ra(tc_config->p_tc, tc_config->tc_channelNumber, ra);
			
			if (tc_config->enable_interrupt)
			{
				drv_tc_enableInterrupt(tc_config);
			}
			tc_start(tc_config->p_tc, tc_config->tc_channelNumber);
			return STATUS_PASS;
		}
		return STATUS_FAIL;
	} 
	else
	{
		// Capture mode is not yet supported.
		return STATUS_FAIL;
	}
}

Status_t drv_tc_enableInterrupt(drv_tc_config_t *tc_config)
{
	tc_enable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, tc_config->interrupt_sources);
	
	if (tc_config->p_tc == DRV_TC_TC0)
	{
		switch (tc_config->tc_channelNumber)
		{
			case DRV_TC_TC_CH0:
				NVIC_EnableIRQ(TC0_IRQn);
			break;
		
			case DRV_TC_TC_CH1:
				NVIC_EnableIRQ(TC1_IRQn);
			break;
		
			case DRV_TC_TC_CH2:
				NVIC_EnableIRQ(TC2_IRQn);
			break;
			default:
			break;
		}
	}
	else
	{
		switch (tc_config->tc_channelNumber)
		{
			#ifdef ID_TC3
			case DRV_TC_TC_CH0:
				NVIC_EnableIRQ(TC3_IRQn);
			break;
			#endif
		
			#ifdef ID_TC4
			case DRV_TC_TC_CH1:
				NVIC_EnableIRQ(TC4_IRQn);
			break;
			#endif
		
			#ifdef ID_TC5
			case DRV_TC_TC_CH2:
				NVIC_EnableIRQ(TC5_IRQn);
			break;
			#endif
		
		default:
		break;
		}
	}
	return STATUS_PASS;
}

Status_t drv_tc_disableInterrupt(drv_tc_config_t *tc_config)
{
	tc_disable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, tc_config->interrupt_sources);
	
	if (tc_config->p_tc == DRV_TC_TC0)
	{
		switch (tc_config->tc_channelNumber)
		{
			case DRV_TC_TC_CH0:
				NVIC_DisableIRQ(TC0_IRQn);
			break;
		
			case DRV_TC_TC_CH1:
				NVIC_DisableIRQ(TC1_IRQn);
			break;
		
			case DRV_TC_TC_CH2:
				NVIC_DisableIRQ(TC2_IRQn);
			break;
			default:
			break;
		}
	}
	else
	{
		switch (tc_config->tc_channelNumber)
		{
			#ifdef ID_TC3
			case DRV_TC_TC_CH0:
				NVIC_DisableIRQ(TC3_IRQn);
			break;
			#endif
		
			#ifdef ID_TC4
			case DRV_TC_TC_CH1:
				NVIC_DisableIRQ(TC4_IRQn);
			break;
			#endif
		
			#ifdef ID_TC5
			case DRV_TC_TC_CH2:
				NVIC_DisableIRQ(TC5_IRQn);
			break;
			#endif
		
			default:
			break;
		}
}
return STATUS_PASS;
}

Status_t drv_tc_start(drv_tc_config_t *tc_config)
{
	tc_start(tc_config->p_tc, tc_config->tc_channelNumber);
	return STATUS_PASS;
}

Status_t drv_tc_stop(drv_tc_config_t *tc_config)
{
	tc_stop(tc_config->p_tc, tc_config->tc_channelNumber);
	return STATUS_PASS;
}

Status_t drv_tc_changeFrequency(drv_tc_config_t *tc_config, uint16_t frequency)
{
	tc_config->frequency = frequency;
	drv_tc_config(tc_config);
	return STATUS_PASS;
}

Status_t drv_tc_isInterruptGenerated(drv_tc_config_t *tc_config, uint32_t interruptSource)
{
	if ((tc_get_status(tc_config->p_tc, tc_config->tc_channelNumber) & interruptSource) == interruptSource)
	{
		return STATUS_PASS;
	}
	return STATUS_FAIL;
}

// interrupt handlers
void TC0_Handler()
{
	if (vCallbackTable[0] != NULL)
	{
		(*vCallbackTable[0])();
	}
}

void TC1_Handler()
{
	if (vCallbackTable[1] != NULL)
	{
		(*vCallbackTable[1])();
	}
}

void TC2_Handler()
{
	if (vCallbackTable[2] != NULL)
	{
		(*vCallbackTable[2])();
	}
}

void TC3_Handler()
{
	if (vCallbackTable[3] != NULL)
	{
		(*vCallbackTable[3])();
	}
}

void TC4_Handler()
{
	if (vCallbackTable[4] != NULL)
	{
		(*vCallbackTable[4])();
	}
}

void TC5_Handler()
{
	if (vCallbackTable[5] != NULL)
	{
		(*vCallbackTable[5])();
	}
}