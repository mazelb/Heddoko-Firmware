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
#include "drv_led.h"
//#include "nvm_nvMemInterface.h"
//#include "Board_Init.h"

#define FIRMWARE_LOCATION 0x00010000u + IFLASH0_ADDR
#define APP_START_ADDRESS 0x00410000u //in final it will go to 0x00410000
#define FIRMWARE_FILE_HEADER_BYTES 0x55AA55AA
#define FIRMWARE_TEMPORARY_LOCATION 0x00488000
#define FIRMWARE_IMAGE_NAME "0:firmware.bin"
#define CRCCU_TIMEOUT   0xFFFFFFFF
#define FIRMWARE_BUFFER_SIZE 512

//FATFS fs;
COMPILER_ALIGNED (512)
//crccu_dscr_type_t crc_dscr;
//static function forward declarations
static status_t initializeSDCard();
static void start_application(void);
//static uint32_t compute_crc(uint8_t *p_buffer, uint32_t ul_length,
//uint32_t ul_polynomial_type);
status_t __attribute__((optimize("O0"))) loadNewFirmware(char* filename);
static void errorBlink(); 
static void successBlink();

typedef struct 
{
	uint32_t fileHeaderBytes;
	uint32_t dbBinLength; 	
    uint32_t dbCRC; 
    uint32_t pbBinLength; 
    uint32_t pbCRC; 
}firmwareHeader_t;

/**
 * runBootloader(void)
 * @brief This is the bootloader program, it is run only when the binary is compiled in bootloader mode. 
 * It will copy a executable to a temporary location, then load it into the main program space. 
 * The bootloader program is always loaded onto a release board at location 0x00000000 and executes the main
 * program. 
 */
void runBootloader()
{
	//nvmSettings_t settings;
    //
    //nvm_readFromFlash(&settings); 
    
    status_t status = STATUS_PASS; 
	drv_gpio_initializeAll();
	//pmc_enable_periph_clk(ID_CRCCU);   		    
	board_init();	
    
	drv_gpio_pin_state_t sw1State = DRV_GPIO_PIN_STATE_HIGH, sw2State = DRV_GPIO_PIN_STATE_HIGH, lboState = DRV_GPIO_PIN_STATE_HIGH; 	
	//check pins for seeing if the bootloader should be entered. 
	uint32_t enterBootloader = 0;
	//drv_gpio_getPinState(DRV_GPIO_PIN_AC_SW1,&sw1State);
	//drv_gpio_getPinState(DRV_GPIO_PIN_AC_SW2,&sw2State);
	int i = 0; 
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_GREEN, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_LED_BLUE, DRV_GPIO_PIN_STATE_LOW); 

	if(enterBootloader == 1)
	{
		//load the new firmware only if the card was initialized. 		
		if(status == STATUS_PASS)
		{		
			//set the LED to purple during the firmware load
			drv_gpio_setPinState(DRV_GPIO_PIN_LED_GREEN, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(DRV_GPIO_PIN_LED_BLUE, DRV_GPIO_PIN_STATE_LOW); 
			drv_gpio_setPinState(DRV_GPIO_PIN_LED_RED, DRV_GPIO_PIN_STATE_LOW); 
			status = loadNewFirmware(FIRMWARE_IMAGE_NAME);						
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
		//unmount the drive
		//f_mount(LUN_ID_SD_MMC_0_MEM, NULL);		
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
	uint32_t app_start_address;
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