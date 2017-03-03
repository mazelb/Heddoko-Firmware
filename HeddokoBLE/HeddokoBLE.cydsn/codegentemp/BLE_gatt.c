/***************************************************************************//**
* \file CYBLE_gatt.c
* \version 3.10
* 
* \brief
*  This file contains the source code for the GATT API of the BLE Component.
* 
********************************************************************************
* \copyright
* Copyright 2014-2016, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/


#include "BLE_eventHandler.h"


/***************************************
* Global variables
***************************************/

CYBLE_STATE_T cyBle_state;

#if ((CYBLE_MODE_PROFILE) && (CYBLE_BONDING_REQUIREMENT == CYBLE_BONDING_YES))
    
#if(CYBLE_MODE_PROFILE)
    #if defined(__ARMCC_VERSION)
        CY_ALIGN(CYDEV_FLS_ROW_SIZE) const CY_BLE_FLASH_STORAGE cyBle_flashStorage CY_SECTION(".cy_checksum_exclude") =
    #elif defined (__GNUC__)
        const CY_BLE_FLASH_STORAGE cyBle_flashStorage CY_SECTION(".cy_checksum_exclude")
            CY_ALIGN(CYDEV_FLS_ROW_SIZE) =
    #elif defined (__ICCARM__)
        #pragma data_alignment=CY_FLASH_SIZEOF_ROW
        #pragma location=".cy_checksum_exclude"
        const CY_BLE_FLASH_STORAGE cyBle_flashStorage =
    #endif  /* (__ARMCC_VERSION) */
    {
        { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u }, 
        {{
            0x00u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
        },
        {
            0x00u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
        },
        {
            0x00u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
        },
        {
            0x00u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
        },
        {
            0x00u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
            0x01u, 0x00u, 
        }}, 
        0x12u, /* CYBLE_GATT_DB_CCCD_COUNT */ 
        0x05u, /* CYBLE_GAP_MAX_BONDED_DEVICE */ 
    };
#endif /* (CYBLE_MODE_PROFILE) */

#endif  /* (CYBLE_MODE_PROFILE) && (CYBLE_BONDING_REQUIREMENT == CYBLE_BONDING_YES) */

#if(CYBLE_GATT_ROLE_SERVER)
    
    const CYBLE_GATTS_T cyBle_gatts =
{
    0x0008u,    /* Handle of the GATT service */
    0x000Au,    /* Handle of the Service Changed characteristic */
    0x000Bu,    /* Handle of the Client Characteristic Configuration descriptor */
};
    
    static uint8 cyBle_attValues[0x0374u] = {
    /* Device Name */
    (uint8)'H', (uint8)'e', (uint8)'d', (uint8)'d', (uint8)'o', (uint8)'k', (uint8)'o', 

    /* Appearance */
    0x00u, 0x00u, 

    /* Peripheral Preferred Connection Parameters */
    0x06u, 0x00u, 0x18u, 0x00u, 0x00u, 0x00u, 0x64u, 0x00u, 

    /* Service Changed */
    0x00u, 0x00u, 0x00u, 0x00u, 

    /* Manufacturer Name String */
    (uint8)'H', (uint8)'e', (uint8)'d', (uint8)'d', (uint8)'o', (uint8)'k', (uint8)'o', 

    /* Model Number String */
    (uint8)'V', (uint8)'e', (uint8)'r', (uint8)'s', (uint8)'i', (uint8)'o', (uint8)'n', (uint8)' ', (uint8)'1', (uint8)'.',
(uint8)'5', 

    /* Serial Number String */
    (uint8)'B', (uint8)'P', (uint8)'0', (uint8)'0', (uint8)'0', (uint8)'1', (uint8)'\0', (uint8)'\0', (uint8)'\0',
(uint8)'\0', 

    /* Hardware Revision String */
    (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0',
(uint8)'\0', 

    /* Firmware Revision String */
    (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0',
(uint8)'\0', 

    /* Software Revision String */
    (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0', (uint8)'\0',
(uint8)'\0', 

    /* System ID */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* IEEE 11073-20601 Regulatory Certification Data List */
    0x00u, 

    /* PnP ID */
    0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Battery Level */
    0x00u, 

    /* Characteristic Presentation Format */
    0x00u, 0x00u, 0x33u, 0x27u, 0x01u, 0x00u, 0x00u, 

    /* BP-Status */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'B', (uint8)'r', (uint8)'a', (uint8)'i', (uint8)'n', (uint8)' ', (uint8)'P', (uint8)'a', (uint8)'c', (uint8)'k',
(uint8)' ', (uint8)'S', (uint8)'t', (uint8)'a', (uint8)'t', (uint8)'u', (uint8)'s', 

    /* Sensor Mask */
    0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'3', (uint8)'2', (uint8)' ', (uint8)'b', (uint8)'i', (uint8)'t', (uint8)' ', (uint8)'m', (uint8)'a', (uint8)'s',
(uint8)'k', (uint8)' ', (uint8)'r', (uint8)'e', (uint8)'p', (uint8)'r', (uint8)'e', (uint8)'s', (uint8)'e', (uint8)'n',
(uint8)'t', (uint8)'i', (uint8)'n', (uint8)'g', (uint8)' ', (uint8)'c', (uint8)'o', (uint8)'n', (uint8)'n', (uint8)'e',
(uint8)'c', (uint8)'t', (uint8)'e', (uint8)'d', (uint8)' ', (uint8)'s', (uint8)'e', (uint8)'n', (uint8)'s', (uint8)'o',
(uint8)'r', (uint8)'s', (uint8)'.', (uint8)' ', 

    /* SSID */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'W', (uint8)'i', (uint8)'F', (uint8)'i', (uint8)' ', (uint8)'S', (uint8)'S', (uint8)'I', (uint8)'D', (uint8)':',
(uint8)' ', (uint8)'3', (uint8)'2', (uint8)' ', (uint8)'b', (uint8)'y', (uint8)'t', (uint8)'e', (uint8)'s', 

    /* PassPhrase */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'W', (uint8)'i', (uint8)'F', (uint8)'i', (uint8)' ', (uint8)'p', (uint8)'a', (uint8)'s', (uint8)'s', (uint8)'p',
(uint8)'h', (uint8)'r', (uint8)'a', (uint8)'s', (uint8)'e', (uint8)':', (uint8)' ', (uint8)'6', (uint8)'4', (uint8)' ',
(uint8)'b', (uint8)'y', (uint8)'t', (uint8)'e', (uint8)'s', 

    /* Security type */
    0x00u, 

    /* Valid Range */
    0x00u, 0x03u, 

    /* Characteristic User Description */
    (uint8)'E', (uint8)'n', (uint8)'u', (uint8)'m', (uint8)'e', (uint8)'r', (uint8)'a', (uint8)'t', (uint8)'i', (uint8)'o',
(uint8)'n', (uint8)' ', (uint8)'o', (uint8)'f', (uint8)' ', (uint8)'S', (uint8)'e', (uint8)'c', (uint8)'u', (uint8)'r',
(uint8)'i', (uint8)'t', (uint8)'y', (uint8)' ', (uint8)'t', (uint8)'y', (uint8)'p', (uint8)'e', (uint8)':', (uint8)' ',
(uint8)'1', (uint8)' ', (uint8)'b', (uint8)'y', (uint8)'t', (uint8)'e', 

    /* Wifi enable */
    0x00u, 

    /* Characteristic User Description */
    (uint8)'W', (uint8)'i', (uint8)'f', (uint8)'i', (uint8)' ', (uint8)'C', (uint8)'o', (uint8)'n', (uint8)'t', (uint8)'r',
(uint8)'o', (uint8)'l', (uint8)',', (uint8)' ', (uint8)'1', (uint8)'=', (uint8)'C', (uint8)'o', (uint8)'n', (uint8)'n',
(uint8)'e', (uint8)'c', (uint8)'t', (uint8)' ', (uint8)'0', (uint8)'=', (uint8)'D', (uint8)'i', (uint8)'s', (uint8)'c',
(uint8)'o', (uint8)'n', (uint8)'n', (uint8)'e', (uint8)'c', (uint8)'t', 

    /* Wifi Connection State */
    0x00u, 

    /* Characteristic User Description */
    (uint8)'W', (uint8)'i', (uint8)'f', (uint8)'i', (uint8)' ', (uint8)'C', (uint8)'o', (uint8)'n', (uint8)'n', (uint8)'e',
(uint8)'c', (uint8)'t', (uint8)'i', (uint8)'o', (uint8)'n', (uint8)' ', (uint8)'S', (uint8)'t', (uint8)'a', (uint8)'t',
(uint8)'e', (uint8)' ', (uint8)'2', (uint8)'=', (uint8)'c', (uint8)'o', (uint8)'n', (uint8)'n', (uint8)'e', (uint8)'c',
(uint8)'t', (uint8)'e', (uint8)'d', (uint8)' ', (uint8)'1', (uint8)'=', (uint8)'d', (uint8)'i', (uint8)'s', (uint8)'c',
(uint8)'o', (uint8)'n', (uint8)'n', (uint8)'e', (uint8)'c', (uint8)'t', (uint8)'e', (uint8)'d', 

    /* Recording State */
    0x00u, 

    /* Characteristic User Description */
    (uint8)'S', (uint8)'t', (uint8)'a', (uint8)'t', (uint8)'e', (uint8)' ', (uint8)'o', (uint8)'f', (uint8)' ', (uint8)'B',
(uint8)'r', (uint8)'a', (uint8)'i', (uint8)'n', (uint8)'p', (uint8)'a', (uint8)'c', (uint8)'k', (uint8)' ', (uint8)'R',
(uint8)'e', (uint8)'c', (uint8)'o', (uint8)'r', (uint8)'d', (uint8)'i', (uint8)'n', (uint8)'g', 

    /* Recording Request */
    0x00u, 

    /* Characteristic User Description */
    (uint8)'R', (uint8)'e', (uint8)'q', (uint8)'u', (uint8)'e', (uint8)'s', (uint8)'t', (uint8)' ', (uint8)'t', (uint8)'o',
(uint8)' ', (uint8)'s', (uint8)'t', (uint8)'a', (uint8)'r', (uint8)'t', (uint8)' ', (uint8)'o', (uint8)'r', (uint8)' ',
(uint8)'s', (uint8)'t', (uint8)'o', (uint8)'p', (uint8)' ', (uint8)'a', (uint8)' ', (uint8)'r', (uint8)'e', (uint8)'c',
(uint8)'o', (uint8)'r', (uint8)'d', (uint8)'i', (uint8)'n', (uint8)'g', (uint8)' ', (uint8)'S', (uint8)'t', (uint8)'a',
(uint8)'r', (uint8)'t', (uint8)' ', (uint8)'=', (uint8)' ', (uint8)'1', (uint8)',', (uint8)' ', (uint8)'S', (uint8)'t',
(uint8)'o', (uint8)'p', (uint8)' ', (uint8)'=', (uint8)' ', (uint8)'2', 

    /* Current Time */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'C', (uint8)'u', (uint8)'r', (uint8)'r', (uint8)'e', (uint8)'n', (uint8)'t', (uint8)' ', (uint8)'T', (uint8)'i',
(uint8)'m', (uint8)'e', (uint8)' ', (uint8)'s', (uint8)'e', (uint8)'t', (uint8)' ', (uint8)'o', (uint8)'n', (uint8)' ',
(uint8)'t', (uint8)'h', (uint8)'e', (uint8)' ', (uint8)'b', (uint8)'r', (uint8)'a', (uint8)'i', (uint8)'n', (uint8)'p',
(uint8)'a', (uint8)'c', (uint8)'k', 

    /* Request Current Time */
    0x00u, 

    /* Characteristic User Description */
    (uint8)'R', (uint8)'e', (uint8)'q', (uint8)'u', (uint8)'e', (uint8)'s', (uint8)'t', (uint8)' ', (uint8)'C', (uint8)'u',
(uint8)'r', (uint8)'r', (uint8)'e', (uint8)'n', (uint8)'t', (uint8)' ', (uint8)'T', (uint8)'i', (uint8)'m', (uint8)'e', 

    /* Event */
    0x00u, 

    /* Custom Descriptor */
    0x00u, 0xFBu, 0x34u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'R', (uint8)'e', (uint8)'c', (uint8)'o', (uint8)'r', (uint8)'d', (uint8)'i', (uint8)'n', (uint8)'g', (uint8)' ',
(uint8)'E', (uint8)'v', (uint8)'e', (uint8)'n', (uint8)'t', (uint8)' ', (uint8)'1', (uint8)'=', (uint8)'P', (uint8)'a',
(uint8)'i', (uint8)'n', (uint8)' ', (uint8)'2', (uint8)'=', (uint8)'C', (uint8)'o', (uint8)'n', (uint8)'c', (uint8)'e',
(uint8)'r', (uint8)'n', 

    /* GPS data */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'3', (uint8)'2', (uint8)' ', (uint8)'b', (uint8)'i', (uint8)'t', (uint8)' ', (uint8)'G', (uint8)'P', (uint8)'S',
(uint8)' ', (uint8)'d', (uint8)'a', (uint8)'t', (uint8)'a', 

    /* Location and Speed */
    0x04u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Server_UART_Tx_data */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 

    /* Server_UART_Rx_data */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Raw data */
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 

    /* Characteristic User Description */
    (uint8)'R', (uint8)'a', (uint8)'w', (uint8)' ', (uint8)'d', (uint8)'a', (uint8)'t', (uint8)'a', (uint8)':', (uint8)' ',
(uint8)'2', (uint8)'0', (uint8)' ', (uint8)'b', (uint8)'y', (uint8)'t', (uint8)'e', (uint8)'s', 

};
#if(CYBLE_GATT_DB_CCCD_COUNT != 0u)
uint8 cyBle_attValuesCCCD[CYBLE_GATT_DB_CCCD_COUNT];
#endif /* CYBLE_GATT_DB_CCCD_COUNT != 0u */

const uint8 cyBle_attUuid128[][16u] = {
    /* Heddoko: BrainPack Status */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x10u, 0xCEu, 0x03u, 0x00u },
    /* BP-Status */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x11u, 0xCEu, 0x03u, 0x00u },
    /* Sensor Mask */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x12u, 0xCEu, 0x03u, 0x00u },
    /* Heddoko: WiFi */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xF0u, 0xCDu, 0x03u, 0x00u },
    /* SSID */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xF1u, 0xCDu, 0x03u, 0x00u },
    /* PassPhrase */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xF2u, 0xCDu, 0x03u, 0x00u },
    /* Security type */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xF3u, 0xCDu, 0x03u, 0x00u },
    /* Wifi enable */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xF4u, 0xCDu, 0x03u, 0x00u },
    /* Wifi Connection State */
    { 0xFBu, 0x34u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u },
    /* Heddoko: Recording Control */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x01u, 0xCDu, 0x03u, 0x00u },
    /* Recording State */
    { 0xB0u, 0x13u, 0xB0u, 0xF9u, 0x05u, 0x08u, 0x00u, 0x00u, 0x08u, 0x00u, 0x01u, 0x00u, 0x02u, 0xCDu, 0x03u, 0x00u },
    /* Recording Request */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x03u, 0xCDu, 0x03u, 0x00u },
    /* Current Time */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x03u, 0xCDu, 0x03u, 0x00u },
    /* Request Current Time */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x04u, 0xCDu, 0x03u, 0x00u },
    /* Event */
    { 0xB0u, 0x13u, 0xB0u, 0xF9u, 0x05u, 0x08u, 0x00u, 0x00u, 0x08u, 0x00u, 0x01u, 0x00u, 0x05u, 0xCDu, 0x03u, 0x00u },
    /* Heddoko: GPS */
    { 0x31u, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xE0u, 0xCDu, 0x03u, 0x00u },
    /* GPS data */
    { 0x31u, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xE1u, 0xCDu, 0x03u, 0x00u },
    /* Server_UART */
    { 0x31u, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xD0u, 0xCDu, 0x03u, 0x00u },
    /* Server_UART_Tx_data */
    { 0x31u, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xD1u, 0xCDu, 0x03u, 0x00u },
    /* Server_UART_Rx_data */
    { 0x31u, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0xD2u, 0xCDu, 0x03u, 0x00u },
    /* Heddoko: Raw data */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x00u, 0xCEu, 0x03u, 0x00u },
    /* Raw data */
    { 0x3Bu, 0x01u, 0x9Bu, 0x5Fu, 0x80u, 0x00u, 0x00u, 0x80u, 0x00u, 0x10u, 0x00u, 0x00u, 0x01u, 0xCEu, 0x03u, 0x00u },
};

CYBLE_GATTS_ATT_GEN_VAL_LEN_T cyBle_attValuesLen[CYBLE_GATT_DB_ATT_VAL_COUNT] = {
    { 0x0007u, (void *)&cyBle_attValues[0] }, /* Device Name */
    { 0x0002u, (void *)&cyBle_attValues[7] }, /* Appearance */
    { 0x0008u, (void *)&cyBle_attValues[9] }, /* Peripheral Preferred Connection Parameters */
    { 0x0004u, (void *)&cyBle_attValues[17] }, /* Service Changed */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[0] }, /* Client Characteristic Configuration */
    { 0x0007u, (void *)&cyBle_attValues[21] }, /* Manufacturer Name String */
    { 0x000Bu, (void *)&cyBle_attValues[28] }, /* Model Number String */
    { 0x000Au, (void *)&cyBle_attValues[39] }, /* Serial Number String */
    { 0x000Au, (void *)&cyBle_attValues[49] }, /* Hardware Revision String */
    { 0x000Au, (void *)&cyBle_attValues[59] }, /* Firmware Revision String */
    { 0x000Au, (void *)&cyBle_attValues[69] }, /* Software Revision String */
    { 0x0008u, (void *)&cyBle_attValues[79] }, /* System ID */
    { 0x0001u, (void *)&cyBle_attValues[87] }, /* IEEE 11073-20601 Regulatory Certification Data List */
    { 0x0007u, (void *)&cyBle_attValues[88] }, /* PnP ID */
    { 0x0001u, (void *)&cyBle_attValues[95] }, /* Battery Level */
    { 0x0007u, (void *)&cyBle_attValues[96] }, /* Characteristic Presentation Format */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[2] }, /* Client Characteristic Configuration */
    { 0x0010u, (void *)&cyBle_attUuid128[0] }, /* Heddoko: BrainPack Status UUID */
    { 0x0010u, (void *)&cyBle_attUuid128[1] }, /* BP-Status UUID */
    { 0x0005u, (void *)&cyBle_attValues[103] }, /* BP-Status */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[4] }, /* Client Characteristic Configuration */
    { 0x0011u, (void *)&cyBle_attValues[108] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[2] }, /* Sensor Mask UUID */
    { 0x0004u, (void *)&cyBle_attValues[125] }, /* Sensor Mask */
    { 0x002Cu, (void *)&cyBle_attValues[129] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[3] }, /* Heddoko: WiFi UUID */
    { 0x0010u, (void *)&cyBle_attUuid128[4] }, /* SSID UUID */
    { 0x0020u, (void *)&cyBle_attValues[173] }, /* SSID */
    { 0x0013u, (void *)&cyBle_attValues[205] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[5] }, /* PassPhrase UUID */
    { 0x0040u, (void *)&cyBle_attValues[224] }, /* PassPhrase */
    { 0x0019u, (void *)&cyBle_attValues[288] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[6] }, /* Security type UUID */
    { 0x0001u, (void *)&cyBle_attValues[313] }, /* Security type */
    { 0x0002u, (void *)&cyBle_attValues[314] }, /* Valid Range */
    { 0x0024u, (void *)&cyBle_attValues[316] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[7] }, /* Wifi enable UUID */
    { 0x0001u, (void *)&cyBle_attValues[352] }, /* Wifi enable */
    { 0x0024u, (void *)&cyBle_attValues[353] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[8] }, /* Wifi Connection State UUID */
    { 0x0001u, (void *)&cyBle_attValues[389] }, /* Wifi Connection State */
    { 0x0030u, (void *)&cyBle_attValues[390] }, /* Characteristic User Description */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[6] }, /* Client Characteristic Configuration */
    { 0x0010u, (void *)&cyBle_attUuid128[9] }, /* Heddoko: Recording Control UUID */
    { 0x0010u, (void *)&cyBle_attUuid128[10] }, /* Recording State UUID */
    { 0x0001u, (void *)&cyBle_attValues[438] }, /* Recording State */
    { 0x001Cu, (void *)&cyBle_attValues[439] }, /* Characteristic User Description */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[8] }, /* Client Characteristic Configuration */
    { 0x0010u, (void *)&cyBle_attUuid128[11] }, /* Recording Request UUID */
    { 0x0001u, (void *)&cyBle_attValues[467] }, /* Recording Request */
    { 0x0038u, (void *)&cyBle_attValues[468] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[12] }, /* Current Time UUID */
    { 0x000Au, (void *)&cyBle_attValues[524] }, /* Current Time */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[10] }, /* Client Characteristic Configuration */
    { 0x0021u, (void *)&cyBle_attValues[534] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[13] }, /* Request Current Time UUID */
    { 0x0001u, (void *)&cyBle_attValues[567] }, /* Request Current Time */
    { 0x0014u, (void *)&cyBle_attValues[568] }, /* Characteristic User Description */
    { 0x0010u, (void *)&cyBle_attUuid128[14] }, /* Event UUID */
    { 0x0001u, (void *)&cyBle_attValues[588] }, /* Event */
    { 0x0001u, (void *)&cyBle_attValues[589] }, /* Custom Descriptor */
    { 0x0020u, (void *)&cyBle_attValues[606] }, /* Characteristic User Description */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[12] }, /* Client Characteristic Configuration */
    { 0x0010u, (void *)&cyBle_attUuid128[15] }, /* Heddoko: GPS UUID */
    { 0x0010u, (void *)&cyBle_attUuid128[16] }, /* GPS data UUID */
    { 0x0020u, (void *)&cyBle_attValues[638] }, /* GPS data */
    { 0x000Fu, (void *)&cyBle_attValues[670] }, /* Characteristic User Description */
    { 0x000Au, (void *)&cyBle_attValues[685] }, /* Location and Speed */
    { 0x0010u, (void *)&cyBle_attUuid128[17] }, /* Server_UART UUID */
    { 0x0010u, (void *)&cyBle_attUuid128[18] }, /* Server_UART_Tx_data UUID */
    { 0x0014u, (void *)&cyBle_attValues[695] }, /* Server_UART_Tx_data */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[14] }, /* Client Characteristic Configuration */
    { 0x0010u, (void *)&cyBle_attUuid128[19] }, /* Server_UART_Rx_data UUID */
    { 0x0080u, (void *)&cyBle_attValues[715] }, /* Server_UART_Rx_data */
    { 0x0010u, (void *)&cyBle_attUuid128[20] }, /* Heddoko: Raw data UUID */
    { 0x0010u, (void *)&cyBle_attUuid128[21] }, /* Raw data UUID */
    { 0x0017u, (void *)&cyBle_attValues[843] }, /* Raw data */
    { 0x0012u, (void *)&cyBle_attValues[866] }, /* Characteristic User Description */
    { 0x0002u, (void *)&cyBle_attValuesCCCD[16] }, /* Client Characteristic Configuration */
};

const CYBLE_GATTS_DB_T cyBle_gattDB[0x62u] = {
    { 0x0001u, 0x2800u /* Primary service                     */, 0x00000001u /*            */, 0x0007u, {{0x1800u, NULL}}                           },
    { 0x0002u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0003u, {{0x2A00u, NULL}}                           },
    { 0x0003u, 0x2A00u /* Device Name                         */, 0x00000201u /* rd         */, 0x0003u, {{0x0007u, (void *)&cyBle_attValuesLen[0]}} },
    { 0x0004u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0005u, {{0x2A01u, NULL}}                           },
    { 0x0005u, 0x2A01u /* Appearance                          */, 0x00000201u /* rd         */, 0x0005u, {{0x0002u, (void *)&cyBle_attValuesLen[1]}} },
    { 0x0006u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0007u, {{0x2A04u, NULL}}                           },
    { 0x0007u, 0x2A04u /* Peripheral Preferred Connection Par */, 0x00000201u /* rd         */, 0x0007u, {{0x0008u, (void *)&cyBle_attValuesLen[2]}} },
    { 0x0008u, 0x2800u /* Primary service                     */, 0x00000001u /*            */, 0x000Bu, {{0x1801u, NULL}}                           },
    { 0x0009u, 0x2803u /* Characteristic                      */, 0x00002201u /* rd,ind     */, 0x000Bu, {{0x2A05u, NULL}}                           },
    { 0x000Au, 0x2A05u /* Service Changed                     */, 0x00002201u /* rd,ind     */, 0x000Bu, {{0x0004u, (void *)&cyBle_attValuesLen[3]}} },
    { 0x000Bu, 0x2902u /* Client Characteristic Configuration */, 0x00000A04u /* rd,wr      */, 0x000Bu, {{0x0002u, (void *)&cyBle_attValuesLen[4]}} },
    { 0x000Cu, 0x2800u /* Primary service                     */, 0x00000001u /*            */, 0x001Eu, {{0x180Au, NULL}}                           },
    { 0x000Du, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x000Eu, {{0x2A29u, NULL}}                           },
    { 0x000Eu, 0x2A29u /* Manufacturer Name String            */, 0x00000201u /* rd         */, 0x000Eu, {{0x0007u, (void *)&cyBle_attValuesLen[5]}} },
    { 0x000Fu, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0010u, {{0x2A24u, NULL}}                           },
    { 0x0010u, 0x2A24u /* Model Number String                 */, 0x00000201u /* rd         */, 0x0010u, {{0x000Bu, (void *)&cyBle_attValuesLen[6]}} },
    { 0x0011u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0012u, {{0x2A25u, NULL}}                           },
    { 0x0012u, 0x2A25u /* Serial Number String                */, 0x00000201u /* rd         */, 0x0012u, {{0x000Au, (void *)&cyBle_attValuesLen[7]}} },
    { 0x0013u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0014u, {{0x2A27u, NULL}}                           },
    { 0x0014u, 0x2A27u /* Hardware Revision String            */, 0x00000201u /* rd         */, 0x0014u, {{0x000Au, (void *)&cyBle_attValuesLen[8]}} },
    { 0x0015u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0016u, {{0x2A26u, NULL}}                           },
    { 0x0016u, 0x2A26u /* Firmware Revision String            */, 0x00000201u /* rd         */, 0x0016u, {{0x000Au, (void *)&cyBle_attValuesLen[9]}} },
    { 0x0017u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x0018u, {{0x2A28u, NULL}}                           },
    { 0x0018u, 0x2A28u /* Software Revision String            */, 0x00000201u /* rd         */, 0x0018u, {{0x000Au, (void *)&cyBle_attValuesLen[10]}} },
    { 0x0019u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x001Au, {{0x2A23u, NULL}}                           },
    { 0x001Au, 0x2A23u /* System ID                           */, 0x00000201u /* rd         */, 0x001Au, {{0x0008u, (void *)&cyBle_attValuesLen[11]}} },
    { 0x001Bu, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x001Cu, {{0x2A2Au, NULL}}                           },
    { 0x001Cu, 0x2A2Au /* IEEE 11073-20601 Regulatory Certifi */, 0x00000201u /* rd         */, 0x001Cu, {{0x0001u, (void *)&cyBle_attValuesLen[12]}} },
    { 0x001Du, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x001Eu, {{0x2A50u, NULL}}                           },
    { 0x001Eu, 0x2A50u /* PnP ID                              */, 0x00000201u /* rd         */, 0x001Eu, {{0x0007u, (void *)&cyBle_attValuesLen[13]}} },
    { 0x001Fu, 0x2800u /* Primary service                     */, 0x00000001u /*            */, 0x0023u, {{0x180Fu, NULL}}                           },
    { 0x0020u, 0x2803u /* Characteristic                      */, 0x00001201u /* rd,ntf     */, 0x0023u, {{0x2A19u, NULL}}                           },
    { 0x0021u, 0x2A19u /* Battery Level                       */, 0x00001201u /* rd,ntf     */, 0x0023u, {{0x0001u, (void *)&cyBle_attValuesLen[14]}} },
    { 0x0022u, 0x2904u /* Characteristic Presentation Format  */, 0x00000201u /* rd         */, 0x0022u, {{0x0007u, (void *)&cyBle_attValuesLen[15]}} },
    { 0x0023u, 0x2902u /* Client Characteristic Configuration */, 0x00000A04u /* rd,wr      */, 0x0023u, {{0x0002u, (void *)&cyBle_attValuesLen[16]}} },
    { 0x0024u, 0x2800u /* Primary service                     */, 0x00080001u /*            */, 0x002Bu, {{0x0010u, (void *)&cyBle_attValuesLen[17]}} },
    { 0x0025u, 0x2803u /* Characteristic                      */, 0x00001201u /* rd,ntf     */, 0x0028u, {{0x0010u, (void *)&cyBle_attValuesLen[18]}} },
    { 0x0026u, 0xCE11u /* BP-Status                           */, 0x00091201u /* rd,ntf     */, 0x0028u, {{0x0005u, (void *)&cyBle_attValuesLen[19]}} },
    { 0x0027u, 0x2903u /* Client Characteristic Configuration */, 0x00010A04u /* rd,wr      */, 0x0027u, {{0x0002u, (void *)&cyBle_attValuesLen[20]}} },
    { 0x0028u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0028u, {{0x0011u, (void *)&cyBle_attValuesLen[21]}} },
    { 0x0029u, 0x2803u /* Characteristic                      */, 0x00000201u /* rd         */, 0x002Bu, {{0x0010u, (void *)&cyBle_attValuesLen[22]}} },
    { 0x002Au, 0xCE12u /* Sensor Mask                         */, 0x00090201u /* rd         */, 0x002Bu, {{0x0004u, (void *)&cyBle_attValuesLen[23]}} },
    { 0x002Bu, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x002Bu, {{0x002Cu, (void *)&cyBle_attValuesLen[24]}} },
    { 0x002Cu, 0x2800u /* Primary service                     */, 0x00080001u /*            */, 0x003Du, {{0x0010u, (void *)&cyBle_attValuesLen[25]}} },
    { 0x002Du, 0x2803u /* Characteristic                      */, 0x00000601u /* rd,wwr     */, 0x002Fu, {{0x0010u, (void *)&cyBle_attValuesLen[26]}} },
    { 0x002Eu, 0xCDF1u /* SSID                                */, 0x00090604u /* rd,wwr     */, 0x002Fu, {{0x0020u, (void *)&cyBle_attValuesLen[27]}} },
    { 0x002Fu, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x002Fu, {{0x0013u, (void *)&cyBle_attValuesLen[28]}} },
    { 0x0030u, 0x2803u /* Characteristic                      */, 0x00000401u /* wwr        */, 0x0032u, {{0x0010u, (void *)&cyBle_attValuesLen[29]}} },
    { 0x0031u, 0xCDF2u /* PassPhrase                          */, 0x00090402u /* wwr        */, 0x0032u, {{0x0040u, (void *)&cyBle_attValuesLen[30]}} },
    { 0x0032u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0032u, {{0x0019u, (void *)&cyBle_attValuesLen[31]}} },
    { 0x0033u, 0x2803u /* Characteristic                      */, 0x00000601u /* rd,wwr     */, 0x0036u, {{0x0010u, (void *)&cyBle_attValuesLen[32]}} },
    { 0x0034u, 0xCDF3u /* Security type                       */, 0x00090604u /* rd,wwr     */, 0x0036u, {{0x0001u, (void *)&cyBle_attValuesLen[33]}} },
    { 0x0035u, 0x2906u /* Valid Range                         */, 0x00010201u /* rd         */, 0x0035u, {{0x0002u, (void *)&cyBle_attValuesLen[34]}} },
    { 0x0036u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0036u, {{0x0024u, (void *)&cyBle_attValuesLen[35]}} },
    { 0x0037u, 0x2803u /* Characteristic                      */, 0x00000401u /* wwr        */, 0x0039u, {{0x0010u, (void *)&cyBle_attValuesLen[36]}} },
    { 0x0038u, 0xCDF4u /* Wifi enable                         */, 0x00090402u /* wwr        */, 0x0039u, {{0x0001u, (void *)&cyBle_attValuesLen[37]}} },
    { 0x0039u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0039u, {{0x0024u, (void *)&cyBle_attValuesLen[38]}} },
    { 0x003Au, 0x2803u /* Characteristic                      */, 0x00001201u /* rd,ntf     */, 0x003Du, {{0x0010u, (void *)&cyBle_attValuesLen[39]}} },
    { 0x003Bu, 0x0000u /* Wifi Connection State               */, 0x00091201u /* rd,ntf     */, 0x003Du, {{0x0001u, (void *)&cyBle_attValuesLen[40]}} },
    { 0x003Cu, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x003Cu, {{0x0030u, (void *)&cyBle_attValuesLen[41]}} },
    { 0x003Du, 0x2902u /* Client Characteristic Configuration */, 0x00010A04u /* rd,wr      */, 0x003Du, {{0x0002u, (void *)&cyBle_attValuesLen[42]}} },
    { 0x003Eu, 0x2800u /* Primary service                     */, 0x00080001u /*            */, 0x0051u, {{0x0010u, (void *)&cyBle_attValuesLen[43]}} },
    { 0x003Fu, 0x2803u /* Characteristic                      */, 0x00001201u /* rd,ntf     */, 0x0042u, {{0x0010u, (void *)&cyBle_attValuesLen[44]}} },
    { 0x0040u, 0xCD02u /* Recording State                     */, 0x00091201u /* rd,ntf     */, 0x0042u, {{0x0001u, (void *)&cyBle_attValuesLen[45]}} },
    { 0x0041u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0041u, {{0x001Cu, (void *)&cyBle_attValuesLen[46]}} },
    { 0x0042u, 0x2902u /* Client Characteristic Configuration */, 0x00010A04u /* rd,wr      */, 0x0042u, {{0x0002u, (void *)&cyBle_attValuesLen[47]}} },
    { 0x0043u, 0x2803u /* Characteristic                      */, 0x00000C01u /* wr,wwr     */, 0x0045u, {{0x0010u, (void *)&cyBle_attValuesLen[48]}} },
    { 0x0044u, 0xCD03u /* Recording Request                   */, 0x00090C02u /* wr,wwr     */, 0x0045u, {{0x0001u, (void *)&cyBle_attValuesLen[49]}} },
    { 0x0045u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0045u, {{0x0038u, (void *)&cyBle_attValuesLen[50]}} },
    { 0x0046u, 0x2803u /* Characteristic                      */, 0x00001801u /* wr,ntf     */, 0x0049u, {{0x0010u, (void *)&cyBle_attValuesLen[51]}} },
    { 0x0047u, 0xCD03u /* Current Time                        */, 0x00091802u /* wr,ntf     */, 0x0049u, {{0x000Au, (void *)&cyBle_attValuesLen[52]}} },
    { 0x0048u, 0x2902u /* Client Characteristic Configuration */, 0x00010A04u /* rd,wr      */, 0x0048u, {{0x0002u, (void *)&cyBle_attValuesLen[53]}} },
    { 0x0049u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0049u, {{0x0021u, (void *)&cyBle_attValuesLen[54]}} },
    { 0x004Au, 0x2803u /* Characteristic                      */, 0x00000C01u /* wr,wwr     */, 0x004Cu, {{0x0010u, (void *)&cyBle_attValuesLen[55]}} },
    { 0x004Bu, 0xCD04u /* Request Current Time                */, 0x00090C02u /* wr,wwr     */, 0x004Cu, {{0x0001u, (void *)&cyBle_attValuesLen[56]}} },
    { 0x004Cu, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x004Cu, {{0x0014u, (void *)&cyBle_attValuesLen[57]}} },
    { 0x004Du, 0x2803u /* Characteristic                      */, 0x00001401u /* wwr,ntf    */, 0x0051u, {{0x0010u, (void *)&cyBle_attValuesLen[58]}} },
    { 0x004Eu, 0xCD05u /* Event                               */, 0x00091402u /* wwr,ntf    */, 0x0051u, {{0x0001u, (void *)&cyBle_attValuesLen[59]}} },
    { 0x004Fu, 0x0000u /* Custom Descriptor                   */, 0x00090001u /*            */, 0x004Fu, {{0x0001u, (void *)&cyBle_attValuesLen[60]}} },
    { 0x0050u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0050u, {{0x0020u, (void *)&cyBle_attValuesLen[61]}} },
    { 0x0051u, 0x2902u /* Client Characteristic Configuration */, 0x00010A04u /* rd,wr      */, 0x0051u, {{0x0002u, (void *)&cyBle_attValuesLen[62]}} },
    { 0x0052u, 0x2800u /* Primary service                     */, 0x00080001u /*            */, 0x0057u, {{0x0010u, (void *)&cyBle_attValuesLen[63]}} },
    { 0x0053u, 0x2803u /* Characteristic                      */, 0x00000401u /* wwr        */, 0x0055u, {{0x0010u, (void *)&cyBle_attValuesLen[64]}} },
    { 0x0054u, 0xCDE1u /* GPS data                            */, 0x00090402u /* wwr        */, 0x0055u, {{0x0020u, (void *)&cyBle_attValuesLen[65]}} },
    { 0x0055u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0055u, {{0x000Fu, (void *)&cyBle_attValuesLen[66]}} },
    { 0x0056u, 0x2803u /* Characteristic                      */, 0x00000A01u /* rd,wr      */, 0x0057u, {{0x2A67u, NULL}}                           },
    { 0x0057u, 0x2A67u /* Location and Speed                  */, 0x00010A04u /* rd,wr      */, 0x0057u, {{0x000Au, (void *)&cyBle_attValuesLen[67]}} },
    { 0x0058u, 0x2800u /* Primary service                     */, 0x00080001u /*            */, 0x005Du, {{0x0010u, (void *)&cyBle_attValuesLen[68]}} },
    { 0x0059u, 0x2803u /* Characteristic                      */, 0x00001001u /* ntf        */, 0x005Bu, {{0x0010u, (void *)&cyBle_attValuesLen[69]}} },
    { 0x005Au, 0xCDD1u /* Server_UART_Tx_data                 */, 0x00091000u /* ntf        */, 0x005Bu, {{0x0014u, (void *)&cyBle_attValuesLen[70]}} },
    { 0x005Bu, 0x2902u /* Client Characteristic Configuration */, 0x00010A04u /* rd,wr      */, 0x005Bu, {{0x0002u, (void *)&cyBle_attValuesLen[71]}} },
    { 0x005Cu, 0x2803u /* Characteristic                      */, 0x00000401u /* wwr        */, 0x005Du, {{0x0010u, (void *)&cyBle_attValuesLen[72]}} },
    { 0x005Du, 0xCDD2u /* Server_UART_Rx_data                 */, 0x00090402u /* wwr        */, 0x005Du, {{0x0080u, (void *)&cyBle_attValuesLen[73]}} },
    { 0x005Eu, 0x2800u /* Primary service                     */, 0x00080001u /*            */, 0x0062u, {{0x0010u, (void *)&cyBle_attValuesLen[74]}} },
    { 0x005Fu, 0x2803u /* Characteristic                      */, 0x00001601u /* rd,wwr,ntf */, 0x0062u, {{0x0010u, (void *)&cyBle_attValuesLen[75]}} },
    { 0x0060u, 0xCE01u /* Raw data                            */, 0x00091604u /* rd,wwr,ntf */, 0x0062u, {{0x0017u, (void *)&cyBle_attValuesLen[76]}} },
    { 0x0061u, 0x2901u /* Characteristic User Description     */, 0x00010201u /* rd         */, 0x0061u, {{0x0012u, (void *)&cyBle_attValuesLen[77]}} },
    { 0x0062u, 0x2903u /* Client Characteristic Configuration */, 0x00010A04u /* rd,wr      */, 0x0062u, {{0x0002u, (void *)&cyBle_attValuesLen[78]}} },
};


#endif /* (CYBLE_GATT_ROLE_SERVER) */

#if(CYBLE_GATT_ROLE_CLIENT)
    
CYBLE_CLIENT_STATE_T cyBle_clientState;
CYBLE_GATTC_T cyBle_gattc;
CYBLE_GATT_ATTR_HANDLE_RANGE_T cyBle_gattcDiscoveryRange;
    
#endif /* (CYBLE_GATT_ROLE_CLIENT) */


#if(CYBLE_GATT_ROLE_SERVER)

/****************************************************************************** 
* Function Name: CyBle_GattsReInitGattDb
***************************************************************************//**
* 
*  Reinitializes the GATT database.
* 
*  \return
*  CYBLE_API_RESULT_T: An API result states if the API succeeded or failed with
*  error codes:

*  Errors codes                          | Description
*  ------------                          | -----------
*  CYBLE_ERROR_OK						 | GATT database was reinitialized successfully.
*  CYBLE_ERROR_INVALID_STATE             | If the function is called in any state except CYBLE_STATE_DISCONNECTED.
*  CYBLE_ERROR_INVALID_PARAMETER         | If the Database has zero entries or is a NULL pointer.
* 
******************************************************************************/
CYBLE_API_RESULT_T CyBle_GattsReInitGattDb(void)
{
    CYBLE_API_RESULT_T apiResult;
    
    if(CYBLE_STATE_DISCONNECTED == CyBle_GetState())
    {
        apiResult = CyBle_GattsDbRegister(cyBle_gattDB, CYBLE_GATT_DB_INDEX_COUNT, CYBLE_GATT_DB_MAX_VALUE_LEN);
    }
    else
    {
        apiResult = CYBLE_ERROR_INVALID_STATE;
    }
    
    return(apiResult);
}


/****************************************************************************** 
* Function Name: CyBle_GattsWriteEventHandler
***************************************************************************//**
* 
*  Handles the Write Request Event for GATT service.
* 
*  \param eventParam: The pointer to the data structure specified by the event.
* 
*  \return
*  CYBLE_GATT_ERR_CODE_T: An API result returns one of the following status 
*  values.

*  Errors codes                          | Description
*  --------------------                  | -----------
*  CYBLE_GATT_ERR_NONE                   | Write is successful.
* 
******************************************************************************/
CYBLE_GATT_ERR_CODE_T CyBle_GattsWriteEventHandler(CYBLE_GATTS_WRITE_REQ_PARAM_T *eventParam)
{
    CYBLE_GATT_ERR_CODE_T gattErr = CYBLE_GATT_ERR_NONE;
    
    /* Client Characteristic Configuration descriptor write request */
    if(eventParam->handleValPair.attrHandle == cyBle_gatts.cccdHandle)
    {
        /* Store value to database */
        gattErr = CyBle_GattsWriteAttributeValue(&eventParam->handleValPair, 0u, 
                        &eventParam->connHandle, CYBLE_GATT_DB_PEER_INITIATED);
        
        if(CYBLE_GATT_ERR_NONE == gattErr)
        {
            if(CYBLE_IS_INDICATION_ENABLED_IN_PTR(eventParam->handleValPair.value.val))
            {
                CyBle_ApplCallback((uint32)CYBLE_EVT_GATTS_INDICATION_ENABLED, eventParam);
            }
            else
            {
                CyBle_ApplCallback((uint32)CYBLE_EVT_GATTS_INDICATION_DISABLED, eventParam);
            }
        }
        cyBle_eventHandlerFlag &= (uint8)~CYBLE_CALLBACK;
    }
    return (gattErr);
}


#endif /* (CYBLE_GATT_ROLE_SERVER) */

#if(CYBLE_GATT_ROLE_CLIENT)


/****************************************************************************** 
* Function Name: CyBle_GattcStartDiscovery
***************************************************************************//**
* 
*  Starts the automatic server discovery process. Two events may be generated 
*  after calling this function - CYBLE_EVT_GATTC_DISCOVERY_COMPLETE or 
*  CYBLE_EVT_GATTC_ERROR_RSP. The CYBLE_EVT_GATTC_DISCOVERY_COMPLETE event is 
*  generated when the remote device was successfully discovered. The
*  CYBLE_EVT_GATTC_ERROR_RSP is generated if the device discovery is failed.
* 
*  \param connHandle: The handle which consists of the device ID and ATT connection ID.
* 
* \return
*	CYBLE_API_RESULT_T : Return value indicates if the function succeeded or
*                        failed. Following are the possible error codes.
*
*   <table>	
*   <tr>
*	  <th>Errors codes</th>
*	  <th>Description</th>
*	</tr>
*	<tr>
*	  <td>CYBLE_ERROR_OK</td>
*	  <td>On successful operation</td>
*	</tr>
*	<tr>
*	  <td>CYBLE_ERROR_INVALID_PARAMETER</td>
*	  <td>'connHandle' value does not represent any existing entry.</td>
*	</tr>
*	<tr>
*	  <td>CYBLE_ERROR_INVALID_OPERATION</td>
*	  <td>The operation is not permitted</td>
*	</tr>
*   <tr>
*	  <td>CYBLE_ERROR_MEMORY_ALLOCATION_FAILED</td>
*	  <td>Memory allocation failed</td>
*	</tr>
*   <tr>
*	  <td>CYBLE_ERROR_INVALID_STATE</td>
*	  <td>If the function is called in any state except connected or discovered</td>
*	</tr>
*   </table>
* 
******************************************************************************/
CYBLE_API_RESULT_T CyBle_GattcStartDiscovery(CYBLE_CONN_HANDLE_T connHandle)
{
    uint8 j;
    CYBLE_API_RESULT_T apiResult;
    
    if((CyBle_GetState() != CYBLE_STATE_CONNECTED) || 
       ((CyBle_GetClientState() != CYBLE_CLIENT_STATE_CONNECTED) && 
        (CyBle_GetClientState() != CYBLE_CLIENT_STATE_DISCOVERED))) 
    {
        apiResult = CYBLE_ERROR_INVALID_STATE;
    }
    else
    {
        /* Clean old discovery information */
        for(j = 0u; j < (uint8) CYBLE_SRVI_COUNT; j++)
        {
            (void)memset(&cyBle_serverInfo[j].range, 0, sizeof(cyBle_serverInfo[0].range));
        }

        cyBle_connHandle = connHandle;
        cyBle_gattcDiscoveryRange.startHandle = CYBLE_GATT_ATTR_HANDLE_START_RANGE;
        cyBle_gattcDiscoveryRange.endHandle = CYBLE_GATT_ATTR_HANDLE_END_RANGE;
        
        CyBle_ServiceInit();
        
        apiResult = CyBle_GattcDiscoverAllPrimaryServices(connHandle);

        if(CYBLE_ERROR_OK == apiResult)
        {
            CyBle_SetClientState(CYBLE_CLIENT_STATE_SRVC_DISCOVERING);
            cyBle_eventHandlerFlag |= CYBLE_AUTO_DISCOVERY;
        }
    }
    
    return (apiResult);
}


/****************************************************************************** 
* Function Name: CyBle_GattcStartPartialDiscovery
***************************************************************************//**
* 
*  Starts the automatic server discovery process as per the range provided
*  on a GATT Server to which it is connected. This API could be used for 
*  partial server discovery after indication received to the Service Changed
*  Characteristic Value. Two events may be generated 
*  after calling this function - CYBLE_EVT_GATTC_DISCOVERY_COMPLETE or 
*  CYBLE_EVT_GATTC_ERROR_RSP. The CYBLE_EVT_GATTC_DISCOVERY_COMPLETE event is 
*  generated when the remote device was successfully discovered. The
*  CYBLE_EVT_GATTC_ERROR_RSP is generated if the device discovery is failed.
* 
*  \param connHandle: The handle which consists of the device ID and ATT connection ID.
*  \param startHandle: Start of affected attribute handle range.
*  \param endHandle: End of affected attribute handle range.
* 
*  \return
*	CYBLE_API_RESULT_T : Return value indicates if the function succeeded or
*                        failed. Following are the possible error codes.
*
*   <table>	
*   <tr>
*	  <th>Errors codes</th>
*	  <th>Description</th>
*	</tr>
*	<tr>
*	  <td>CYBLE_ERROR_OK</td>
*	  <td>On successful operation</td>
*	</tr>
*	<tr>
*	  <td>CYBLE_ERROR_INVALID_PARAMETER</td>
*	  <td>'connHandle' value does not represent any existing entry.</td>
*	</tr>
*	<tr>
*	  <td>CYBLE_ERROR_INVALID_OPERATION</td>
*	  <td>The operation is not permitted</td>
*	</tr>
*   <tr>
*	  <td>CYBLE_ERROR_MEMORY_ALLOCATION_FAILED</td>
*	  <td>Memory allocation failed</td>
*	</tr>
*   <tr>
*	  <td>CYBLE_ERROR_INVALID_STATE</td>
*	  <td>If the function is called in any state except connected or discovered</td>
*	</tr>
*   </table>
* 
******************************************************************************/
CYBLE_API_RESULT_T CyBle_GattcStartPartialDiscovery(CYBLE_CONN_HANDLE_T connHandle,
                        CYBLE_GATT_DB_ATTR_HANDLE_T startHandle, CYBLE_GATT_DB_ATTR_HANDLE_T endHandle)
{
    uint8 j;
    CYBLE_API_RESULT_T apiResult;
    
    if((CyBle_GetState() != CYBLE_STATE_CONNECTED) || 
       ((CyBle_GetClientState() != CYBLE_CLIENT_STATE_CONNECTED) && 
        (CyBle_GetClientState() != CYBLE_CLIENT_STATE_DISCOVERED))) 
    {
        apiResult = CYBLE_ERROR_INVALID_STATE;
    }
    else
    {
        /* Clean old discovery information of affected attribute range */
        for(j = 0u; j < (uint8) CYBLE_SRVI_COUNT; j++)
        {
            if((cyBle_serverInfo[j].range.startHandle >= startHandle) &&
               (cyBle_serverInfo[j].range.startHandle <= endHandle))
            {
                (void)memset(&cyBle_serverInfo[j].range, 0, sizeof(cyBle_serverInfo[0].range));
            }
        }

        cyBle_connHandle = connHandle;
        cyBle_gattcDiscoveryRange.startHandle = startHandle;
        cyBle_gattcDiscoveryRange.endHandle = endHandle;

        CyBle_ServiceInit();

        apiResult = CyBle_GattcDiscoverPrimaryServices(connHandle, &cyBle_gattcDiscoveryRange);

        if(CYBLE_ERROR_OK == apiResult)
        {
            CyBle_SetClientState(CYBLE_CLIENT_STATE_SRVC_DISCOVERING);
            cyBle_eventHandlerFlag |= CYBLE_AUTO_DISCOVERY;
        }
    }
    
    return (apiResult);
}


/******************************************************************************
* Function Name: CyBle_GattcDiscoverCharacteristicsEventHandler
***************************************************************************//**
* 
*  This function is called on receiving a "CYBLE_EVT_GATTC_READ_BY_TYPE_RSP"
*  event. Based on the service UUID, an appropriate data structure is populated
*  using the data received as part of the callback.
* 
*  \param *discCharInfo: The pointer to a characteristic information structure.
* 
* \return
*  None
* 
******************************************************************************/
void CyBle_GattcDiscoverCharacteristicsEventHandler(CYBLE_DISC_CHAR_INFO_T *discCharInfo)
{
    if(discCharInfo->uuid.uuid16 == CYBLE_UUID_CHAR_SERVICE_CHANGED)
    {
        CyBle_CheckStoreCharHandle(cyBle_gattc.serviceChanged);
    }
}


/******************************************************************************
* Function Name: CyBle_GattcDiscoverCharDescriptorsEventHandler
***************************************************************************//**
* 
*  This function is called on receiving a "CYBLE_EVT_GATTC_FIND_INFO_RSP" event.
*  Based on the descriptor UUID, an appropriate data structure is populated 
*  using the data received as part of the callback.
* 
*  \param *discDescrInfo: The pointer to a descriptor information structure.
*  \param discoveryService: The index of the service instance
* 
* \return
*  None
* 
******************************************************************************/
void CyBle_GattcDiscoverCharDescriptorsEventHandler(CYBLE_DISC_DESCR_INFO_T *discDescrInfo)
{
    if(discDescrInfo->uuid.uuid16 == CYBLE_UUID_CHAR_CLIENT_CONFIG)
    {
        CyBle_CheckStoreCharDescrHandle(cyBle_gattc.cccdHandle);
    }
}


/******************************************************************************
* Function Name: CyBle_GattcIndicationEventHandler
***************************************************************************//**
* 
*  Handles the Indication Event.
* 
*  \param *eventParam: The pointer to the data structure specified by the event.
* 
* \return
*  None.
* 
******************************************************************************/
void CyBle_GattcIndicationEventHandler(CYBLE_GATTC_HANDLE_VALUE_IND_PARAM_T *eventParam)
{
    if(cyBle_gattc.serviceChanged.valueHandle == eventParam->handleValPair.attrHandle)
    {
        CyBle_ApplCallback((uint32)CYBLE_EVT_GATTC_INDICATION, eventParam);
        cyBle_eventHandlerFlag &= (uint8)~CYBLE_CALLBACK;
    }
}


#endif /* (CYBLE_GATT_ROLE_CLIENT) */


/* [] END OF FILE */
