/*
 * tftp_fileTransferClient.h
 *
 * Created: 11/22/2016 2:38:56 PM
 *  Author: sean
 */ 


#ifndef TFTP_FILETRANSFERCLIENT_H_
#define TFTP_FILETRANSFERCLIENT_H_
#include "common.h"
#include "net_wirelessNetwork.h"
#define TFTP_FILE_BUFFER_SIZE 2000
#define TFTP_PACKET_TIMEOUT 500 //0.25 seconds for now... TODO update this value
#define TFTP_BLOCK_SIZE 1350
#define TFTP_BLOCK_SIZE_STRING "1350"
#define TFTP_MAX_BLOCK_RETRY 20
#define TFTP_WINDOW_SIZE_STRING "8"
#define TFTP_WINDOW_SIZE 8

typedef enum 
{
    TFTP_TRANSFER_TYPE_GET_FILE = 1,
    TFTP_TRANSFER_TYPE_SEND_FILE = 2    
}tftp_transferType_t;

typedef enum
{
    TFTP_OPCODE_RRQ = 1,
    TFTP_OPCODE_WRQ = 2,
    TFTP_OPCODE_DATA = 3,
    TFTP_OPCODE_ACK = 4,
    TFTP_OPCODE_ERROR = 5,
    TFTP_OPCODE_OACK = 6  
}tftp_opcodes_t;


typedef enum
{
    TFTP_TRANSFER_STATE_IDLE = 0,
    TFTP_TRANSFER_STATE_SENDING = 1,
    TFTP_TRANSFER_STATE_RECEIVING = 2
}tftp_transferState_t;
typedef enum
{
    TFTP_TRANSFER_RESULT_PASS = 0,
    TFTP_TRANSFER_RESULT_FAIL = 1,
    TFTP_TRANSFER_RESULT_FAIL_FILE_NOT_FOUND = 2,    
    TFTP_TRANSFER_RESULT_FAIL_TIMEOUT = 3,
    TFTP_TRANSFER_RESULT_FAIL_IN_PROGRESS = 4
}tftp_transferResult_t;



typedef struct  
{
    char filename[255]; 
    tftp_transferType_t transferType; 
    struct sockaddr_in transferEndpoint;    
}tftp_transferParameters_t;

typedef struct  
{
    uint8_t* packet;
    uint16_t packetLength;
}tftp_receivedPacket_t;


void tftp_FileTransferTask(void *pvParameters);

#endif /* TFTP_FILETRANSFERCLIENT_H_ */