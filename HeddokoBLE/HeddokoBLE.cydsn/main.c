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

#include <project.h>
#include "main.h"

#include "pkt_packetParser.h"

/*  Static function forward declarations    */
static void saveWifiDefaultConfig(rawPacket_t* packet);   // send the default wifi config received from data board
static void sendUnsentWifiData();   // send new wifi data available if any
void saveReceivedRawData(uint8 *data, uint16_t length);
void saveReceivedBpStatusData(uint8 *data, uint16_t length);

#define RAW_DATA_SIZE 20
#define SSID_DATA_SIZE 32
#define PASSPHRASE_DATA_SIZE 64
#define SECURITY_TYPE_DATA_SIZE 1
#define BP_STATUS_DATA_SIZE 5
#define SENSOR_MASK_SIZE 4
uint8 rawData[RAW_DATA_SIZE] = {0};
uint8 bpStatusData[BP_STATUS_DATA_SIZE + SENSOR_MASK_SIZE] = {0};
struct 
{
    uint8 ssid[SSID_DATA_SIZE];
    uint8 passphrase[PASSPHRASE_DATA_SIZE];
    uint8 securityType;
}wifi_data;

typedef struct
{
    uint8_t ssid[SSID_DATA_SIZE];
    uint8_t passphrase[PASSPHRASE_DATA_SIZE];
    uint8_t securityType;   
    uint8_t serialNumber[10];
    uint8_t fwVersion[10]; //update this to define
    uint8_t hwRevision[10];
    uint8_t modelString[20]; //the model of the brainpack     
}ble_pkt_initialData_t;

bool newRawDataAvailable = true;
bool newBpStatusDataAvailable = false;
bool newWifiDataAvailable = false;
bool restartFastAdv = false;
rawPacket_t dataPacket;

extern uint8 txDataClientConfigDesc[2];
extern uint8 rawDataConfigDesc[2];
extern uint8 bpStatusConfigDesc[2];

void cmd_processPacket(rawPacket_t* packet)
{
    CYBLE_API_RESULT_T      bleApiResult;
    CYBLE_STATE_T bleState;
    if (packet->payloadSize < 2)
    {
        #ifdef PRINT_MESSAGE_LOG
        UART_UartPutString("Packet incomplete\r\n");
        #endif
        return; //if payload size is less than 2 then it is not a valid packet
    }
    
    if (packet->payload[0] == PACKET_TYPE_MASTER_CONTROL)
    {
        //only process the packets received from the controller.
        switch (packet->payload[1])
        {            
            case PACKET_COMMAND_ID_GPS_DATA_REQ:
                #ifdef PRINT_MESSAGE_LOG
                UART_UartPutString("Received GPS data request\r\n");
                #endif
                getSendAttrData(CYBLE_HEDDOKO_GPS_GPS_DATA_CHAR_HANDLE, PACKET_COMMAND_ID_GPS_DATA_RESP, 32);
            break;
                
            case PACKET_COMMAND_ID_BP_STATUS:
                #ifdef PRINT_MESSAGE_LOG
                UART_UartPutString("Received Brain Pack Status\r\n");
                #endif
                saveReceivedBpStatusData((uint8 *) &packet->payload[2], (packet->payloadSize - 2)); // remove two bytes of header
            break;
                
//            case PACKET_COMMAND_ID_SSID_DATA_REQ:
//                #ifdef PRINT_MESSAGE_LOG
//                UART_UartPutString("Received SSID data request\r\n");
//                #endif
//                getSendAttrData(CYBLE_HEDDOKO_WIFI_SSID_CHAR_HANDLE, PACKET_COMMAND_ID_SSID_DATA_RESP, 32);
//            break;
//            
//            case PACKET_COMMAND_ID_PASSPHRASE_DATA_REQ:
//                #ifdef PRINT_MESSAGE_LOG
//                UART_UartPutString("Received Passphrase data request\r\n");
//                #endif
//                getSendAttrData(CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHAR_HANDLE, PACKET_COMMAND_ID_PASSPHRASE_DATA_RESP, 64);
//            break;
//            
//            case PACKET_COMMAND_ID_SECURITY_TYPE_DATA_REQ:
//                #ifdef PRINT_MESSAGE_LOG
//                UART_UartPutString("Received Security type data request\r\n");
//                #endif
//                getSendAttrData(CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHAR_HANDLE, PACKET_COMMAND_ID_SECURITY_TYPE_DATA_RESP, 1);
//            break;
            
            case PACKET_COMMAND_ID_BLE_INITIAL_PARAMETERS:    // default WIFI data received from data board on boot-up
                #ifdef PRINT_MESSAGE_LOG
                UART_UartPutString("Received default WIFI config\r\n");
                #endif
                saveWifiDefaultConfig(packet);
            break;
                
            case PACKET_COMMAND_ID_ALL_WIFI_DATA_REQ:
                #ifdef PRINT_MESSAGE_LOG
                UART_UartPutString("Received All WiFi data request\r\n");
                #endif
                getSendWiFiDataAll();       // fetch complete WiFi data and send it
            break;
            
            case PACKET_COMMAND_ID_GET_RAW_DATA_REQ:
                #ifdef PRINT_MESSAGE_LOG
                UART_UartPutString("Received get raw data request\r\n");
                #endif
                getSendAttrData(CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHAR_HANDLE, PACKET_COMMAND_ID_GET_RAW_DATA_RESP, 20);
            break;
            
            case PACKET_COMMAND_ID_SEND_RAW_DATA_TO_BLE:
                #ifdef PRINT_MESSAGE_LOG
                UART_UartPutString("Received new raw data from data board\r\n");
                #endif
                saveReceivedRawData((uint8 *) &packet->payload[2], (packet->payloadSize - 2));  // remove two bytes for header
            break;
                
            case PACKET_COMMAND_ID_START_FAST_ADV:
                if (cyBle_state == CYBLE_STATE_DISCONNECTED)
                {
                    bleApiResult = CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
                    bleState = CyBle_GetState();
                }
                else
                {
                    CyBle_GappStopAdvertisement();
                    restartFastAdv = true;
                }
            break;
            
            default:
                #ifdef PRINT_MESSAGE_LOG 
                UART_UartPutString("Received unknown command request\r\n");
                #endif
            break;
        }
    }
}

pkt_packetParserConfiguration_t packetParserConfig =
{
	.packetReceivedCallback = cmd_processPacket
};

int main()
{
     #ifdef LOW_POWER_MODE    
        CYBLE_LP_MODE_T         lpMode;
        CYBLE_BLESS_STATE_T     blessState;
    #endif
    
    CYBLE_API_RESULT_T      bleApiResult;
    
    CyGlobalIntEnable;
    
     /* Start UART and BLE component */
    CySysTickInit();
    CySysTickStart();
    UART_Start();   
    pkt_packetParserInit(&packetParserConfig);
    bleApiResult = CyBle_Start(AppCallBack); 
    Adv_led_SetDriveMode(CY_SYS_PINS_DM_STRONG);
    Conn_Led_SetDriveMode(CY_SYS_PINS_DM_STRONG);

    if(bleApiResult == CYBLE_ERROR_OK)
    {
        #ifdef PRINT_MESSAGE_LOG
            UART_UartPutString("\n\rDevice role \t: PERIPHERAL");
            
            #ifdef LOW_POWER_MODE
                UART_UartPutString("\n\rLow Power Mode \t: ENABLED");
            #else
                UART_UartPutString("\n\rLow Power Mode \t: DISABLED");
            #endif
            
            #ifdef FLOW_CONTROL
                UART_UartPutString("\n\rFlow Control \t: ENABLED");  
            #else
                UART_UartPutString("\n\rFlow Control \t: DISABLED");
            #endif
            
        #endif
    }
    else
    {
        #ifdef PRINT_MESSAGE_LOG   
            UART_UartPutString("\n\r\t\tCyBle stack initilization FAILED!!! \n\r ");
        #endif
        
        /* Enter infinite loop */
        while(1);
    }
    
    CyBle_ProcessEvents();
    
    for(;;)
    {
        /* Place your application code here. */
         #ifdef LOW_POWER_MODE
            
            if((CyBle_GetState() != CYBLE_STATE_INITIALIZING) && (CyBle_GetState() != CYBLE_STATE_DISCONNECTED))
            {
                /* Enter DeepSleep mode between connection intervals */
                
                lpMode = CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP);
                CyGlobalIntDisable;
                blessState = CyBle_GetBleSsState();

                if(lpMode == CYBLE_BLESS_DEEPSLEEP) 
                {   
                    if((blessState == CYBLE_BLESS_STATE_ECO_ON || blessState == CYBLE_BLESS_STATE_DEEPSLEEP) && \
                            (UART_SpiUartGetTxBufferSize() + UART_GET_TX_FIFO_SR_VALID) == 0u)
                    {
                        #ifdef FLOW_CONTROL
                            EnableUartRxInt();
                        #endif
                        
                        CySysPmSleep();
                        
                        #ifdef FLOW_CONTROL
                            DisableUartRxInt();
                        #endif
                    }
                }
                else
                {
                    if((blessState != CYBLE_BLESS_STATE_EVENT_CLOSE) && \
                            (UART_SpiUartGetTxBufferSize() + UART_GET_TX_FIFO_SR_VALID) == 0u)
                    {
                        #ifdef FLOW_CONTROL
                            EnableUartRxInt();
                        #endif
                        
                        CySysPmSleep();
                        
                        #ifdef FLOW_CONTROL
                            DisableUartRxInt();
                        #endif
                    }
                }
                CyGlobalIntEnable;
                
                /* Handle advertising led blinking */
                HandleLeds();
            }
        #else
            HandleLeds();
        #endif
        
        /*******************************************************************
        *  Process all pending BLE events in the stack
        *******************************************************************/       
        HandleBleProcessing();
        CyBle_ProcessEvents();
        pkt_getRawPacket(&dataPacket, 200, 0);
    }
}

void makeSendPacket(uint8 dataType, uint8_t* payload, uint16_t payloadSize)
{
    uint8 outputBuffer[RAW_PACKET_MAX_SIZE] = {0};
    
    outputBuffer[0] = PACKET_TYPE_BLUETOOTH_MODULE;
    outputBuffer[1] = dataType;
    
    memcpy(outputBuffer+2, payload, payloadSize);
    pkt_SendRawPacket(outputBuffer, payloadSize+2);
}

void getSendAttrData(CYBLE_GATT_DB_ATTR_HANDLE_T attrHandle, uint8 outDataType, uint32 size)
{
    uint8 outputBuffer[RAW_PACKET_MAX_SIZE] = {0};
    CYBLE_GATT_HANDLE_VALUE_PAIR_T handlePair = {{0, size, size}, attrHandle};
    
    outputBuffer[0] = PACKET_TYPE_BLUETOOTH_MODULE;
    outputBuffer[1] = outDataType;
    
    handlePair.value.val = (uint8 *) &outputBuffer[2];
    
    CyBle_GattsReadAttributeValue(&handlePair, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    pkt_SendRawPacket(outputBuffer, size+2);
}

void getSendWiFiDataAll(void)
{
    uint8 outputBuffer[RAW_PACKET_MAX_SIZE] = {0};
    CYBLE_GATT_HANDLE_VALUE_PAIR_T handlePair = {{0, SSID_DATA_SIZE, SSID_DATA_SIZE}, CYBLE_HEDDOKO_WIFI_SSID_CHAR_HANDLE};
    
    outputBuffer[0] = PACKET_TYPE_BLUETOOTH_MODULE;
    outputBuffer[1] = PACKET_COMMAND_ID_ALL_WIFI_DATA_RESP;
    
    handlePair.value.val = (uint8 *) &outputBuffer[2];
    //get the SSID data
    CyBle_GattsReadAttributeValue(&handlePair, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    
    //get the passphrase
    handlePair.attrHandle = CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHAR_HANDLE;
    handlePair.value.len = PASSPHRASE_DATA_SIZE;
    handlePair.value.actualLen = PASSPHRASE_DATA_SIZE;
    handlePair.value.val = (uint8 *) &outputBuffer[34];
    CyBle_GattsReadAttributeValue(&handlePair, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    
    //get security type 
    handlePair.attrHandle = CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHAR_HANDLE;
    handlePair.value.len = SECURITY_TYPE_DATA_SIZE;
    handlePair.value.actualLen = SECURITY_TYPE_DATA_SIZE;
    handlePair.value.val = (uint8 *) &outputBuffer[98];
    CyBle_GattsReadAttributeValue(&handlePair, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    
    //we have all the data, pass it over UART now
    pkt_SendRawPacket(outputBuffer, 99);
}

void saveWifiDefaultConfig(rawPacket_t* packet)
{
    ble_pkt_initialData_t* initialData = (ble_pkt_initialData_t*)&(packet->payload[2]); 
    
    
    CYBLE_GATT_HANDLE_VALUE_PAIR_T handlePair = {{0, SSID_DATA_SIZE, SSID_DATA_SIZE}, CYBLE_HEDDOKO_WIFI_SSID_CHAR_HANDLE};
    
    //memset(&wifi_data, 0, sizeof(wifi_data));
    //memcpy(wifi_data.ssid, (uint8 *) &packet->payload[2], SSID_DATA_SIZE);    
    //memcpy(wifi_data.passphrase, (uint8 *) &packet->payload[34], PASSPHRASE_DATA_SIZE);
    //wifi_data.securityType = packet->payload[98];
    uint16 stringLength = 0;     
    // save SSID data
    handlePair.attrHandle = CYBLE_HEDDOKO_WIFI_SSID_CHAR_HANDLE;
    stringLength = strnlen((const char*)initialData->ssid,SSID_DATA_SIZE);  
    handlePair.value.len = stringLength;
    handlePair.value.actualLen = stringLength;
    handlePair.value.val = initialData->ssid;
    
    CyBle_GattsWriteAttributeValue(&handlePair, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    CyBle_ProcessEvents();
    // save passphrase data
    handlePair.attrHandle = CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHAR_HANDLE;
    stringLength = strnlen((const char*)initialData->passphrase,PASSPHRASE_DATA_SIZE);  
    handlePair.value.len = stringLength;
    handlePair.value.actualLen = stringLength;
    handlePair.value.val = initialData->passphrase;
    CyBle_GattsWriteAttributeValue(&handlePair, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    CyBle_ProcessEvents();
    // save security type data
    handlePair.attrHandle = CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHAR_HANDLE;
    handlePair.value.len = SECURITY_TYPE_DATA_SIZE;
    handlePair.value.actualLen = SECURITY_TYPE_DATA_SIZE;
    handlePair.value.val = &initialData->securityType;
    CyBle_GattsWriteAttributeValue(&handlePair, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    

    //save the model number
    stringLength = strnlen((const char*)initialData->modelString,20);  
    CyBle_DissSetCharacteristicValue(CYBLE_DIS_MODEL_NUMBER, stringLength,initialData->modelString);    
    CyBle_ProcessEvents();
    // save the serial number
    stringLength = strnlen((const char*)initialData->serialNumber,10);  
    CyBle_DissSetCharacteristicValue(CYBLE_DIS_SERIAL_NUMBER, stringLength,initialData->serialNumber);
    CyBle_ProcessEvents();
    //save the fw number
    stringLength = strnlen((const char*)initialData->fwVersion,10);  
    CyBle_DissSetCharacteristicValue(CYBLE_DIS_FIRMWARE_REV, stringLength,initialData->fwVersion);  
    CyBle_ProcessEvents();
    //save the hw number
    stringLength = strnlen((const char*)initialData->hwRevision,10);  
    CyBle_DissSetCharacteristicValue(CYBLE_DIS_HARDWARE_REV, stringLength,initialData->hwRevision);    
    CyBle_ProcessEvents();
    // set the flag to indicate the presence of new data
    //newWifiDataAvailable = true;
}

/*  OBSOLETE: WiFi should not send out notifications
static void sendUnsentWifiData()
{
    CYBLE_API_RESULT_T                  bleApiResult;
    CYBLE_GATTS_HANDLE_VALUE_NTF_T      uartTxDataNtf;
    
    if (newWifiDataAvailable)
    {
        if (NOTIFICATON_ENABLED == txDataClientConfigDesc[0])
        {
            uartTxDataNtf.value.val  = wifi_data.ssid;
            uartTxDataNtf.value.len  = SSID_DATA_SIZE;
            uartTxDataNtf.attrHandle = CYBLE_HEDDOKO_WIFI_SSID_CHAR_HANDLE;
            
            do
            {
                bleApiResult = CyBle_GattsNotification(cyBle_connHandle, &uartTxDataNtf);
                CyBle_ProcessEvents();
            }
            while((CYBLE_ERROR_OK != bleApiResult)  && (CYBLE_STATE_CONNECTED == cyBle_state));
            
            // send passphrase data
            uartTxDataNtf.value.val  = wifi_data.passphrase;
            uartTxDataNtf.value.len  = PASSPHRASE_DATA_SIZE;
            uartTxDataNtf.attrHandle = CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHAR_HANDLE;
            
            do
            {
                bleApiResult = CyBle_GattsNotification(cyBle_connHandle, &uartTxDataNtf);
                CyBle_ProcessEvents();
            }
            while((CYBLE_ERROR_OK != bleApiResult)  && (CYBLE_STATE_CONNECTED == cyBle_state));
            
            // send security type data
            uartTxDataNtf.value.val  = &wifi_data.securityType;
            uartTxDataNtf.value.len  = SECURITY_TYPE_DATA_SIZE;
            uartTxDataNtf.attrHandle = CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHAR_HANDLE;
            
            do
            {
                bleApiResult = CyBle_GattsNotification(cyBle_connHandle, &uartTxDataNtf);
                CyBle_ProcessEvents();
            }
            while((CYBLE_ERROR_OK != bleApiResult)  && (CYBLE_STATE_CONNECTED == cyBle_state));
        }
        
         newWifiDataAvailable = false;
    }
}
*/

void sendUnsentRawData()
{
    CYBLE_API_RESULT_T                  bleApiResult;
    CYBLE_GATTS_HANDLE_VALUE_NTF_T      uartTxDataNtf;
    
    if (newRawDataAvailable)
    {
        if (NOTIFICATON_ENABLED == rawDataConfigDesc[0])
        {
            uartTxDataNtf.value.val  = rawData;
            uartTxDataNtf.value.len  = RAW_DATA_SIZE;
            uartTxDataNtf.attrHandle = CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHAR_HANDLE;
            
            do
            {
                bleApiResult = CyBle_GattsNotification(cyBle_connHandle, &uartTxDataNtf);
                CyBle_ProcessEvents();
            }
            while((CYBLE_ERROR_OK != bleApiResult)  && (CYBLE_STATE_CONNECTED == cyBle_state));
        }
         newRawDataAvailable = false;
    }
}

void sendUnsentBpStatusData()
{
    CYBLE_API_RESULT_T                  bleApiResult;
    CYBLE_GATTS_HANDLE_VALUE_NTF_T      uartTxDataNtf;
    
    if (newBpStatusDataAvailable)
    {
        if (NOTIFICATON_ENABLED == bpStatusConfigDesc[0])
        {
            uartTxDataNtf.value.val  = bpStatusData;
            uartTxDataNtf.value.len  = BP_STATUS_DATA_SIZE;
            uartTxDataNtf.attrHandle = CYBLE_HEDDOKO_BRAINPACK_STATUS_BPSTATUS_CHAR_HANDLE;
            
            do
            {
                bleApiResult = CyBle_GattsNotification(cyBle_connHandle, &uartTxDataNtf);
                CyBle_ProcessEvents();
            }
            while((CYBLE_ERROR_OK != bleApiResult)  && (CYBLE_STATE_CONNECTED == cyBle_state));
        }
         newBpStatusDataAvailable = false;
    }
}

void saveReceivedBpStatusData(uint8 *data, uint16_t length)
{
    CYBLE_GATT_HANDLE_VALUE_PAIR_T handlePair = {{0, BP_STATUS_DATA_SIZE, BP_STATUS_DATA_SIZE}, CYBLE_HEDDOKO_BRAINPACK_STATUS_BPSTATUS_CHAR_HANDLE};
    
    if((length != 0) && (length <= (BP_STATUS_DATA_SIZE + SENSOR_MASK_SIZE)))
    {
        memset(bpStatusData, 0, BP_STATUS_DATA_SIZE + SENSOR_MASK_SIZE);
        memcpy(bpStatusData, data, length);
        newBpStatusDataAvailable = true;        
        handlePair.value.val = (uint8 *) bpStatusData;
        CyBle_GattsWriteAttributeValue(&handlePair, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
        CyBle_ProcessEvents();
        handlePair.attrHandle = CYBLE_HEDDOKO_BRAINPACK_STATUS_SENSOR_MASK_CHAR_HANDLE;
        handlePair.value.len = 4;
        handlePair.value.actualLen = 4;
        handlePair.value.val = (bpStatusData + 5); //pointer to the start of the mask data.  
        CyBle_GattsWriteAttributeValue(&handlePair, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
        CyBle_ProcessEvents();
        handlePair.attrHandle = CYBLE_HEDDOKO_RECORDING_CONTROL_RECORDING_STATE_CHAR_HANDLE;
        handlePair.value.len = 1;
        handlePair.value.actualLen = 1;
        handlePair.value.val = (bpStatusData + 2); //pointer to the current state.  
        CyBle_GattsWriteAttributeValue(&handlePair, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);       
        CyBle_ProcessEvents();
        CyBle_BassSetCharacteristicValue(0, 0,1, &bpStatusData[0]);        
    }
}

void saveReceivedRawData(uint8 *data, uint16_t length)
{
    CYBLE_GATT_HANDLE_VALUE_PAIR_T handlePair = {{0, RAW_DATA_SIZE, RAW_DATA_SIZE}, CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHAR_HANDLE};
    
    if((length != 0) && (length <= RAW_DATA_SIZE))
    {
        memset(rawData, 0, RAW_DATA_SIZE);
        memcpy(rawData, data, length);
        newRawDataAvailable = true;        
        handlePair.value.val = (uint8 *) rawData;
        CyBle_GattsWriteAttributeValue(&handlePair, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
    }
}

void sendUnsentData()
{
    // send notifications
    sendUnsentRawData();
    //sendUnsentWifiData();
    sendUnsentBpStatusData();
}

/* [] END OF FILE */
