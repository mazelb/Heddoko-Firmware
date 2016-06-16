/*******************************************************************************
* File Name: Adv_led.h  
* Version 2.10
*
* Description:
*  This file containts Control Register function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2014, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_Adv_led_H) /* Pins Adv_led_H */
#define CY_PINS_Adv_led_H

#include "cytypes.h"
#include "cyfitter.h"
#include "Adv_led_aliases.h"


/***************************************
*        Function Prototypes             
***************************************/    

void    Adv_led_Write(uint8 value) ;
void    Adv_led_SetDriveMode(uint8 mode) ;
uint8   Adv_led_ReadDataReg(void) ;
uint8   Adv_led_Read(void) ;
uint8   Adv_led_ClearInterrupt(void) ;


/***************************************
*           API Constants        
***************************************/

/* Drive Modes */
#define Adv_led_DRIVE_MODE_BITS        (3)
#define Adv_led_DRIVE_MODE_IND_MASK    (0xFFFFFFFFu >> (32 - Adv_led_DRIVE_MODE_BITS))

#define Adv_led_DM_ALG_HIZ         (0x00u)
#define Adv_led_DM_DIG_HIZ         (0x01u)
#define Adv_led_DM_RES_UP          (0x02u)
#define Adv_led_DM_RES_DWN         (0x03u)
#define Adv_led_DM_OD_LO           (0x04u)
#define Adv_led_DM_OD_HI           (0x05u)
#define Adv_led_DM_STRONG          (0x06u)
#define Adv_led_DM_RES_UPDWN       (0x07u)

/* Digital Port Constants */
#define Adv_led_MASK               Adv_led__MASK
#define Adv_led_SHIFT              Adv_led__SHIFT
#define Adv_led_WIDTH              1u


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Adv_led_PS                     (* (reg32 *) Adv_led__PS)
/* Port Configuration */
#define Adv_led_PC                     (* (reg32 *) Adv_led__PC)
/* Data Register */
#define Adv_led_DR                     (* (reg32 *) Adv_led__DR)
/* Input Buffer Disable Override */
#define Adv_led_INP_DIS                (* (reg32 *) Adv_led__PC2)


#if defined(Adv_led__INTSTAT)  /* Interrupt Registers */

    #define Adv_led_INTSTAT                (* (reg32 *) Adv_led__INTSTAT)

#endif /* Interrupt Registers */


/***************************************
* The following code is DEPRECATED and 
* must not be used.
***************************************/

#define Adv_led_DRIVE_MODE_SHIFT       (0x00u)
#define Adv_led_DRIVE_MODE_MASK        (0x07u << Adv_led_DRIVE_MODE_SHIFT)


#endif /* End Pins Adv_led_H */


/* [] END OF FILE */
