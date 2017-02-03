/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#ifndef APP_BLE_H
    
    #define APP_BLE_H
    
    #include <project.h>
    #include "stdbool.h"
    #include "app_UART.h"
    #include "main.h"
    
    /***************************************
    *       Constants
    ***************************************/
    #define NOTIFICATON_ENABLED         0x0001
    #define NOTIFICATON_DISABLED        0x0000
    
    /***************************************
    *       Function Prototypes
    ***************************************/
    void HandleBleProcessing(void);
    CYBLE_API_RESULT_T CyBle_CustomSetCharacteristicValue(CYBLE_GATT_DB_ATTR_HANDLE_T handle, 
    uint8 attrSize, uint8 *attrValue);
    CYBLE_API_RESULT_T CyBle_CustomSendNotification(CYBLE_CONN_HANDLE_T connHandle,
    CYBLE_GATT_DB_ATTR_HANDLE_T handle, CYBLE_GATT_DB_ATTR_HANDLE_T cccdHandle,
    uint8 attrSize, uint8 *attrValue);
#endif

/* [] END OF FILE */
