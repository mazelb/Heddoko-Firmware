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
    
#endif

/* [] END OF FILE */
