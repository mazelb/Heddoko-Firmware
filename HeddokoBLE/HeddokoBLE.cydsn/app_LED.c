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

#include "app_LED.h"


/*******************************************************************************
* Function Name: HandleLeds
********************************************************************************
*
* Summary:
*  This function handles the toggling of Scan and Connection LED. 
*
* Parameters:
*  None.
*
* Return:
*   None.
*
*******************************************************************************/
void HandleLeds(void)
{
    static uint8        ledState        = LED_OFF;
    static uint32       advLedTimer     = ADV_LED_TIMEOUT;   
    static uint32       connLedTimer    = CONN_LED_TIMEOUT;   
    
    switch (cyBle_state)
    {
        case CYBLE_STATE_ADVERTISING:
            
            Conn_Led_Write(LED_OFF);
            
            if(--advLedTimer == 0u) 
            {
                advLedTimer = ADV_LED_TIMEOUT;
                ledState ^= LED_OFF;
                
                Adv_led_Write(ledState);
            }
            
            break;
            
        case CYBLE_STATE_CONNECTED: 
            
            Adv_led_Write(LED_OFF);
            
            if(--connLedTimer == 0u) 
            {
                connLedTimer = CONN_LED_TIMEOUT;
                ledState    ^= LED_OFF;
                
                Conn_Led_Write(ledState);
            }
            
        default:
            break;
    }
}

/* [] END OF FILE */
