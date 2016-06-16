/*******************************************************************************
File Name: CYBLE_custom.c
Version 1.20

Description:
 Contains the source code for the Custom Service.

********************************************************************************
Copyright 2014-2015, Cypress Semiconductor Corporation.  All rights reserved.
You may use this file only in accordance with the license, terms, conditions,
disclaimers, and limitations in the end user license agreement accompanying
the software package with which this file was provided.
*******************************************************************************/


#include "BLE_eventHandler.h"

#if(0u != CYBLE_CUSTOMS_SERVICE_COUNT)

/* If any custom service with custom characterisctis is defined in the
* customizer's GUI their handles will be present in this array.
*/
/* This array contains attribute handles for the defined Custom Services and their characteristics and descriptors.
   The array index definitions are located in the CYBLE_custom.h file. */
const CYBLE_CUSTOMS_T cyBle_customs[0x04u] = {

    /* Server_UART service */
    {
        0x000Cu, /* Handle of the Server_UART service */ 
        {

            /* Server_UART_Tx_data characteristic */
            {
                0x000Eu, /* Handle of the Server_UART_Tx_data characteristic */ 
                
                /* Array of Descriptors handles */
                {
                    0x000Fu, /* Handle of the Client Characteristic Configuration descriptor */ 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },

            /* Server_UART_Rx_data characteristic */
            {
                0x0011u, /* Handle of the Server_UART_Rx_data characteristic */ 
                
                /* Array of Descriptors handles */
                {
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },
            {
                CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                
                /* Array of Descriptors handles */
                {
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },
        }, 
    },

    /* Heddoko: GPS service */
    {
        0x0012u, /* Handle of the Heddoko: GPS service */ 
        {

            /* GPS data characteristic */
            {
                0x0014u, /* Handle of the GPS data characteristic */ 
                
                /* Array of Descriptors handles */
                {
                    0x0015u, /* Handle of the Characteristic User Description descriptor */ 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },
            {
                CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                
                /* Array of Descriptors handles */
                {
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },
            {
                CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                
                /* Array of Descriptors handles */
                {
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },
        }, 
    },

    /* Heddoko: WiFi service */
    {
        0x0016u, /* Handle of the Heddoko: WiFi service */ 
        {

            /* SSID characteristic */
            {
                0x0018u, /* Handle of the SSID characteristic */ 
                
                /* Array of Descriptors handles */
                {
                    0x0019u, /* Handle of the Characteristic User Description descriptor */ 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },

            /* PassPhrase characteristic */
            {
                0x001Bu, /* Handle of the PassPhrase characteristic */ 
                
                /* Array of Descriptors handles */
                {
                    0x001Cu, /* Handle of the Characteristic User Description descriptor */ 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },

            /* Security type characteristic */
            {
                0x001Eu, /* Handle of the Security type characteristic */ 
                
                /* Array of Descriptors handles */
                {
                    0x001Fu, /* Handle of the Valid Range descriptor */ 
                    0x0020u, /* Handle of the Characteristic User Description descriptor */ 
                }, 
            },
        }, 
    },

    /* Heddoko: Raw data service */
    {
        0x0021u, /* Handle of the Heddoko: Raw data service */ 
        {

            /* Raw data characteristic */
            {
                0x0023u, /* Handle of the Raw data characteristic */ 
                
                /* Array of Descriptors handles */
                {
                    0x0024u, /* Handle of the Characteristic User Description descriptor */ 
                    0x0025u, /* Handle of the Client Characteristic Configuration descriptor */ 
                }, 
            },
            {
                CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                
                /* Array of Descriptors handles */
                {
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },
            {
                CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                
                /* Array of Descriptors handles */
                {
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                    CYBLE_GATT_INVALID_ATTR_HANDLE_VALUE, 
                }, 
            },
        }, 
    },
};


#endif /* (0u != CYBLE_CUSTOMS_SERVICE_COUNT) */

#if(0u != CYBLE_CUSTOMC_SERVICE_COUNT)

#endif /* (0u != CYBLE_CUSTOMC_SERVICE_COUNT) */


/* [] END OF FILE */
