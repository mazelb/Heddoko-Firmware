/*
 * drv_tc.c
 *
 * Created: 2016-06-15 2:22:23 PM
 *  Author: Hriday Mehta
 */ 

#include "drv_tc.h"

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
	
	switch (tc_config->tc_channelId)
	{
		case ID_TC0:
			if (channelA_present != NULL)
				gpio_configure_pin(PIN_TC0_TIOA0, PIN_TC0_TIOA0_FLAGS);
			if (channelB_present != NULL)
				gpio_configure_pin(PIN_TC0_TIOB0, PIN_TC0_TIOB0_FLAGS);
		break;
		case ID_TC1:
			if (channelA_present != NULL)
				gpio_configure_pin(PIN_TC0_TIOA1, PIN_TC0_TIOA1_FLAGS);
			if (channelB_present != NULL)
				gpio_configure_pin(PIN_TC0_TIOB1, PIN_TC0_TIOB1_FLAGS);
		break;
		case ID_TC2:
			if (channelA_present != NULL)
				gpio_configure_pin(PIN_TC0_TIOA2, PIN_TC0_TIOA2_FLAGS);
			if (channelB_present != NULL)
				gpio_configure_pin(PIN_TC0_TIOB2, PIN_TC0_TIOB2_FLAGS);
		break;
		//case ID_TC3:
			//if (channelA_present != NULL)
				//gpio_configure_pin(PIN_TC1_TIOA3, PIN_TC1_TIOA3_FLAGS);
			//if (channelB_present != NULL)
				//gpio_configure_pin(PIN_TC1_TIOB3, PIN_TC1_TIOB3_FLAGS);
		//break;
		//case ID_TC4:
			//if (channelA_present != NULL)
				//gpio_configure_pin(PIN_TC1_TIOA4, PIN_TC1_TIOA4_FLAGS);
			//if (channelB_present != NULL)
				//gpio_configure_pin(PIN_TC1_TIOB4, PIN_TC1_TIOB4_FLAGS);
		//break;
		//case ID_TC5:
			//if (channelA_present != NULL)
				//gpio_configure_pin(PIN_TC1_TIOA5, PIN_TC1_TIOA5_FLAGS);
			//if (channelB_present != NULL)
				//gpio_configure_pin(PIN_TC1_TIOB5, PIN_TC1_TIOB5_FLAGS);
		//break;
		default:
		break;
	}
}

Status_t drv_tc_init(drv_tc_config_t *tc_config)
{
	//stop the timer first
	tc_stop(tc_config->p_tc, tc_config->tc_channelNumber);
	tc_disable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, TC_ALL_INTERRUPT_MASK);
	
	//now configure the module
	sysclk_enable_peripheral_clock(tc_config->tc_channelId);
	configureTio(tc_config);
	if ((tc_config->enable_interrupt) && (tc_config->tc_handler != NULL))
	{
		tc_config->tc_handler = (void *) TC_Handler;
	}
	return 1;
}

Status_t drv_tc_config(drv_tc_config_t *tc_config)
{
	uint16_t ra = 0, rb = 0, rc = 0;
	uint32_t clk_divisor = 0, clk_index = 0;
	uint32_t status = 0;
	
	if (tc_config->tc_mode == DRV_TC_WAVEFORM)
	{
		tc_stop(tc_config->p_tc, tc_config->tc_channelNumber);
		tc_disable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, TC_ALL_INTERRUPT_MASK);
		
		status = tc_find_mck_divisor(tc_config->frequency, sysclk_get_cpu_hz(), &clk_divisor, &clk_index, sysclk_get_main_hz());
		#ifdef ENABLE_TC_DEBUG_PRINTS
		printf("Frequency: %d\r\n Divisor: %d\r\n Clock index: %d\r\n", tc_config->frequency, clk_divisor, clk_index);
		#endif
		
		if (status == 1)
		{
			tc_init(tc_config->p_tc, tc_config->tc_channelNumber, (clk_index << 0) | TC_CMR_WAVE | tc_config->channel_mode);
			
			rc = ((sysclk_get_peripheral_bus_hz(TC) - 1) / (2*clk_divisor) /tc_config->frequency);			#ifdef ENABLE_TC_DEBUG_PRINTS			printf("Rc value set to be: %d\r\n", rc);			#endif			tc_write_rc(TC, DRV_TC_CHANNEL_WAVEFORM, rc);
			
			ra = ((100 - tc_config->duty_cycle)*rc)/100;
			#ifdef ENABLE_TC_DEBUG_PRINTS
			printf("Ra value set to be: %d\r\n", ra);
			#endif
			tc_write_ra(TC, DRV_TC_CHANNEL_WAVEFORM, ra);
			
			if (tc_config->enable_interrupt)
			{
				drv_tc_enableInterrupt(tc_config);
			}
			tc_start(tc_config->p_tc, tc_config->tc_channelNumber);
		}
	} 
	else
	{
		// Capture mode is not yet supported.
		return 0;
	}
}

Status_t drv_tc_enableInterrupt(drv_tc_config_t *tc_config)
{
	tc_enable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, tc_config->interrupt_sources);
	switch (tc_config->tc_channelId)
	{
		case ID_TC0:
			NVIC_EnableIRQ(TC0_IRQn);
		break;
		case ID_TC1:
			NVIC_EnableIRQ(TC1_IRQn);
		break;
		case ID_TC2:
			NVIC_EnableIRQ(TC2_IRQn);
		break;
		//case ID_TC3:
			//NVIC_EnableIRQ(TC3_IRQn);
		//break;
		//case ID_TC4:
			//NVIC_EnableIRQ(TC4_IRQn);
		//break;
		//case ID_TC5:
			//NVIC_EnableIRQ(TC5_IRQn);
		//break;
		default:
		break;
	}
}

Status_t drv_tc_disableInterrupt(drv_tc_config_t *tc_config)
{
	tc_disable_interrupt(tc_config->p_tc, tc_config->tc_channelNumber, tc_config->interrupt_sources);
	switch (tc_config->tc_channelId)
	{
		case ID_TC0:
			NVIC_DisableIRQ(TC0_IRQn);
		break;
		case ID_TC1:
			NVIC_DisableIRQ(TC1_IRQn);
		break;
		case ID_TC2:
			NVIC_DisableIRQ(TC2_IRQn);
		break;
		//case ID_TC3:
			//NVIC_DisableIRQ(TC3_IRQn);
		//break;
		//case ID_TC4:
			//NVIC_DisableIRQ(TC4_IRQn);
		//break;
		//case ID_TC5:
			//NVIC_DisableIRQ(TC5_IRQn);
		//break;
		default:
		break;
	}
}

Status_t drv_tc_start(drv_tc_config_t *tc_config)
{
	tc_start(tc_config->p_tc, tc_config->tc_channelNumber);
}

Status_t drv_tc_stop(drv_tc_config_t *tc_config)
{
	tc_stop(tc_config->p_tc, tc_config->tc_channelNumber);
}

Status_t drv_tc_changeFrequency(drv_tc_config_t *tc_config, uint16_t frequency)
{
	tc_config->frequency = frequency;
	drv_tc_config(tc_config);
}

Status_t drv_tc_isInterruptGenerated(drv_tc_config_t *tc_config, uint32_t interruptSource)
{
	if ((tc_get_status(tc_config->p_tc, tc_config->tc_channelNumber) & interruptSource) == interruptSource)
	{
		return STATUS_PASS;
	}
	return STATUS_FAIL;
}