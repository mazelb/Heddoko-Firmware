/*******************************************************************************
* File Name: Adv_led.h  
* Version 2.20
*
* Description:
*  This file contains the Alias definitions for Per-Pin APIs in cypins.h. 
*  Information on using these APIs can be found in the System Reference Guide.
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_Adv_led_ALIASES_H) /* Pins Adv_led_ALIASES_H */
#define CY_PINS_Adv_led_ALIASES_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"


/***************************************
*              Constants        
***************************************/
#define Adv_led_0			(Adv_led__0__PC)
#define Adv_led_0_PS		(Adv_led__0__PS)
#define Adv_led_0_PC		(Adv_led__0__PC)
#define Adv_led_0_DR		(Adv_led__0__DR)
#define Adv_led_0_SHIFT	(Adv_led__0__SHIFT)
#define Adv_led_0_INTR	((uint16)((uint16)0x0003u << (Adv_led__0__SHIFT*2u)))

#define Adv_led_INTR_ALL	 ((uint16)(Adv_led_0_INTR))


#endif /* End Pins Adv_led_ALIASES_H */


/* [] END OF FILE */
