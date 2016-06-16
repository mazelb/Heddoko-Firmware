/*******************************************************************************
File Name: CYBLE_custom.h
Version 1.20

Description:
 Contains the function prototypes and constants for the Custom Service.

********************************************************************************
Copyright 2014-2015, Cypress Semiconductor Corporation.  All rights reserved.
You may use this file only in accordance with the license, terms, conditions,
disclaimers, and limitations in the end user license agreement accompanying
the software package with which this file was provided.
*******************************************************************************/


#if !defined(CY_BLE_CYBLE_CUSTOM_H)
#define CY_BLE_CYBLE_CUSTOM_H

#include "BLE_gatt.h"


/***************************************
##Conditional Compilation Parameters
***************************************/

/* Maximum supported Custom Services */
#define CYBLE_CUSTOMS_SERVICE_COUNT                  (0x04u)
#define CYBLE_CUSTOMC_SERVICE_COUNT                  (0x00u)
#define CYBLE_CUSTOM_SERVICE_CHAR_COUNT              (0x03u)
#define CYBLE_CUSTOM_SERVICE_CHAR_DESCRIPTORS_COUNT  (0x02u)

/* Below are the indexes and handles of the defined Custom Services and their characteristics */
#define CYBLE_SERVER_UART_SERVICE_INDEX   (0x00u) /* Index of Server_UART service in the cyBle_customs array */
#define CYBLE_SERVER_UART_SERVER_UART_TX_DATA_CHAR_INDEX   (0x00u) /* Index of Server_UART_Tx_data characteristic */
#define CYBLE_SERVER_UART_SERVER_UART_TX_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_INDEX   (0x00u) /* Index of Client Characteristic Configuration descriptor */
#define CYBLE_SERVER_UART_SERVER_UART_RX_DATA_CHAR_INDEX   (0x01u) /* Index of Server_UART_Rx_data characteristic */
#define CYBLE_HEDDOKO_GPS_SERVICE_INDEX   (0x01u) /* Index of Heddoko: GPS service in the cyBle_customs array */
#define CYBLE_HEDDOKO_GPS_GPS_DATA_CHAR_INDEX   (0x00u) /* Index of GPS data characteristic */
#define CYBLE_HEDDOKO_GPS_GPS_DATA_CHARACTERISTIC_USER_DESCRIPTION_DESC_INDEX   (0x00u) /* Index of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_WIFI_SERVICE_INDEX   (0x02u) /* Index of Heddoko: WiFi service in the cyBle_customs array */
#define CYBLE_HEDDOKO_WIFI_SSID_CHAR_INDEX   (0x00u) /* Index of SSID characteristic */
#define CYBLE_HEDDOKO_WIFI_SSID_CHARACTERISTIC_USER_DESCRIPTION_DESC_INDEX   (0x00u) /* Index of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHAR_INDEX   (0x01u) /* Index of PassPhrase characteristic */
#define CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHARACTERISTIC_USER_DESCRIPTION_DESC_INDEX   (0x00u) /* Index of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHAR_INDEX   (0x02u) /* Index of Security type characteristic */
#define CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_VALID_RANGE_DESC_INDEX   (0x00u) /* Index of Valid Range descriptor */
#define CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHARACTERISTIC_USER_DESCRIPTION_DESC_INDEX   (0x01u) /* Index of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_RAW_DATA_SERVICE_INDEX   (0x03u) /* Index of Heddoko: Raw data service in the cyBle_customs array */
#define CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHAR_INDEX   (0x00u) /* Index of Raw data characteristic */
#define CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHARACTERISTIC_USER_DESCRIPTION_DESC_INDEX   (0x00u) /* Index of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_INDEX   (0x01u) /* Index of Client Characteristic Configuration descriptor */

#define CYBLE_SERVER_UART_SERVICE_HANDLE   (0x000Cu) /* Handle of Server_UART service */
#define CYBLE_SERVER_UART_SERVER_UART_TX_DATA_CHAR_HANDLE   (0x000Eu) /* Handle of Server_UART_Tx_data characteristic */
#define CYBLE_SERVER_UART_SERVER_UART_TX_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE   (0x000Fu) /* Handle of Client Characteristic Configuration descriptor */
#define CYBLE_SERVER_UART_SERVER_UART_RX_DATA_CHAR_HANDLE   (0x0011u) /* Handle of Server_UART_Rx_data characteristic */
#define CYBLE_HEDDOKO_GPS_SERVICE_HANDLE   (0x0012u) /* Handle of Heddoko: GPS service */
#define CYBLE_HEDDOKO_GPS_GPS_DATA_CHAR_HANDLE   (0x0014u) /* Handle of GPS data characteristic */
#define CYBLE_HEDDOKO_GPS_GPS_DATA_CHARACTERISTIC_USER_DESCRIPTION_DESC_HANDLE   (0x0015u) /* Handle of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_WIFI_SERVICE_HANDLE   (0x0016u) /* Handle of Heddoko: WiFi service */
#define CYBLE_HEDDOKO_WIFI_SSID_CHAR_HANDLE   (0x0018u) /* Handle of SSID characteristic */
#define CYBLE_HEDDOKO_WIFI_SSID_CHARACTERISTIC_USER_DESCRIPTION_DESC_HANDLE   (0x0019u) /* Handle of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHAR_HANDLE   (0x001Bu) /* Handle of PassPhrase characteristic */
#define CYBLE_HEDDOKO_WIFI_PASSPHRASE_CHARACTERISTIC_USER_DESCRIPTION_DESC_HANDLE   (0x001Cu) /* Handle of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHAR_HANDLE   (0x001Eu) /* Handle of Security type characteristic */
#define CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_VALID_RANGE_DESC_HANDLE   (0x001Fu) /* Handle of Valid Range descriptor */
#define CYBLE_HEDDOKO_WIFI_SECURITY_TYPE_CHARACTERISTIC_USER_DESCRIPTION_DESC_HANDLE   (0x0020u) /* Handle of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_RAW_DATA_SERVICE_HANDLE   (0x0021u) /* Handle of Heddoko: Raw data service */
#define CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHAR_HANDLE   (0x0023u) /* Handle of Raw data characteristic */
#define CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CHARACTERISTIC_USER_DESCRIPTION_DESC_HANDLE   (0x0024u) /* Handle of Characteristic User Description descriptor */
#define CYBLE_HEDDOKO_RAW_DATA_RAW_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE   (0x0025u) /* Handle of Client Characteristic Configuration descriptor */

	

/***************************************
##Data Struct Definition
***************************************/

#if(0u != CYBLE_CUSTOMS_SERVICE_COUNT)

/* Contains information about Custom Characteristic structure */
typedef struct
{
    /* Custom Characteristic handle */
    CYBLE_GATT_DB_ATTR_HANDLE_T customServiceCharHandle;
    /* Custom Characteristic Descriptors handles */
    CYBLE_GATT_DB_ATTR_HANDLE_T customServiceCharDescriptors[CYBLE_CUSTOM_SERVICE_CHAR_DESCRIPTORS_COUNT];
} CYBLE_CUSTOMS_INFO_T;

/* Structure with Custom Service attribute handles. */
typedef struct
{
    /* Handle of a Custom Service */
    CYBLE_GATT_DB_ATTR_HANDLE_T customServiceHandle;
    
    /* Information about Custom Characteristics */
    CYBLE_CUSTOMS_INFO_T customServiceInfo[CYBLE_CUSTOM_SERVICE_CHAR_COUNT];
} CYBLE_CUSTOMS_T;


#endif /* (0u != CYBLE_CUSTOMS_SERVICE_COUNT) */

/* DOM-IGNORE-BEGIN */
/* The custom Client functionality is not functional in current version of 
* the component.
*/
#if(0u != CYBLE_CUSTOMC_SERVICE_COUNT)

typedef struct
{
    /* Custom Descriptor handle */
    CYBLE_GATT_DB_ATTR_HANDLE_T descHandle;
	/* Custom Descriptor UUID */
	CYBLE_UUID_T uuid;           
    /* UUID Format - 16-bit (0x01) or 128-bit (0x02) */
	uint8 uuidFormat;
   
} CYBLE_CUSTOMC_DESC_T;

typedef struct
{
    /* Characteristic handle */
    CYBLE_GATT_DB_ATTR_HANDLE_T customServiceCharHandle;
	/* Characteristic end handle */
    CYBLE_GATT_DB_ATTR_HANDLE_T customServiceCharEndHandle;
	/* Characteristic UUID */
	CYBLE_UUID_T uuid;           
    /* UUID Format - 16-bit (0x01) or 128-bit (0x02) */
	uint8 uuidFormat;
	/* Number of descriptors */
    uint8 descCount;
    /* Characteristic Descriptors */
    CYBLE_CUSTOMC_DESC_T * customServiceCharDescriptors;
} CYBLE_CUSTOMC_CHAR_T;

/* Structure with discovered attributes information of Custom Service */
typedef struct
{
    /* Custom Service handle */
    CYBLE_GATT_DB_ATTR_HANDLE_T customServiceHandle;
	/* Service UUID */
	CYBLE_UUID_T uuid;           
    /* UUID Format - 16-bit (0x01) or 128-bit (0x02) */
	uint8 uuidFormat;
	/* Number of characteristics */
    uint8 charCount;
    /* Custom Service Characteristics */
    CYBLE_CUSTOMC_CHAR_T * customServiceInfo;
} CYBLE_CUSTOMC_T;

#endif /* (0u != CYBLE_CUSTOMC_SERVICE_COUNT) */
/* DOM-IGNORE-END */


#if(0u != CYBLE_CUSTOMS_SERVICE_COUNT)

extern const CYBLE_CUSTOMS_T cyBle_customs[CYBLE_CUSTOMS_SERVICE_COUNT];

#endif /* (0u != CYBLE_CUSTOMS_SERVICE_COUNT) */

/* DOM-IGNORE-BEGIN */
#if(0u != CYBLE_CUSTOMC_SERVICE_COUNT)

extern CYBLE_CUSTOMC_T cyBle_customc[CYBLE_CUSTOMC_SERVICE_COUNT];

#endif /* (0u != CYBLE_CUSTOMC_SERVICE_COUNT) */
/* DOM-IGNORE-END */

#endif /* CY_BLE_CYBLE_CUSTOM_H  */


/* [] END OF FILE */
