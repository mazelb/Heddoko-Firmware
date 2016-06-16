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

#include "app_UART.h"
#include "app_Ble.h"
#include "pkt_packetParser.h"
#include "main.h"


/*****************************************************************************************
* Function Name: HandleUartTxTraffic
******************************************************************************************
*
* Summary:
*  This function takes data from UART RX buffer and pushes it to the server 
*  as Notifications.
*
* Parameters:
*  uint16 - CCCD for checking if notifications are enabled  
*
* Return:
*   None.
*
*****************************************************************************************/
void HandleUartTxTraffic(uint16 txDataClientConfigDesc)
{
    uint8   index;
    uint8   uartTxData[MAX_MTU_SIZE - 3];
    uint16  uartTxDataLength;
    
    static uint16 uartIdleCount = UART_IDLE_TIMEOUT;
    
    CYBLE_API_RESULT_T                  bleApiResult;
    CYBLE_GATTS_HANDLE_VALUE_NTF_T      uartTxDataNtf;
    
    uartTxDataLength = UART_SpiUartGetRxBufferSize();   //NOTE: Get the length of Data received over UART
    
    #ifdef FLOW_CONTROL
        if(uartTxDataLength >= (UART_UART_RX_BUFFER_SIZE - (UART_UART_RX_BUFFER_SIZE/2)))
        {
            DisableUartRxInt();
        }
        else
        {
            EnableUartRxInt();
        }
    #endif
    
    if((0 != uartTxDataLength) && (NOTIFICATON_ENABLED == txDataClientConfigDesc))
    {
        if(uartTxDataLength >= (mtuSize - 3))
        {
            uartIdleCount       = UART_IDLE_TIMEOUT;
            uartTxDataLength    = mtuSize - 3;
        }
        else
        {
            if(--uartIdleCount == 0)
            {
                /*uartTxDataLength remains unchanged */;
            }
            else
            {
                uartTxDataLength = 0;
            }
        }
        
        if(0 != uartTxDataLength)
        {
            uartIdleCount       = UART_IDLE_TIMEOUT;
            
            for(index = 0; index < uartTxDataLength; index++)
            {
                uartTxData[index] = (uint8) UART_UartGetByte();
            }
            
            uartTxDataNtf.value.val  = uartTxData;
            uartTxDataNtf.value.len  = uartTxDataLength;
            uartTxDataNtf.attrHandle = CYBLE_SERVER_UART_SERVER_UART_TX_DATA_CHAR_HANDLE;
            
            #ifdef FLOW_CONTROL
                DisableUartRxInt();
            #endif
            
            do
            {
                bleApiResult = CyBle_GattsNotification(cyBle_connHandle, &uartTxDataNtf);
                CyBle_ProcessEvents();
            }
            while((CYBLE_ERROR_OK != bleApiResult)  && (CYBLE_STATE_CONNECTED == cyBle_state));
        }
    }
}

/*****************************************************************************************
* Function Name: HandleUartRxTraffic
******************************************************************************************
*
* Summary:
*  This function takes data from received "write without response" command from
*  server and, pushes it to the UART TX buffer. 
*
* Parameters:
*  CYBLE_GATTS_WRITE_REQ_PARAM_T * - the "write without response" param as
*                                    recieved by the BLE stack
*
* Return:
*   None.
*
*****************************************************************************************/
void HandleUartRxTraffic(CYBLE_GATTS_WRITE_REQ_PARAM_T * uartRxDataWrReq)
{
    if(uartRxDataWrReq->handleValPair.attrHandle == CYBLE_SERVER_UART_SERVER_UART_RX_DATA_CHAR_HANDLE)
    {
        UART_SpiUartPutArray(uartRxDataWrReq->handleValPair.value.val, \
                                    (uint32) uartRxDataWrReq->handleValPair.value.len);
    }
    else if (uartRxDataWrReq->handleValPair.attrHandle == CYBLE_HEDDOKO_GPS_GPS_DATA_CHAR_HANDLE)
    {
        UART_UartPutString("Received GPS data: \r\n");
        UART_UartPutChar(uartRxDataWrReq->handleValPair.value.len);
//        UART_SpiUartPutArray(uartRxDataWrReq->handleValPair.value.val, \
//                                    (uint32) uartRxDataWrReq->handleValPair.value.len);
        makeSendPacket(PACKET_COMMAND_ID_GPS_DATA_RESP, uartRxDataWrReq->handleValPair.value.val,\
                        (uint16_t) uartRxDataWrReq->handleValPair.value.len);
    }
    else if (uartRxDataWrReq->handleValPair.attrHandle == CYBLE_HEDDOKO_WIFI_SSID_CHAR_HANDLE)
    {
        UART_UartPutString("Received SSID data: \r\n");
//        UART_SpiUartPutArray(uartRxDataWrReq->handleValPair.value.val, \
//                                    (uint32) uartRxDataWrReq->handleValPair.value.len);
        makeSendPacket(PACKET_COMMAND_ID_SSID_DATA_RESP, uartRxDataWrReq->handleValPair.value.val,\
                        (uint16_t) uartRxDataWrReq->handleValPair.value.len);
    }
    else if (uartRxDataWrReq->handleValPair.attrHandle == CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHAR_HANDLE)
    {
        UART_UartPutString("Received Passphrase data: \r\n");
//        UART_SpiUartPutArray(uartRxDataWrReq->handleValPair.value.val, \
//                                    (uint32) uartRxDataWrReq->handleValPair.value.len);
        makeSendPacket(PACKET_COMMAND_ID_PASSPHRASE_DATA_RESP, uartRxDataWrReq->handleValPair.value.val,\
                        (uint16_t) uartRxDataWrReq->handleValPair.value.len);
    }
    else if (uartRxDataWrReq->handleValPair.attrHandle == CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHAR_HANDLE)
    {
        UART_UartPutString("Received Security type data: \r\n");
//        UART_SpiUartPutArray(uartRxDataWrReq->handleValPair.value.val, \
//                                    (uint32) uartRxDataWrReq->handleValPair.value.len);
        makeSendPacket(PACKET_COMMAND_ID_SECURITY_TYPE_DATA_RESP, uartRxDataWrReq->handleValPair.value.val,\
                        (uint16_t) uartRxDataWrReq->handleValPair.value.len);
    }
    else if (uartRxDataWrReq->handleValPair.attrHandle == CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHAR_HANDLE)
    {
        UART_UartPutString("Received raw data: \r\n");
//        UART_SpiUartPutArray(uartRxDataWrReq->handleValPair.value.val, \
//                                    (uint32) uartRxDataWrReq->handleValPair.value.len);
        makeSendPacket(PACKET_COMMAND_ID_SEND_RAW_DATA_RESP, uartRxDataWrReq->handleValPair.value.val,\
                        (uint16_t) uartRxDataWrReq->handleValPair.value.len);
    }
}

/*****************************************************************************************
* Function Name: DisableUartRxInt
******************************************************************************************
*
* Summary:
*  This function disables the UART RX interrupt.
*
* Parameters:
*   None.
*
* Return:
*   None.
*
*****************************************************************************************/
void DisableUartRxInt(void)
{
    UART_INTR_RX_MASK_REG &= ~UART_RX_INTR_MASK;  
}

/*****************************************************************************************
* Function Name: EnableUartRxInt
******************************************************************************************
*
* Summary:
*  This function enables the UART RX interrupt.
*
* Parameters:
*   None.
*
* Return:
*   None.
*
*****************************************************************************************/
void EnableUartRxInt(void)
{
    UART_INTR_RX_MASK_REG |= UART_RX_INTR_MASK;  
}

/* [] END OF FILE */
