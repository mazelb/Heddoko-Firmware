/*******************************************************************************
* File Name: Adv_led.c  
* Version 2.20
*
* Description:
*  This file contains APIs to set up the Pins component for low power modes.
*
* Note:
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "cytypes.h"
#include "Adv_led.h"

static Adv_led_BACKUP_STRUCT  Adv_led_backup = {0u, 0u, 0u};


/*******************************************************************************
* Function Name: Adv_led_Sleep
****************************************************************************//**
*
* \brief Stores the pin configuration and prepares the pin for entering chip 
*  deep-sleep/hibernate modes. This function must be called for SIO and USBIO
*  pins. It is not essential if using GPIO or GPIO_OVT pins.
*
* <b>Note</b> This function is available in PSoC 4 only.
*
* \return 
*  None 
*  
* \sideeffect
*  For SIO pins, this function configures the pin input threshold to CMOS and
*  drive level to Vddio. This is needed for SIO pins when in device 
*  deep-sleep/hibernate modes.
*
* \funcusage
*  \snippet Adv_led_SUT.c usage_Adv_led_Sleep_Wakeup
*******************************************************************************/
void Adv_led_Sleep(void)
{
    #if defined(Adv_led__PC)
        Adv_led_backup.pcState = Adv_led_PC;
    #else
        #if (CY_PSOC4_4200L)
            /* Save the regulator state and put the PHY into suspend mode */
            Adv_led_backup.usbState = Adv_led_CR1_REG;
            Adv_led_USB_POWER_REG |= Adv_led_USBIO_ENTER_SLEEP;
            Adv_led_CR1_REG &= Adv_led_USBIO_CR1_OFF;
        #endif
    #endif
    #if defined(CYIPBLOCK_m0s8ioss_VERSION) && defined(Adv_led__SIO)
        Adv_led_backup.sioState = Adv_led_SIO_REG;
        /* SIO requires unregulated output buffer and single ended input buffer */
        Adv_led_SIO_REG &= (uint32)(~Adv_led_SIO_LPM_MASK);
    #endif  
}


/*******************************************************************************
* Function Name: Adv_led_Wakeup
****************************************************************************//**
*
* \brief Restores the pin configuration that was saved during Pin_Sleep().
*
* For USBIO pins, the wakeup is only triggered for falling edge interrupts.
*
* <b>Note</b> This function is available in PSoC 4 only.
*
* \return 
*  None
*  
* \funcusage
*  Refer to Adv_led_Sleep() for an example usage.
*******************************************************************************/
void Adv_led_Wakeup(void)
{
    #if defined(Adv_led__PC)
        Adv_led_PC = Adv_led_backup.pcState;
    #else
        #if (CY_PSOC4_4200L)
            /* Restore the regulator state and come out of suspend mode */
            Adv_led_USB_POWER_REG &= Adv_led_USBIO_EXIT_SLEEP_PH1;
            Adv_led_CR1_REG = Adv_led_backup.usbState;
            Adv_led_USB_POWER_REG &= Adv_led_USBIO_EXIT_SLEEP_PH2;
        #endif
    #endif
    #if defined(CYIPBLOCK_m0s8ioss_VERSION) && defined(Adv_led__SIO)
        Adv_led_SIO_REG = Adv_led_backup.sioState;
    #endif
}


/* [] END OF FILE */
