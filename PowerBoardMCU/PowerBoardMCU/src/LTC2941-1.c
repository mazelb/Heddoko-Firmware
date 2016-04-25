/*
 * LTC2941_1.c
 *
 * Created: 2016-03-23 4:57:01 PM
 *  Author: Hriday Mehta
 */ 

#include "LTC2941-1.h"
#include "common.h"

#define CHARGE_EMPTY_VALUE 0x4B03
status_t ltc2941Init(slave_twi_config_t* slaveConfig)
{
	status_t status = STATUS_FAIL;
	
	status = drv_i2c_write(slaveConfig, LTC_2941_CONTROL_REG_ADDR, 0xf0);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
	
	//Write the high threshold registers
	status = drv_i2c_write(slaveConfig, LTC_2941_CHRG_THRESHOLD_HIGH_REG_ADDR, 0xff);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
	status = drv_i2c_write(slaveConfig, LTC_2941_CHRG_THRESHOLD_HIGH_REG_ADDR + 1, 0xff);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
	
	//Write the low threshold values
	status = drv_i2c_write(slaveConfig, LTC_2941_CHRG_THRESHOLD_LOW_REG_ADDR, 0x48);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
	status = drv_i2c_write(slaveConfig, LTC_2941_CHRG_THRESHOLD_LOW_REG_ADDR + 1, 0x2c);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
}
status_t ltc2941SetCharge(slave_twi_config_t* slaveConfig, uint16_t chargeLevel)
{
	status_t status = STATUS_FAIL;
	
	//Set the Accumulated charge register to indicate battery full
	status = drv_i2c_write(slaveConfig, 0x03, chargeLevel&0xFF);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
	status = drv_i2c_write(slaveConfig, 0x02, (chargeLevel>>8)&0xFF);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}	
};
uint16_t ltc2941GetCharge(slave_twi_config_t* slaveConfig)
{
	status_t status = STATUS_FAIL;
	uint8_t chargeBytes[2] = {0};
	uint16_t readData = 0;	
	//Read two bytes of Accumulated charge
	status = drv_i2c_read(slaveConfig, 0x02, chargeBytes, 2);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Read from register failed\r\n");
		#endif
		return 0;
	}
	readData = (((chargeBytes[0]&0xFF)<<8) + (chargeBytes[1]&0xFF));	
	return readData;
}
//[?3/?25/?2016 2:19 PM] Hriday Mehta: 
uint32_t getCalculatedPercentage(slave_twi_config_t* slaveConfig)
{
	
	uint32_t charge = ltc2941GetCharge(slaveConfig);
	return (((charge - CHARGE_EMPTY_VALUE)*100) / (0xffff - CHARGE_EMPTY_VALUE));
} 
uint32_t getRegValueForPercent(uint32_t percent)
{
	return (percent*(0xFFFF-CHARGE_EMPTY_VALUE)/100 + CHARGE_EMPTY_VALUE); 
}
status_t ltc2941SetChargeComplete(slave_twi_config_t* slaveConfig)
{
	status_t status = STATUS_FAIL;
	
	//Set the Accumulated charge register to indicate battery full
	status = drv_i2c_write(slaveConfig, 0x02, 0xff);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
	status = drv_i2c_write(slaveConfig, 0x03, 0xff);
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return STATUS_FAIL;
	}
}

uint8_t ltc2941GetStatus(slave_twi_config_t* slaveConfig)
{
	status_t status = STATUS_FAIL;
	uint8_t readData = 0;
	
	//uint8_t fullRead[10]; 
	
	//Read the status register
	status = drv_i2c_read(slaveConfig, 0x00, &readData, 1);
	//status = drv_i2c_read(slaveConfig, 0x00, &fullRead, 8);
	//int i = 0;
	//for(i=0;i<8;i++)
	//{
		//printf("%02x:",fullRead[i]);
	//}
	//printf("\r\n");
	if (status != STATUS_PASS)
	{
		#ifdef ENABLE_DEBUG_PRINTS
		puts("Write to Control register failed\r\n");
		#endif
		return 0;
	}	
	return readData;
}
