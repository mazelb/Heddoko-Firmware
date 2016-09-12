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

#ifndef MAIN_H

    #define MAIN_H
    
    #include <project.h>
    #include "stdbool.h"
    #include "app_Ble.h"
    #include "app_LED.h"
    
    #define PACKET_TYPE_MASTER_CONTROL                  0x01
    #define PACKET_TYPE_BLUETOOTH_MODULE                0x04
    
    #define PACKET_COMMAND_ID_GPS_DATA_REQ              0x41
    #define PACKET_COMMAND_ID_GPS_DATA_RESP             0x42
    #define PACKET_COMMAND_ID_BP_STATUS                 0x43
    #define PACKET_COMMAND_ID_DEFAULT_WIFI_DATA         0x48
    #define PACKET_COMMAND_ID_ALL_WIFI_DATA_REQ			0x49
    #define PACKET_COMMAND_ID_ALL_WIFI_DATA_RESP		0x4a
    #define PACKET_COMMAND_ID_SEND_RAW_DATA_TO_BLE      0x4b
    #define PACKET_COMMAND_ID_SEND_RAW_DATA_TO_MASTER   0x4c
    #define PACKET_COMMAND_ID_GET_RAW_DATA_REQ          0x4d
    #define PACKET_COMMAND_ID_GET_RAW_DATA_RESP         0x4e
    #define PACKET_COMMAND_ID_START_FAST_ADV            0x4f
    
    typedef enum
    {
        STATUS_PASS = 0,
        STATUS_FAIL = 1,
        STATUS_EOF = 2,
        STATUS_EAGAIN = 3
    }status_t;
    
    
    /***************************************
    *   Conditional compilation parameters
    ***************************************/      
    //#define     FLOW_CONTROL
    //#define     PRINT_MESSAGE_LOG
    #define     LOW_POWER_MODE
    
    /***************************************
    *       Function Prototypes
    ***************************************/
    void AppCallBack(uint32 , void *);
    void makeSendPacket(uint8 dataType, uint8_t* payload, uint16_t payloadSize);
    void getSendAttrData(CYBLE_GATT_DB_ATTR_HANDLE_T attrHandle, uint8 outDataType, uint32 size);   // get data from the Database and send it to data board
    void getSendWiFiDataAll(void);      // get all WIFI parameters from the Database and send it to data board
    void sendUnsentData();
    
#endif

/* [] END OF FILE */
