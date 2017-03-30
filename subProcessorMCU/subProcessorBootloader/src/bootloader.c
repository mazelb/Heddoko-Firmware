/*
 * bootloader.c
 *
 * Created: 11/9/2015 8:11:45 AM
 *  Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */
#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h> 
#include "drv_gpio.h"
#include "drv_uart.h"
#include "pkt_packetParser.h"
#include "nvm_nvMemInterface.h"
#include "pkt_packetCommandsList.h"
//#include "Board_Init.h"

//Bootloader has a max size of 0x5800 = 44*512 pages = 22528
//Max space on processor is 0x20000 = 256*512 pages = 131072
//Max code size is 0x1A800 = 212*512 pages = 108544

#define FIRMWARE_LOCATION 0x00005800u + IFLASH0_ADDR
#define APP_START_ADDRESS 0x00405800u 
#define APP_END_ADDRESS 0x00420000u 
#define FIRMWARE_FILE_HEADER_BYTES 0x55AA55AA
#define CRCCU_TIMEOUT   0xFFFFFFFF
#define FIRMWARE_BUFFER_SIZE 512

drv_uart_config_t uart1Config =
{
    .p_usart = UART1,
    .mem_index = 1,
    .uart_options =
    {
        .baudrate   = 460800,
        .charlength = CONF_CHARLENGTH,
        .paritytype = CONF_PARITY,
        .stopbits   = CONF_STOPBITS
    },
    .mode = DRV_UART_MODE_INTERRUPT
};

//FATFS fs;
COMPILER_ALIGNED (512)
//crccu_dscr_type_t crc_dscr;
//static function forward declarations
static void start_application(void);
//static uint32_t compute_crc(uint8_t *p_buffer, uint32_t ul_length,
//uint32_t ul_polynomial_type);
static void errorBlink(); 
static void successBlink();
static status_t getPacketTimed(drv_uart_config_t* uartConfig, pkt_rawPacket_t* packet, uint32_t maxTime);
static status_t processPacket(pkt_rawPacket_t *packet); 
static status_t processFirmwareDataBlock(pkt_rawPacket_t *packet);
static void sendBeginFlashProcessMessage();
static void prepareForNewFirmware(); 
static void sendFirmwareBlockAck(uint16_t blockNumber);

volatile uint32_t sgSysTickCount = 0; 
volatile uint16_t totalFirmwareBlocks = 0;
volatile uint16_t lastReceivedFirmwareBlocks = 0;

typedef struct 
{
	uint32_t fileHeaderBytes;
	uint32_t dbBinLength; 	
    uint32_t dbCRC; 
    uint32_t pbBinLength; 
    uint32_t pbCRC; 
}firmwareHeader_t;


/**
 * \brief Handler for System Tick interrupt.
 */

void SysTick_Handler(void)
{
	sgSysTickCount++;
}

/**
 * runBootloader(void)
 * @brief This is the bootloader program, it is run only when the binary is compiled in bootloader mode. 
 * It will copy a executable to a temporary location, then load it into the main program space. 
 * The bootloader program is always loaded onto a release board at location 0x00000000 and executes the main
 * program. 
 */
void __attribute__((optimize("O0"))) runBootloader()
{
	nvmSettings_t settings;
	
	nvm_readFromFlash(&settings);
    char c; 
    pkt_rawPacket_t packet; 
    status_t status = STATUS_PASS; 
	drv_gpio_pin_state_t pwrButtonState = DRV_GPIO_PIN_STATE_HIGH;  		    
	board_init();	
	uint32_t enterBootloader = 0;
	int i = 0; 
    int failCount = 0;
    uint32_t cpuClock = sysclk_get_cpu_hz(); 
    drv_gpio_initializeAll();
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_GREEN, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_BLUE, DRV_GPIO_PIN_STATE_LOW); 
    //check the nvm signature
    if(settings.validSignature == NVM_SETTINGS_NEW_FIRMWARE_FLAG)
    {
        enterBootloader = 1; 
    }    
	if(enterBootloader == 1)
	{
		drv_gpio_initializeAll();
        drv_uart_init(&uart1Config); 
        //enable the systick timer
        SysTick->LOAD = ( cpuClock/ 1000 ) - 1UL; //set the systick load to trigger every ms
        SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;	        
        //turn on the power for the data board       
        drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_LOW);
        delay_ms(1000);   
        drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_HIGH);
        //wait until we get a packet from the data board to know it is alive. 
        if(getPacketTimed(&uart1Config, &packet, 5000) == STATUS_PASS)
        {
            processPacket(&packet); 
        }  
        delay_ms(1000);       
 
        while(failCount < 5)
        {
            //send the enter bootloader command
            sendBeginFlashProcessMessage();
            if(getPacketTimed(&uart1Config, &packet, 5000) == STATUS_PASS)
            {
                if(packet.payload[1] == PACKET_COMMAND_ID_SUBP_BEGIN_FLASH_RESP)
                {
                    if(packet.payload[2] == STATUS_PASS)
                    {
                        //read the total firmware block count. 
                        totalFirmwareBlocks = (uint16_t)(packet.payload[4] << 8) + packet.payload[3];
                        prepareForNewFirmware();
                    }
                    else
                    {
                        //failed to begin firmware update
                        errorBlink();
                        //reset the processor, and try again from the start
                        rstc_start_software_reset(RSTC);                        
                    }
                    break;
                }
            }
            failCount++;   
            wdt_restart(WDT);       
        }
        if(status == STATUS_PASS)
        {
            //set the LED to purple during the firmware load
            drv_gpio_setPinState(DRV_GPIO_PIN_LED_GREEN, DRV_GPIO_PIN_STATE_HIGH);
            drv_gpio_setPinState(DRV_GPIO_PIN_LED_BLUE, DRV_GPIO_PIN_STATE_LOW);
            drv_gpio_setPinState(DRV_GPIO_PIN_LED_RED, DRV_GPIO_PIN_STATE_LOW);
        }      
        //if we have reached here it means we are starting the load process. 
        //send ACK block 0, to start the process. 
        lastReceivedFirmwareBlocks = 0;
        //send ack block zero to start the process
        sendFirmwareBlockAck(lastReceivedFirmwareBlocks);                  
		failCount = 0;
        while(1)
        {
            if(getPacketTimed(&uart1Config, &packet, 3000) == STATUS_PASS)
            {                
                if(processFirmwareDataBlock(&packet) == STATUS_PASS)
                {
                    failCount = 0;
                    sendFirmwareBlockAck(lastReceivedFirmwareBlocks);    
                    //we received all the blocks
                    if(lastReceivedFirmwareBlocks == totalFirmwareBlocks)
                    {
                        //write the flag to make the processor boot normally.
                        delay_ms(100); 
                        status = STATUS_PASS; 
                        break;
                    } 
                }
                else
                {
                   sendFirmwareBlockAck(lastReceivedFirmwareBlocks);  
                }
                drv_gpio_togglePin(DRV_GPIO_PIN_LED_BLUE);                
            }
            else
            {
                failCount++;
                if(failCount > 10)
                {
                    status = STATUS_FAIL;
                    break; 
                }
            }
            wdt_restart(WDT);            
        }
		if(status != STATUS_PASS)
		{
			//blink an error, we should still be able to start the firmware afterwards. 
			errorBlink(); 
		}
		else
		{
			successBlink();    
		}
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk; //disable the systick		
        drv_gpio_setPinState(DRV_GPIO_PIN_GPIO, DRV_GPIO_PIN_STATE_LOW);	
	} 	   
	start_application();	
}

/**
 * start_application(void)
 * @brief This function starts the application that resides at APP_START_ADDRESS
 *  
 */
static void start_application(void)
{
	uint32_t app_start_address =0;
	/* Rebase the Stack Pointer */
	__set_MSP(*(uint32_t *) APP_START_ADDRESS);
	/* Rebase the vector table base address */
	SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);
	/* Load the Reset Handler address of the application */
	app_start_address = *(uint32_t *)(APP_START_ADDRESS+4); 
	/* Jump to application Reset Handler in the application */
	asm("bx %0"::"r"(app_start_address));
}


///*
 //* compute_crc(void)
 //* @brief computes the CRC using the built in CRCCU hardware on processor. 
 //* @param uint8_t *p_buffer start address of memory that you want to calculate the CRC of. 
 //* @param uint32_t ul_length how many bytes you want to calculate CRC for. 
 //* @param uint32_t ul_polynomial_type The polynomial type, (CRCCU_MR_PTYPE_CCITT8023/CRCCU_MR_PTYPE_CASTAGNOLI/CRCCU_MR_PTYPE_CCITT16) 
 //* @return uint32_t representing the CRC value. 
 //*/
//static uint32_t compute_crc(uint8_t *p_buffer, uint32_t ul_length,
         //uint32_t ul_polynomial_type)
//{
	//uint32_t ul_crc;
	//uint32_t ul_timeout = 0;
//
	///* Reset the CRCCU */
	//crccu_reset(CRCCU);
//
	//memset((void *)&crc_dscr, 0, sizeof(crccu_dscr_type_t));
//
	//crc_dscr.ul_tr_addr = (uint32_t) p_buffer;
//
	///* Transfer width: byte, interrupt enable */
	//crc_dscr.ul_tr_ctrl =	CRCCU_TR_CTRL_TRWIDTH_WORD | (ul_length/4) |
		//CRCCU_TR_CTRL_IEN_ENABLE;
//
//
	//crccu_configure_descriptor(CRCCU, (uint32_t) &crc_dscr);
//
	///* Configure CRCCU mode */
	//crccu_configure_mode(CRCCU, CRCCU_MR_ENABLE | ul_polynomial_type);
//
	///* Start the CRC calculation */
	//crccu_enable_dma(CRCCU);
//
	///* Wait for calculation ready */
	//while ((crccu_get_dma_status(CRCCU) == CRCCU_DMA_SR_DMASR) &&
			//(ul_timeout++ < CRCCU_TIMEOUT)) 
	//{
		//
	//}
	///* Get CRC value */
	//ul_crc = crccu_read_crc_value(CRCCU);
	///* Display CRC */
	//if (ul_polynomial_type == CRCCU_MR_PTYPE_CCITT16) 
	//{
		///* 16-bits CRC */
		//ul_crc &= 0xFFFF;
		////printf("  CRC of the buffer is 0x%04lu\n\r",
				////(uint32_t)ul_crc);
	//} 
	//else 
	//{
		///* 32-bits CRC */
		////printf("  CRC of the buffer is 0x%08lu\n\r",
				////(uint32_t)ul_crc);
	//}
     //return ul_crc;
//}

uint32_t fileCRC = 0; 
extern void efc_write_fmr(Efc *p_efc, uint32_t ul_fmr);
extern uint32_t efc_perform_fcr(Efc *p_efc, uint32_t ul_fcr);


/**
 * errorBlink()
 * @brief blink led red 5 times to indicate error
 */
static void errorBlink()
{
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_RED, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_GREEN, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_BLUE, DRV_GPIO_PIN_STATE_HIGH); 	
	int i = 0;
	for(i=0; i<10; i++)
	{
		delay_ms(200); 
		drv_gpio_togglePin(DRV_GPIO_PIN_LED_RED); 	
	}	
}

/**
 * successBlink()
 * @brief blink led green 3 times if successful
 */
static void successBlink()
{
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_RED, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_GREEN, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_BLUE, DRV_GPIO_PIN_STATE_HIGH); 
	
	int i = 0;
	for(i=0; i<6; i++)
	{
		delay_ms(200); 
		drv_gpio_togglePin(DRV_GPIO_PIN_LED_GREEN); 	
	}	
}


static status_t getPacketTimed(drv_uart_config_t* uartConfig, pkt_rawPacket_t* packet, uint32_t maxTime)
{
    status_t result = STATUS_PASS;
    char val;
    int pointer = 0;
    uint32_t startTime = sgSysTickCount;
    while(1)
    {
        result = drv_uart_getChar(uartConfig,&val);
        if(result == STATUS_PASS)
        {
            //process the byte as it comes in
            if(pkt_processIncomingByte(packet,val) == STATUS_PASS)
            {
                //the packet is complete
                result = STATUS_PASS;
                break;
            }
        }
        else
        {
            //check if we've timed out yet...
            if(sgSysTickCount > (startTime + maxTime))
            {
                //return fail, we've timed out.
                result = STATUS_FAIL;
                break;
            }
            delay_ms(1);  
            wdt_restart(WDT);              
        }
    }
    return result;
}

static status_t processPacket(pkt_rawPacket_t *packet)
{
    status_t status = STATUS_PASS;
    if(packet->payload[0] == PACKET_TYPE_SUB_PROCESSOR)
    {
        switch(packet->payload[1])
        {
            case PACKET_COMMAND_ID_SUBP_BEGIN_FLASH_RESP:
                
            break;
            case PACKET_COMMAND_ID_SUBP_FLASH_DATA_BLOCK:
            
            break;
        }
    }
    return status;     
}

static status_t processFirmwareDataBlock(pkt_rawPacket_t *packet)
{
    status_t status = STATUS_PASS;
    uint16_t packetNumber = 0;
    uint32_t destAddress = 0; 
    //confirm the packet type
    if(packet->payload[1] == PACKET_COMMAND_ID_SUBP_FLASH_DATA_BLOCK)
    {
        packetNumber = (uint16_t)(packet->payload[3] << 8) + packet->payload[2];   
        destAddress = APP_START_ADDRESS + (packetNumber-1)*FIRMWARE_BUFFER_SIZE;  
        if(flash_write(destAddress, (void*)(packet->payload+4),packet->payloadSize-4,0) == 0)
        {
            status = STATUS_PASS;
            lastReceivedFirmwareBlocks = packetNumber; 
        }
        else
        {
            status = STATUS_FAIL;
        }
    }
    else
    {
        status = STATUS_FAIL;
    }   
    return status; 
    
}



static void sendBeginFlashProcessMessage()
{
    uint8_t packetData[6] = {0}; 
    packetData[0] = PACKET_TYPE_SUB_PROCESSOR;
    packetData[1] = PACKET_COMMAND_ID_SUBP_BEGIN_FLASH_PROCESS;
    packetData[2] = 0x55;
    packetData[3] = 0xAA;
    packetData[4] = 0x55;
    packetData[5] = 0xAA;
    pkt_sendRawPacket(&uart1Config, packetData, 6);
}

static void prepareForNewFirmware()
{
    //once this function is called there is no going back
    //the previous version of firmware is erased.
    unsigned long i = 0, retVal = 0, error;
    //unlock the memory location
    retVal = flash_unlock(APP_START_ADDRESS,APP_END_ADDRESS,NULL,NULL);	
    
    for(i=APP_START_ADDRESS;i< APP_END_ADDRESS;i+=0x200)
    {
        retVal = flash_erase_page(i,IFLASH_ERASE_PAGES_4);
        if(retVal != 0)
        {
            error++;
        }
    }
}

static void sendFirmwareBlockAck(uint16_t blockNumber)
{
    uint8_t packetData[4] = {0};
    packetData[0] = PACKET_TYPE_SUB_PROCESSOR;
    packetData[1] = PACKET_COMMAND_ID_SUBP_FLASH_DATA_BLOCK_ACK;
    packetData[2] = (uint8_t)(blockNumber & 0xFF);
    packetData[3] = (uint8_t)((blockNumber >> 8) & 0xFF);
    pkt_sendRawPacket(&uart1Config, packetData, 4);
}