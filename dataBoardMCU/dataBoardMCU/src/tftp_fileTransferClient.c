/*
 * tftp_fileTransferClient.c
 *
 * Created: 11/22/2016 2:38:42 PM
 *  Author: sean
 */ 

#include "tftp_fileTransferClient.h"
#include "msg_messenger.h"
#include "net_wirelessNetwork.h"
#include "sdc_sdCard.h"

/* Global Variables */

/*	Static function forward declarations	*/
static void processMessage(msg_message_t* message);

/*	Local variables	*/
xQueueHandle msg_queue_tftpClient = NULL;
volatile uint8_t fileBufferA[TFTP_FILE_BUFFER_SIZE] = {0} , fileBufferB[TFTP_FILE_BUFFER_SIZE] = {0};

static sdc_file_t fileObject =
{
    .bufferIndexA = 0,
    .bufferIndexB = 0,
    .bufferPointerA = fileBufferA,
    .bufferPointerB = fileBufferB,
    .bufferSize = TFTP_FILE_BUFFER_SIZE,
    .fileObj = NULL,
    .fileName = "datalog",
    .fileOpen = false,
    .activeBuffer = 0,
    .sem_bufferAccess = NULL
};

static net_socketConfig_t transferSocket =
{
    .endpoint.sin_addr = 0xFFFFFFFF, //broadcast Address
    .endpoint.sin_family = AF_INET,
    .endpoint.sin_port = _htons(6669),
    .sourceModule = MODULE_TFTP_CLIENT,
    .socketId = -1,
    .clientSocketId = -1
};

static tftp_transferState_t transferState = TFTP_TRANSFER_STATE_IDLE; 
static tftp_transferType_t transferType = TFTP_TRANSFER_TYPE_GET_FILE; 
static uint16_t blockCount = 0;

/*	Extern functions	*/

/*	Extern variables	*/

/*	Function definitions	*/
void tftp_FileTransferTask(void *pvParameters)
{
    msg_message_t receivedMessage;
    
    msg_queue_tftpClient = xQueueCreate(10, sizeof(msg_message_t));
    if (msg_queue_tftpClient != NULL)
    {
        msg_registerForMessages(MODULE_TFTP_CLIENT, 0xff, msg_queue_tftpClient);
    }    
    while (1)
    {
        //receive from message queue and data queue, take necessary actions
        if (xQueueReceive(msg_queue_tftpClient, &receivedMessage, 50) == TRUE)
        {
            processMessage(&receivedMessage);
        }
        
    }
}

static void processMessage(msg_message_t* message)
{
    switch(message->msgType)
    {
        case MSG_TYPE_TFTP_INITIATE_TRANSFER:
        //check if we're already in a transfer, if so, return result failed
        
        
        break; 
        case MSG_TYPE_TFTP_PACKET_RECEIVED:
        
        break;
        default:
        break;
        
    }
}

static status_t startFileSend(tftp_transferParameters_t* transferParameters)
{
    status_t status = STATUS_PASS;
    //open the file, verify it exists
    
    //open the socket
    
    //set the block count to zero
    blockCount = 0;
    //send write request
    
    //set the transfer state to sending
    transferState = TFTP_TRANSFER_STATE_SENDING; 
    //call receiveFrom call
    
    
    return status; 
}