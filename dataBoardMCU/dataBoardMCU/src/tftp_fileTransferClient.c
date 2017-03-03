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
#include "dbg_debugManager.h"
#include "drv_gpio.h"

/* Global Variables */


/*	Static function forward declarations	*/
static void processMessage(msg_message_t* message);
void tftpPacketTimeoutCallback( xTimerHandle xTimer );
static void tftpReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength); 
static void tftpSocketStatusCallback(SOCKET socketId, net_socketStatus_t status);
static status_t sendRequestPacket(net_socketConfig_t* socket, tftp_opcodes_t opCode,char* filename);
static status_t sendAckPacket(net_socketConfig_t* socket, uint16_t blockNumber);
static void processPacket(uint8_t* buffer, uint32_t length);
static status_t startFileTransfer(tftp_transferParameters_t* transferParameters);
static void endFileTransfer(tftp_transferResult_t result);
static void sendBlock(uint16_t blockNumber);
static void sendWindowedBlocks(uint32_t blockNumber);
/*	Local variables	*/
xQueueHandle msg_queue_tftpClient = NULL;
volatile uint8_t fileBufferA[128] = {0} , fileBufferB[128] = {0};


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
    .clientSocketId = -1,
    .socketDataReceivedCallback = tftpReceivedDataCallback,
    .socketStatusCallback = tftpSocketStatusCallback,
    .socketType = SOCK_DGRAM
};

static tftp_transferState_t transferState = TFTP_TRANSFER_STATE_IDLE; 
static tftp_transferType_t transferType = TFTP_TRANSFER_TYPE_GET_FILE; 
static uint32_t blockCount = 0;
static uint32_t expectedAckedBlock = 0;
static uint16_t blockRetryCount = 0; 
static uint32_t lastReceiveAckedBlock = 0;
xTimerHandle tftpTimeoutTimer;	//timer handle for the led timer task
tftp_receivedPacket_t receivedPacket; //TODO possibly dynamically allocate this value as needed. 
volatile uint8_t packetBuffer[TFTP_FILE_BUFFER_SIZE]; 
static modules_t transferInitiator = MODULE_DEBUG; 
volatile uint8_t FileReadBuffer[TFTP_WINDOW_SIZE*TFTP_BLOCK_SIZE] = {0}; 
    
/*	Extern functions	*/

/*	Extern variables	*/

/*	Function definitions	*/
void tftp_FileTransferTask(void *pvParameters)
{
    msg_message_t receivedMessage;

    msg_queue_tftpClient = xQueueCreate(10, sizeof(msg_message_t));
    tftpTimeoutTimer = xTimerCreate("tftpTmr", (TFTP_PACKET_TIMEOUT/portTICK_RATE_MS), pdFALSE, NULL, tftpPacketTimeoutCallback);
    if (msg_queue_tftpClient != NULL)
    {
        msg_registerForMessages(MODULE_TFTP_CLIENT, 0xff, msg_queue_tftpClient);
    }    
    while (1)
    {
        //receive from message queue and data queue, take necessary actions
        if (xQueueReceive(msg_queue_tftpClient, &receivedMessage, 10) == TRUE)
        {
            processMessage(&receivedMessage);
        }
             
        net_processWifiEvents(5); //only wait 5ms max.   
        vTaskDelay(1);
    }
}

/**
 * @brief: Timer callback function.Used for detecting packet timeouts
 **/
void tftpPacketTimeoutCallback( xTimerHandle xTimer )
{
    //so the message timed out. 
    msg_sendMessage(MODULE_TFTP_CLIENT,MODULE_TFTP_CLIENT, MSG_TYPE_TFTP_PACKET_TIMEOUT, NULL); 
}

static void tftpReceivedDataCallback(SOCKET socketId, uint8_t* buf, uint16_t bufLength)
{
    int membufferIndex = 0;
    //queue up the packet. 
    receivedPacket.packet = buf; 
    receivedPacket.packetLength = bufLength; 
    
    if(msg_sendMessage(MODULE_TFTP_CLIENT,MODULE_TFTP_CLIENT, MSG_TYPE_TFTP_PACKET_RECEIVED, &receivedPacket) != STATUS_PASS)
    {
        dbg_printf(DBG_LOG_LEVEL_DEBUG,"Failed to enqueue message\r\n"); 
    }
    //setup the receive for the next packet. 
    //net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0); 
    //processPacket(buf, bufLength);
    
}
static void tftpSocketStatusCallback(SOCKET socketId, net_socketStatus_t status)
{
    switch (status)
    {
        case NET_SOCKET_STATUS_RECEIVE_FROM_FAILED:
        dbg_printString(DBG_LOG_LEVEL_VERBOSE, "Receive Failed\r\n");
        //we'll try again
        //net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0); 
        msg_sendMessage(MODULE_TFTP_CLIENT, MODULE_TFTP_CLIENT, MSG_TYPE_TFTP_PACKET_TIMEOUT, NULL); 
        break;

    }    
}
static void processMessage(msg_message_t* message)
{
    tftp_transferParameters_t* transferParams;
    tftp_receivedPacket_t* rcvdPacket; 
    switch(message->msgType)
    {
        case MSG_TYPE_TFTP_INITIATE_TRANSFER:
        //check if we're already in a transfer, if so, return result failed
        if(transferState != TFTP_TRANSFER_STATE_IDLE)
        {
            msg_sendMessageSimple(message->source, MODULE_TFTP_CLIENT, MSG_TYPE_TFTP_TRANSFER_RESULT, TFTP_TRANSFER_RESULT_FAIL_IN_PROGRESS);             
            return; 
        }
        transferParams = (tftp_transferParameters_t*)(message->parameters); 
        //set the transfer initiator source, so we can send a message back when the transfer is complete. 
        transferInitiator = message->source;         

        if(startFileTransfer(transferParams) != STATUS_PASS)
        {
            endFileTransfer(TFTP_TRANSFER_RESULT_FAIL);
        }

        break; 
        case MSG_TYPE_TFTP_PACKET_RECEIVED:
        {
            rcvdPacket = (tftp_receivedPacket_t*)message->parameters; 
            processPacket(rcvdPacket->packet, rcvdPacket->packetLength);
        }        
        break;
        case MSG_TYPE_TFTP_PACKET_TIMEOUT:
            
            if(transferState == TFTP_TRANSFER_STATE_SENDING)
            {
                //try to send the packet again
                if(blockRetryCount > TFTP_MAX_BLOCK_RETRY)
                {
                    endFileTransfer(TFTP_TRANSFER_RESULT_FAIL_TIMEOUT);
                    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Max retry reached!\r\n");    
                    blockRetryCount = 0;                
                }
                else
                {
                    if(expectedAckedBlock == 0)
                    {
                        //resend the start transfer packet
                        sendRequestPacket(&transferSocket, TFTP_OPCODE_WRQ, fileObject.fileName); 
                    }
                    else                    
                    {
                        sendWindowedBlocks(lastReceiveAckedBlock+1);    
                    }
                    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Timeout!\r\n");   
                    xTimerReset(tftpTimeoutTimer,TFTP_PACKET_TIMEOUT/portTICK_RATE_MS);                     
                    blockRetryCount++;
                }
            }
            else
            {
                if(blockRetryCount++ > TFTP_MAX_BLOCK_RETRY)
                {
                    endFileTransfer(TFTP_TRANSFER_RESULT_FAIL_TIMEOUT);
                    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Max retry reached!\r\n");
                    blockRetryCount = 0;
                }
                else
                {
                    xTimerReset(tftpTimeoutTimer,TFTP_PACKET_TIMEOUT/portTICK_RATE_MS);    
                }
               
            }
            
        break; 
        default:
        break;
        
    }
}

static void processPacket(uint8_t* buffer, uint32_t length)
{
    
    uint16_t opcode = buffer[1] + (buffer[0] << 8); 
    uint32_t blockNumber = 0;
    size_t offset = 0;
    size_t blockLength = 0;
    
    if(opcode == TFTP_OPCODE_ACK)
    {
        blockNumber = buffer[3] + (buffer[2] << 8);
        //add the first bytes onto the expected block count. 
        
        blockNumber |= (expectedAckedBlock & 0xFFFF0000); 
        //if(expectedAckedBlock + 1000 < blockNumber)
        //{
           //dbg_printf(DBG_LOG_LEVEL_DEBUG, "don't know what to do here...\r\n");              
        //}
        if(blockNumber == blockCount)
        {
            //all the packets have been ACKED
            dbg_printf(DBG_LOG_LEVEL_DEBUG, "Transfer complete\r\n");  
            endFileTransfer(TFTP_TRANSFER_RESULT_PASS);   
            //if the timeout timer is active, turn it off since we are done!. 
            if (xTimerIsTimerActive(tftpTimeoutTimer) != pdFALSE)	
            {
                xTimerStop(tftpTimeoutTimer, 0);
            }              
            return;             
        }
        if(blockNumber < expectedAckedBlock)
        {
            //get ready to receive the response
            net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0); 
            dbg_printf(DBG_LOG_LEVEL_DEBUG, "Received Duplicate ACK!: %d expected: %d\r\n",blockNumber,expectedAckedBlock);   
            lastReceiveAckedBlock = blockNumber;
            //don't reset the timer here... we will reset it when we retry. 
            //xTimerReset(tftpTimeoutTimer,TFTP_PACKET_TIMEOUT/portTICK_RATE_MS);   
            return;
        }
        
        lastReceiveAckedBlock = blockNumber;
        blockRetryCount = 0;
        blockNumber++;     
        sendWindowedBlocks(blockNumber);   
        xTimerReset(tftpTimeoutTimer,TFTP_PACKET_TIMEOUT/portTICK_RATE_MS);   
                
    }
    else if(opcode == TFTP_OPCODE_DATA)
    {
        blockNumber = buffer[3] + (buffer[2] << 8);
        offset = TFTP_BLOCK_SIZE * (blockNumber-1); //blocks start at 1, so subtract 1
        blockLength = length - 4;
        //only ACK the block if we wrote it into the file. 
        if(sdc_writeDirectToFile(&fileObject,buffer + 4, offset, blockLength) == STATUS_PASS)
        {
            //get ready to receive the next packet
            net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0); 
            //send the ACK if it was successful
            sendAckPacket(&transferSocket, blockNumber); 
            lastReceiveAckedBlock = blockNumber; 
            vTaskDelay(1);    
            if(blockLength < TFTP_BLOCK_SIZE)
            {
                //this is the last block! yay we did it!
                dbg_printf(DBG_LOG_LEVEL_DEBUG, "Receive complete!\r\n");
                vTaskDelay(500); //wait for the last ACK to go out. 
                endFileTransfer(TFTP_TRANSFER_RESULT_PASS);
            }
        }
        xTimerReset(tftpTimeoutTimer,TFTP_PACKET_TIMEOUT/portTICK_RATE_MS);                 
    }
    else if(opcode == TFTP_OPCODE_ERROR)
    {
        //if the timeout timer is active, turn it off since we received the error packet.
        if (xTimerIsTimerActive(tftpTimeoutTimer) != pdFALSE)
        {
            xTimerStop(tftpTimeoutTimer, 0);
        }
        dbg_printf(DBG_LOG_LEVEL_DEBUG, "Received Error Packet\r\n");    
        endFileTransfer(TFTP_TRANSFER_RESULT_FAIL);
    }  
    else if(opcode == TFTP_OPCODE_OACK)
    {
        //option ACK, we only get this after the first request is sent, so send out the first packet
        blockNumber = 0;
        blockRetryCount = 0;
        blockNumber++;
        if(transferType == TFTP_TRANSFER_TYPE_SEND_FILE)
        {
            sendWindowedBlocks(blockNumber);         
        }
        else
        {
            net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0); 
            sendAckPacket(&transferSocket, 0); 
            lastReceiveAckedBlock = 0;
        }        
        xTimerReset(tftpTimeoutTimer,TFTP_PACKET_TIMEOUT/portTICK_RATE_MS);        
    } 
         
     
}

static status_t startFileTransfer(tftp_transferParameters_t* transferParameters)
{
    status_t status = STATUS_PASS;
    //copy over the parameters
    transferSocket.endpoint.sin_addr.s_addr = transferParameters->transferEndpoint.sin_addr.s_addr; 
    transferSocket.endpoint.sin_port = transferParameters->transferEndpoint.sin_port; 
    //open the file, verify it exists
    strcpy(fileObject.fileName,transferParameters->filename); 
    if(transferParameters->transferType == TFTP_TRANSFER_TYPE_SEND_FILE)
    {
        //if we're sending the file, open it as read only
        status = sdc_openFile(&fileObject, transferParameters->filename, SDC_FILE_OPEN_READ_ONLY); 
    }
    else
    {
        //if we're reading a file, then open it as write/overwrite
        status = sdc_openFile(&fileObject, transferParameters->filename, SDC_FILE_OPEN_READ_WRITE_NEW); 
    }
    
    if(status != STATUS_PASS)
    {
        dbg_printString(DBG_LOG_LEVEL_DEBUG, "Failed to open file\r\n"); 
        return status; 
    }    
    //open the socket
    status = net_createUdpSocket(&transferSocket, 1400);	
    //bind the socket to a known port
    status|= net_bindUpdSocket(&transferSocket, 2323); 
    //wait for the bind
    vTaskDelay(500);
    dbg_printf(DBG_LOG_LEVEL_DEBUG, "Bind Should be done: %d\r\n",status); 
    //call receiveFrom
    
    if(transferParameters->transferType == TFTP_TRANSFER_TYPE_SEND_FILE)
    {
        //set the total block count to the number of blocks that need to be transfered.
        blockCount = (fileObject.fileObj.fsize / TFTP_BLOCK_SIZE) + 1;
        expectedAckedBlock = 0; 
        lastReceiveAckedBlock = 0;
        //send write request
        status |= sendRequestPacket(&transferSocket, TFTP_OPCODE_WRQ, transferParameters->filename);       
        if(status == STATUS_PASS)
        {
            //set the transfer state to sending
            transferState = TFTP_TRANSFER_STATE_SENDING;
        } 
        
    }
    else
    {
        //send read request
        status |= sendRequestPacket(&transferSocket, TFTP_OPCODE_RRQ, transferParameters->filename);   
        if(status == STATUS_PASS)
        {
            //set the transfer state to sending
            transferState = TFTP_TRANSFER_STATE_RECEIVING;
        }     
    }
    xTimerReset(tftpTimeoutTimer,TFTP_PACKET_TIMEOUT/portTICK_RATE_MS); 

    //now, wait for ack    
    return status; 
}


static status_t sendRequestPacket(net_socketConfig_t* socket, tftp_opcodes_t opCode,char* filename)
{
    //static uint8_t buffer[512] = {0};
    static char mode[] = "octet";
    static char blkSizeOption[] = "blksize";
    static char blkSizeString[] = TFTP_BLOCK_SIZE_STRING; 
    static char windowSizeOption[] = "windowsize";
    static char windowSizeString[] = TFTP_WINDOW_SIZE_STRING;
    
    packetBuffer[0] = 0;
    packetBuffer[1] = (uint8_t)opCode;
    int offset = 2;
    int filenameLength = strlen(filename);
    memcpy(packetBuffer+offset,filename,filenameLength+1); //copy over the filename with the null
    offset += filenameLength +1; 
    memcpy(packetBuffer+offset,mode,6); //add the mode string along with the 
    offset += 6;
    memcpy(packetBuffer+offset,blkSizeOption,8); //add the block size option code
    offset += 8;
    memcpy(packetBuffer+offset,blkSizeString,5); //add a string of the desired block size
    offset += 5; 
    if(opCode == TFTP_OPCODE_WRQ)
    {    
        memcpy(packetBuffer+offset,windowSizeOption,11); //add the block size option code
        offset += 11;
        memcpy(packetBuffer+offset,windowSizeString,2); //add a string of the desired block size
        offset += 3; 
    }    
    net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0); 
    return net_sendUdpPacket(socket,packetBuffer, offset);         
}

static status_t sendAckPacket(net_socketConfig_t* socket, uint16_t blockNumber)
{
    packetBuffer[0] = 0;
    packetBuffer[1] = (uint8_t)TFTP_OPCODE_ACK;
    packetBuffer[2] = blockNumber >> 8;
    packetBuffer[3] = blockNumber & 0xFF;
    //dbg_printf(DBG_LOG_LEVEL_VERBOSE,"Sent Ack for block:%d\r\n",blockNumber); 
    return net_sendUdpPacket(socket,packetBuffer, 4);
}

static void endFileTransfer(tftp_transferResult_t result)
{
    msg_sendMessageSimple(transferInitiator, MODULE_TFTP_CLIENT, MSG_TYPE_TFTP_TRANSFER_RESULT, result);   
    //close the socket
    net_closeSocket(&transferSocket);     
    //make sure the file is closed
    sdc_closeFile(&fileObject);   
    //set the transfer state back to idle. 
    transferState = TFTP_TRANSFER_STATE_IDLE;      
    xTimerStop(tftpTimeoutTimer, 100); 
}

static void sendBlock(uint16_t blockNumber)
{
    size_t offset = 0;
    size_t blockLength = 0;
    offset = TFTP_BLOCK_SIZE * (blockNumber -1); //increment the block number since it's +1 for the transfer
    blockLength = TFTP_BLOCK_SIZE;
    //check if this is the last block, if so, modify the transfer size.
    if((offset+blockLength) > fileObject.fileObj.fsize)
    {
        blockLength = fileObject.fileObj.fsize - offset ;
    }
    packetBuffer[0] = 0;
    packetBuffer[1] = TFTP_OPCODE_DATA;
    packetBuffer[2] = blockNumber >> 8;
    packetBuffer[3] = blockNumber & 0xFF;
    int i = 0;
    if(sdc_readFromFile(&fileObject,packetBuffer + 4, offset, blockLength) == STATUS_PASS)
    {        
        drv_gpio_setPinState(DRV_GPIO_PIN_AUX_GPIO, DRV_GPIO_PIN_STATE_HIGH);
        //send the data packet        
        for(i = 0; i < 8 ; i++)
        {
            packetBuffer[2] = blockNumber >> 8;
            packetBuffer[3] = blockNumber & 0xFF;
            if(net_sendUdpPacket(&transferSocket,packetBuffer, blockLength + 4) != STATUS_PASS)
            {
                //if we couldn't send it, wait and try again. 
                vTaskDelay(4);
                net_sendUdpPacket(&transferSocket,packetBuffer, blockLength + 4);
            }
            expectedAckedBlock = blockNumber++;    
        }
        drv_gpio_setPinState(DRV_GPIO_PIN_AUX_GPIO, DRV_GPIO_PIN_STATE_LOW);
        //get ready to receive the response
        //net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0);        
    }
    else
    {
        //error!
        dbg_printString(DBG_LOG_LEVEL_DEBUG, "Failed to read from file\r\n");
    }
}

static void sendWindowedBlocks(uint32_t blockNumber)
{
    size_t offset = 0;
    size_t blockLength = TFTP_BLOCK_SIZE;
    uint16_t lastBlockLength = TFTP_BLOCK_SIZE;
    uint16_t numberOfBlocks = TFTP_WINDOW_SIZE;
    offset = TFTP_BLOCK_SIZE * (blockNumber -1); //decrement the block number since it's +1 for the transfer
    size_t readBufferSize = blockLength*numberOfBlocks;  
    
    
    //check if this is the last set of blocks, if so, modify the transfer size.
    if(offset+(blockLength*numberOfBlocks) > fileObject.fileObj.fsize)
    {
        //calculate the number of blocks left
        numberOfBlocks = ((fileObject.fileObj.fsize - offset) / blockLength);
        //calculate last block length
        lastBlockLength = ((fileObject.fileObj.fsize - offset) % blockLength);
        //calculate how much data we must read
        readBufferSize = numberOfBlocks*blockLength + lastBlockLength;
        numberOfBlocks++; //increment the number of blocks to take into account the last incomplete block. 
    }
    packetBuffer[0] = 0;
    packetBuffer[1] = TFTP_OPCODE_DATA;
    packetBuffer[2] = blockNumber >> 8;
    packetBuffer[3] = blockNumber & 0xFF;
    int i = 0;
    drv_gpio_setPinState(DRV_GPIO_PIN_AUX_GPIO, DRV_GPIO_PIN_STATE_HIGH);
    
    if(sdc_readFromFile(&fileObject,FileReadBuffer, offset, readBufferSize) == STATUS_PASS)
    {               
        net_receiveUdpPacket(&transferSocket,transferSocket.buffer, transferSocket.bufferLength, 0);       
        //send the data packet        
        for(i = 0; i < numberOfBlocks ; i++)
        {            
            packetBuffer[2] = blockNumber >> 8;
            packetBuffer[3] = blockNumber & 0xFF;
            //check if it's the last block
            if(i+1 == numberOfBlocks)
            {
                //if it's the last block send the packet with the proper length
                memcpy(packetBuffer+4,(FileReadBuffer+(i*blockLength)),lastBlockLength);
                net_sendUdpPacket(&transferSocket,packetBuffer, lastBlockLength + 4);                     
            }
            else
            {
                memcpy(packetBuffer+4,(FileReadBuffer+(i*blockLength)),blockLength);
                net_sendUdpPacket(&transferSocket,packetBuffer, blockLength + 4);                      
            }                 
            expectedAckedBlock =  blockNumber++;                
        }
        //blockNumber; 
        //get ready to receive the response
        //vTaskDelay(1); 
            
    }
    else
    {
        //error!
        dbg_printString(DBG_LOG_LEVEL_DEBUG, "Failed to read from file\r\n");
    }
    drv_gpio_setPinState(DRV_GPIO_PIN_AUX_GPIO, DRV_GPIO_PIN_STATE_LOW);
}