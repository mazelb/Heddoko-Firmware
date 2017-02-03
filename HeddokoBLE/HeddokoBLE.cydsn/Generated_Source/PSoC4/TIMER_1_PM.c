/*******************************************************************************
* File Name: TIMER_1_PM.c
* Version 2.10
*
* Description:
*  This file contains the setup, control, and status commands to support
*  the component operations in the low power mode.
*
* Note:
*  None
*
********************************************************************************
* Copyright 2013-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "TIMER_1.h"

static TIMER_1_BACKUP_STRUCT TIMER_1_backup;


/*******************************************************************************
* Function Name: TIMER_1_SaveConfig
********************************************************************************
*
* Summary:
*  All configuration registers are retention. Nothing to save here.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void TIMER_1_SaveConfig(void)
{

}


/*******************************************************************************
* Function Name: TIMER_1_Sleep
********************************************************************************
*
* Summary:
*  Stops the component operation and saves the user configuration.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void TIMER_1_Sleep(void)
{
    if(0u != (TIMER_1_BLOCK_CONTROL_REG & TIMER_1_MASK))
    {
        TIMER_1_backup.enableState = 1u;
    }
    else
    {
        TIMER_1_backup.enableState = 0u;
    }

    TIMER_1_Stop();
    TIMER_1_SaveConfig();
}


/*******************************************************************************
* Function Name: TIMER_1_RestoreConfig
********************************************************************************
*
* Summary:
*  All configuration registers are retention. Nothing to restore here.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void TIMER_1_RestoreConfig(void)
{

}


/*******************************************************************************
* Function Name: TIMER_1_Wakeup
********************************************************************************
*
* Summary:
*  Restores the user configuration and restores the enable state.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void TIMER_1_Wakeup(void)
{
    TIMER_1_RestoreConfig();

    if(0u != TIMER_1_backup.enableState)
    {
        TIMER_1_Enable();
    }
}


/* [] END OF FILE */
