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
#include "Board_Init.h"

#define FIRMWARE_LOCATION 0x00008000u + IFLASH0_ADDR
#define APP_START_ADDRESS 0x00408000u //in final it will go to 0x00408000
#define FIRMWARE_FILE_HEADER_BYTES 0xAA55AA55
#define FIRMWARE_TEMPORARY_LOCATION 0x00424000
#define FIRMWARE_IMAGE_NAME "0:firmware.bin"
#define CRCCU_TIMEOUT   0xFFFFFFFF
#define FIRMWARE_BUFFER_SIZE 512

extern void lowBatteryBlink();
extern FATFS fs;
COMPILER_ALIGNED (512)
crccu_dscr_type_t crc_dscr;
//static function foward declarations
static status_t initializeSDCard();
static void start_application(void);
static uint32_t compute_crc(uint8_t *p_buffer, uint32_t ul_length,
uint32_t ul_polynomial_type);
status_t __attribute__((optimize("O0"))) loadNewFirmware(char* filename);
static void errorBlink(); 
static void successBlink();

typedef struct 
{
	uint32_t fileHeaderBytes;
	uint32_t crc; 
	uint32_t length; 	
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
	status_t status = STATUS_PASS; 
	drv_gpio_initializeAll();
	pmc_enable_periph_clk(ID_CRCCU);   		    
	board_init();	
	drv_gpio_pin_state_t sw1State = DRV_GPIO_PIN_STATE_HIGH, sw2State = DRV_GPIO_PIN_STATE_HIGH, lboState = DRV_GPIO_PIN_STATE_HIGH; 	
	//check pins for seeing if the bootloader should be entered. 
	uint32_t enterBootloader = 0;
	drv_gpio_getPinState(DRV_GPIO_PIN_AC_SW1,&sw1State);
	drv_gpio_getPinState(DRV_GPIO_PIN_AC_SW2,&sw2State);
	drv_gpio_getPinState(DRV_GPIO_PIN_LBO, &lboState);	//Check for Low Battery
	int i = 0; 
	drv_gpio_setPinState(DRV_GPIO_PIN_GREEN_LED, DRV_GPIO_PIN_STATE_LOW);
	drv_gpio_setPinState(DRV_GPIO_PIN_BLUE_LED, DRV_GPIO_PIN_STATE_LOW); 
	if(sw1State == DRV_GPIO_PIN_STATE_LOW && sw2State == DRV_GPIO_PIN_STATE_LOW)
	{
		if (lboState == DRV_GPIO_PIN_STATE_LOW)
		{
			//battery is low, don't run bootloader
			lowBatteryBlink();
			enterBootloader = 0;
		}
		else
		{
			//make sure that both IO stay low for 1 second
			for(i=0;i<10;i++)
			{		
				enterBootloader = 1; //set the enter bootloader flag
				drv_gpio_getPinState(DRV_GPIO_PIN_AC_SW1,&sw1State);
				drv_gpio_getPinState(DRV_GPIO_PIN_AC_SW2,&sw2State);
				if(sw1State != DRV_GPIO_PIN_STATE_LOW || sw2State != DRV_GPIO_PIN_STATE_LOW)
				{
					enterBootloader = 0; 
				}
				delay_ms(100); 			
			}		
		}
	}	
	if(enterBootloader == 1)
	{
		//if bootloader conditions met initialize SD card and load firmware	
		drv_gpio_pin_state_t sdCardCD = DRV_GPIO_PIN_STATE_LOW; 
		//Check if SD card is installed
		drv_gpio_getPinState(DRV_GPIO_PIN_SD_CD,&sdCardCD);
		if(sdCardCD != DRV_GPIO_PIN_STATE_HIGH)
		{
			status = STATUS_FAIL; 
		}
		//Initialize the SD card only if the card was detected. 
		if(status == STATUS_PASS)
		{
			status = initializeSDCard(); 	
		}
		//load the new firmware only if the card was initialized. 		
		if(status == STATUS_PASS)
		{		
			//set the LED to purple during the firmware load
			drv_gpio_setPinState(DRV_GPIO_PIN_GREEN_LED, DRV_GPIO_PIN_STATE_HIGH);
			drv_gpio_setPinState(DRV_GPIO_PIN_BLUE_LED, DRV_GPIO_PIN_STATE_LOW); 
			drv_gpio_setPinState(DRV_GPIO_PIN_RED_LED, DRV_GPIO_PIN_STATE_LOW); 
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
/**
 * initializeSDCard(void)
 * @brief This function starts the application that resides at APP_START_ADDRESS
 *  
 */
static status_t initializeSDCard()
{
	static FRESULT res;
	status_t status = STATUS_PASS; 
	////Initialize SD card
	sd_mmc_init();
	//
	///* Wait card present and ready */
	//
	////we don't want the firmware to freeze if we don't have an SD card.
	int maxAttemptCount = 500; 
	do
	{
		res = sd_mmc_test_unit_ready(0);
		if (res == CTRL_FAIL)
		{
			while (CTRL_NO_PRESENT != sd_mmc_check(0))
			{
				maxAttemptCount--; 
				if(maxAttemptCount == 0)
				{
					status = STATUS_FAIL;
					break; 
				}
			}
		}
		maxAttemptCount--;
		if(maxAttemptCount == 0)
		{
			status = STATUS_FAIL;
			break;
		}
		delay_ms(10);
		
		
	} while (CTRL_GOOD != res);
		
		
	/*	Mount the SD card	*/
	memset(&fs, 0, sizeof(FATFS));
	res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
	if (res == FR_INVALID_DRIVE)
	{
		status = STATUS_FAIL;
	}
	return status;	
}
/*
 * compute_crc(void)
 * @brief computes the CRC using the built in CRCCU hardware on processor. 
 * @param uint8_t *p_buffer start address of memory that you want to calculate the CRC of. 
 * @param uint32_t ul_length how many bytes you want to calculate CRC for. 
 * @param uint32_t ul_polynomial_type The polynomial type, (CRCCU_MR_PTYPE_CCITT8023/CRCCU_MR_PTYPE_CASTAGNOLI/CRCCU_MR_PTYPE_CCITT16) 
 * @return uint32_t representing the CRC value. 
 */
static uint32_t compute_crc(uint8_t *p_buffer, uint32_t ul_length,
         uint32_t ul_polynomial_type)
{
	uint32_t ul_crc;
	uint32_t ul_timeout = 0;

	/* Reset the CRCCU */
	crccu_reset(CRCCU);

	memset((void *)&crc_dscr, 0, sizeof(crccu_dscr_type_t));

	crc_dscr.ul_tr_addr = (uint32_t) p_buffer;

	/* Transfer width: byte, interrupt enable */
	crc_dscr.ul_tr_ctrl =	CRCCU_TR_CTRL_TRWIDTH_WORD | (ul_length/4) |
		CRCCU_TR_CTRL_IEN_ENABLE;


	crccu_configure_descriptor(CRCCU, (uint32_t) &crc_dscr);

	/* Configure CRCCU mode */
	crccu_configure_mode(CRCCU, CRCCU_MR_ENABLE | ul_polynomial_type);

	/* Start the CRC calculation */
	crccu_enable_dma(CRCCU);

	/* Wait for calculation ready */
	while ((crccu_get_dma_status(CRCCU) == CRCCU_DMA_SR_DMASR) &&
			(ul_timeout++ < CRCCU_TIMEOUT)) 
	{
		
	}
	/* Get CRC value */
	ul_crc = crccu_read_crc_value(CRCCU);
	/* Display CRC */
	if (ul_polynomial_type == CRCCU_MR_PTYPE_CCITT16) 
	{
		/* 16-bits CRC */
		ul_crc &= 0xFFFF;
		//printf("  CRC of the buffer is 0x%04lu\n\r",
				//(uint32_t)ul_crc);
	} 
	else 
	{
		/* 32-bits CRC */
		//printf("  CRC of the buffer is 0x%08lu\n\r",
				//(uint32_t)ul_crc);
	}
     return ul_crc;
}

uint32_t fileCRC = 0; 
extern void efc_write_fmr(Efc *p_efc, uint32_t ul_fmr);
extern uint32_t efc_perform_fcr(Efc *p_efc, uint32_t ul_fcr);
/**
 * loadNewFirmware(char* filename)
 * @brief Load new firmware function. 
 * @return status_t returns STATUS_PASS if successful and STATUS_FAIL if there is a failure. 
 */
status_t __attribute__((optimize("O0"))) loadNewFirmware(char* filename)
{	
	status_t result = STATUS_PASS;
	FIL firmwareFileObj = {0};
	//initialize the suitNumber

	filename[0] = LUN_ID_SD_MMC_0_MEM + '0'; //is this necessary? 
	wdt_disable(WDT);
	
	FRESULT res = f_open(&firmwareFileObj, (char const *)filename, FA_OPEN_EXISTING | FA_READ);
	if (res != FR_OK)
	{
		return STATUS_FAIL;
	}
	//read the header from the file. 
	firmwareHeader_t header = {0}; 
	uint32_t bytes_read = 0, total_bytes_read = 0;	
	res = f_read(&firmwareFileObj, (void*)&header, sizeof(firmwareHeader_t), &bytes_read);
	//confirm that the file has a valid header, with the CRC bytes matching. 
	if(header.fileHeaderBytes != FIRMWARE_FILE_HEADER_BYTES ) //|| header.crc1 != header.crc2
	{
		//bad header, don't even try to load this crap!
		f_close(&firmwareFileObj); 
		return STATUS_FAIL; 
	}	
	uint32_t destAddress = FIRMWARE_TEMPORARY_LOCATION; //that where we are writing
	uint32_t retVal = 0;
	//initialize the memory
	retVal = flash_init(FLASH_ACCESS_MODE_128, 6); //	| EEFC_FMR_CLOE | EEFC_FMR_SCOD 
	int resultTest = 	efc_perform_fcr(EFC0,
	EEFC_FCR_FKEY_PASSWD | EEFC_FCR_FARG(0x0123) |
	0x07u);
	
	retVal = flash_unlock(destAddress,destAddress + firmwareFileObj.fsize ,NULL,NULL);			
	char buf[FIRMWARE_BUFFER_SIZE] = {0}; 	 
	char nullBuf[FIRMWARE_BUFFER_SIZE] = {0}; 		
		
	uint32_t i = 0, error = 0;
	//erase the program space first
	for(i=0x424000ul;i< 0x440000ul;i+=0x4000)
	{
		resultTest = flash_erase_page(i,IFLASH_ERASE_PAGES_32);
		if(resultTest != 0)
		{
			error++;
		}	
	}
	drv_gpio_setPinState(DRV_GPIO_PIN_RED_LED, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(DRV_GPIO_PIN_GREEN_LED, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(DRV_GPIO_PIN_BLUE_LED, DRV_GPIO_PIN_STATE_HIGH);			
	while(total_bytes_read < firmwareFileObj.fsize - sizeof(firmwareHeader_t) && res == FR_OK)
	{
		res = f_read(&firmwareFileObj, buf, FIRMWARE_BUFFER_SIZE, &bytes_read);
		if(bytes_read != 0)
		{		
			if(flash_write(destAddress+total_bytes_read, (void*)buf,bytes_read,0) != 0)
			{
				result = STATUS_FAIL;
				break;
			}
		}
		total_bytes_read += bytes_read; 
		if(total_bytes_read ==  firmwareFileObj.fsize - sizeof(firmwareHeader_t))
		{
			break; //this is redundant
		}
		drv_gpio_togglePin(DRV_GPIO_PIN_GREEN_LED); 
		delay_ms(100);
	}
	drv_gpio_setPinState(DRV_GPIO_PIN_BLUE_LED, DRV_GPIO_PIN_STATE_LOW);
	delay_ms(100);
	drv_gpio_setPinState(DRV_GPIO_PIN_BLUE_LED, DRV_GPIO_PIN_STATE_HIGH);
	delay_ms(100);	
	for(i=0x408000ul;i< 0x424000ul;i+=0x2000)
	{
		resultTest = flash_erase_page(i,IFLASH_ERASE_PAGES_16);
		if(resultTest != 0)
		{
			error++;
		}
	}	
	//verify firmware
	drv_gpio_setPinState(DRV_GPIO_PIN_GREEN_LED, DRV_GPIO_PIN_STATE_HIGH);
	drv_gpio_setPinState(DRV_GPIO_PIN_RED_LED, DRV_GPIO_PIN_STATE_LOW);
	delay_ms(100);
	drv_gpio_setPinState(DRV_GPIO_PIN_RED_LED, DRV_GPIO_PIN_STATE_HIGH);
	delay_ms(100);
	//TODO fix CRC problem. 
	//uint32_t ul_crc = compute_crc((uint8_t *)FIRMWARE_TEMPORARY_LOCATION, firmwareFileObj.fsize - sizeof(firmwareHeader_t),
	//CRCCU_MR_PTYPE_CASTAGNOLI);
	uint32_t ul_crc =0;
	fileCRC = ul_crc; 
	//if(header.crc == fileCRC)
	//{
		//CRC checks out, let copy the firmware to the final location
		total_bytes_read =0;
		destAddress = APP_START_ADDRESS;
		retVal = flash_unlock(destAddress,destAddress + firmwareFileObj.fsize ,NULL,NULL);
		while(total_bytes_read < firmwareFileObj.fsize - sizeof(firmwareHeader_t))
		{			
			memcpy(buf,FIRMWARE_TEMPORARY_LOCATION+total_bytes_read,FIRMWARE_BUFFER_SIZE); 			
			if(flash_write(destAddress+total_bytes_read, (void*)buf,FIRMWARE_BUFFER_SIZE,0) != 0)
			{
				result = STATUS_FAIL;
				break;
			}
			total_bytes_read += FIRMWARE_BUFFER_SIZE; 
			drv_gpio_togglePin(DRV_GPIO_PIN_BLUE_LED);
			delay_ms(100);
		}			
	//}
	//else
	//{
		//result = STATUS_FAIL; 
	//}
		//
	res = f_close(&firmwareFileObj);
	if (res != FR_OK)
	{
		return STATUS_FAIL;
	}
	return STATUS_PASS; 	
}


/**
 * errorBlink()
 * @brief blink led red 5 times to indicate error
 */
static void errorBlink()
{
	drv_gpio_setPinState(DRV_GPIO_PIN_RED_LED, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_GREEN_LED, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_BLUE_LED, DRV_GPIO_PIN_STATE_HIGH); 
	
	int i = 0;
	for(i=0; i<10; i++)
	{
		delay_ms(200); 
		drv_gpio_togglePin(DRV_GPIO_PIN_RED_LED); 	
	}	
}

/**
 * successBlink()
 * @brief blink led green 3 times if successful
 */
static void successBlink()
{
	drv_gpio_setPinState(DRV_GPIO_PIN_RED_LED, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_GREEN_LED, DRV_GPIO_PIN_STATE_HIGH); 
	drv_gpio_setPinState(DRV_GPIO_PIN_BLUE_LED, DRV_GPIO_PIN_STATE_HIGH); 
	
	int i = 0;
	for(i=0; i<6; i++)
	{
		delay_ms(200); 
		drv_gpio_togglePin(DRV_GPIO_PIN_GREEN_LED); 	
	}	
}