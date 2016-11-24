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
#define TFTP_FILE_BUFFER_SIZE 1024
#define TFTP_PACKET_TIMEOUT 5000 //5 seconds for now... TODO update this value

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
    TFTP_OPCODE_ERROR = 5 
}tftp_opcodes_t;


typedef enum
{
    TFTP_TRANSFER_STATE_IDLE = 0,
    TFTP_TRANSFER_STATE_SENDING = 1,
    TFTP_TRANSFER_STATE_RECEIVING = 2
}tftp_transferState_t;

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