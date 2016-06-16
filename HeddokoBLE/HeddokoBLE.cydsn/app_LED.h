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

#if !defined(APP_LED_H)

    #define APP_LED_H
    
    #include "main.h"
    #include <project.h>    
    
    /***************************************
    *       Constants
    ***************************************/
    #define LED_OFF             0x01
    #define LED_ON              0x00
    
    #ifdef LOW_POWER_MODE
        #define ADV_LED_TIMEOUT     3
        #define CONN_LED_TIMEOUT    30
    #else
        #define ADV_LED_TIMEOUT     30000
        #define CONN_LED_TIMEOUT    50000
    #endif
    
    /***************************************
    *       Function Prototypes
    ***************************************/
    void HandleLeds(void);  
    
#endif

/* [] END OF FILE */
