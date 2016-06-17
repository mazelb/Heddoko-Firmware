/*******************************************************************************
* File Name: Conn_Led.h  
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

#if !defined(CY_PINS_Conn_Led_ALIASES_H) /* Pins Conn_Led_ALIASES_H */
#define CY_PINS_Conn_Led_ALIASES_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"


/***************************************
*              Constants        
***************************************/
#define Conn_Led_0			(Conn_Led__0__PC)
#define Conn_Led_0_PS		(Conn_Led__0__PS)
#define Conn_Led_0_PC		(Conn_Led__0__PC)
#define Conn_Led_0_DR		(Conn_Led__0__DR)
#define Conn_Led_0_SHIFT	(Conn_Led__0__SHIFT)
#define Conn_Led_0_INTR	((uint16)((uint16)0x0003u << (Conn_Led__0__SHIFT*2u)))

#define Conn_Led_INTR_ALL	 ((uint16)(Conn_Led_0_INTR))


#endif /* End Pins Conn_Led_ALIASES_H */


/* [] END OF FILE */
