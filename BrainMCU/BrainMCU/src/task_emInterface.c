/*
 * task_emInterface.c
 *
 * Created: 2/16/2016 1:35:28 PM
 * Copyright Heddoko(TM) 2015, all rights reserved 
 *  Author: Hriday Mehta
 */ 

#include <string.h>
#include "task_emInterface.h"
#include "task_dataProcessor.h"
#include "task_commandProc.h"

//extern variables
extern xQueueHandle queue_dataHandler;
extern unsigned long sgSysTickCount;
extern slave_twi_config_t em7180Config[2];
extern uint16_t packetReceivedMask;
extern task_em_config_t task_em_config[];

dataPacket_t packet;
static bool enableRecording = FALSE;

void task_emHandler(void *pvParameters)
{
	task_em_config_t* em_config = (task_em_config_t*) pvParameters;
	int status = 0, average = 0, count = 0;
	char dataBuffer[40] = {0};
	uint32_t readData[4] = {0};
	//temporary declarations, clean up regularly
	unsigned long sgSysTickCountOld = 0;
	//end of temporary declarations
	
	packetReceivedMask |= 1<<9;	//add mask for EM sensors
	packetReceivedMask |= 1<<10;
	
	//if (task_em_initializeImus(&task_em_config[0]) != STATUS_PASS)
	//{
		//debugPrintString("EM initialization failed\r\n");
	//}
	//task_em_start(&task_em_config[0]);
	
	while (1)
	{
		if ((em_config->isInit == TRUE) && (enableRecording == TRUE))
		{
			//First device
			packet.type = DATA_PACKET_TYPE_EM;
			packet.imuId = 9;	//TODO: make it generalized
			sgSysTickCountOld = sgSysTickCount;
			status = drv_i2c_read(em_config->slaveConfigArray[0], EM_EVENT_STATUS_REGISTER, &readData[0], 1);
			delay_ms(1);
			if (status != TWI_SUCCESS)
			{
				debugPrintString("Error in reading status register\r\n");
			}
			else
			{
				if (readData[0] & EM_RESET_STATUS_MASK)
				{
					debugPrintString("CPU reset\r\n");
					if(resetAndInitialize(em_config->slaveConfigArray[0]) != STATUS_PASS)
					{
						status = STATUS_FAIL;
					}
				}
				else  if (readData[0] & EM_ERROR_STATUS_MASK)
				{
					debugPrintString("Error\r\n");
					//if(resetAndInitialize(em_config->slaveConfigArray[0]) != STATUS_PASS)
					//{
						//status = STATUS_FAIL;
					//}
				}
				else if (readData[0] & EM_RESULT_MASK_QUATERNION)
				{
					//debugPrintString("Quaternion result\r\n");
					status = drv_i2c_read(em_config->slaveConfigArray[0], EM_QUATERNION_RESULT_ADDRESS, &readData[0], 16);
					delay_ms(1);
					if (status != TWI_SUCCESS)
					{
						debugPrintString("Error in reading data register\r\n");
					}
					else
					{//TODO: verify the correct endian order at output
						//convert the data to ASCII
						snprintf(dataBuffer, EM_TOTAL_PACKET_LENGTH, "%x;%x;%x;%x", readData[0], readData[1], readData[2], readData[3]);
						memcpy(packet.data, dataBuffer, EM_TOTAL_PACKET_LENGTH);	//ignore the last null character
						//enqueue the packet for the data processor.
						if(queue_dataHandler != NULL)
						{
							if(xQueueSendToBack( queue_dataHandler,( void * ) &packet,10 ) != TRUE)
							{
								vTaskDelay(1);
							}
						}
						average = sgSysTickCount - sgSysTickCountOld;
						count++;
					}
				}
				else if (readData[0] & EM_RESULT_MASK_MAG)
				{
					//debugPrintString("Mag result\r\n");
				}
				else if (readData[0] & EM_RESULT_MASK_ACCEL)
				{
					//debugPrintString("Accel result\r\n");
				}
				else if (readData[0] & EM_RESULT_MASK_GYRO)
				{
					//debugPrintString("Gyro result\r\n");
				}
				else
				{
					//debugPrintString("Corrupt data\r\n");
				}
			}
			vTaskDelay(7);
			////Second device
			packet.type = DATA_PACKET_TYPE_EM;
			packet.imuId = 10;
			sgSysTickCountOld = sgSysTickCount;
			status = drv_i2c_read(em_config->slaveConfigArray[1], EM_EVENT_STATUS_REGISTER, &readData[0], 1);
			delay_ms(1);
			if (status != TWI_SUCCESS)
			{
				debugPrintString("Error in reading status register\r\n");
			}
			else
			{
				if (readData[0] & EM_RESET_STATUS_MASK)
				{
					debugPrintString("CPU reset\r\n");	//TODO: add independent loop to reinitialize and remove isInit flag
					if(resetAndInitialize(em_config->slaveConfigArray[1]) != STATUS_PASS)
					{
						status = STATUS_FAIL;
					}
				}
				else  if (readData[0] & EM_ERROR_STATUS_MASK)
				{
					debugPrintString("Error\r\n");
					//if(resetAndInitialize(em_config->slaveConfigArray[1]) != STATUS_PASS)
					//{
						//status = STATUS_FAIL;
					//}
				}
				else if (readData[0] & EM_RESULT_MASK_QUATERNION)
				{
					//debugPrintString("Quaternion result\r\n");
					status = drv_i2c_read(em_config->slaveConfigArray[1], EM_QUATERNION_RESULT_ADDRESS, &readData[0], 16);
					delay_ms(1);
					if (status != TWI_SUCCESS)
					{
						debugPrintString("Error in reading data register\r\n");
					}
					else
					{
						//convert the data to ASCII
						snprintf(dataBuffer, EM_TOTAL_PACKET_LENGTH, "%x;%x;%x;%x", readData[0], readData[1], readData[2], readData[3]);
						memcpy(packet.data, dataBuffer, EM_TOTAL_PACKET_LENGTH);
						//enqueue the packet for the data processor.
						if(queue_dataHandler != NULL)
						{
							if(xQueueSendToBack( queue_dataHandler,( void * ) &packet,10 ) != TRUE)
							{
								vTaskDelay(1);
							}
						}
						average = sgSysTickCount - sgSysTickCountOld;
						count++;
					}
				}
				else if (readData[0] & EM_RESULT_MASK_MAG)
				{
					//debugPrintString("Mag result\r\n");
				}
				else if (readData[0] & EM_RESULT_MASK_ACCEL)
				{
					//debugPrintString("Accel result\r\n");
				}
				else if (readData[0] & EM_RESULT_MASK_GYRO)
				{
					//debugPrintString("Gyro result\r\n");
				}
				else
				{
					//debugPrintString("Corrupt data\r\n");
				}
			}
		}
		//vTaskDelay(em_config->pollRate);
		vTaskDelay(7);
	}
}

int task_em_initializeImus(task_em_config_t* emConfig)
{
	int status = STATUS_FAIL;
	status = resetAndInitialize(emConfig->slaveConfigArray[0]);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	status = resetAndInitialize(emConfig->slaveConfigArray[1]);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	emConfig->isInit = 1;
	return STATUS_PASS;
}


static int resetAndInitialize(slave_twi_config_t* slave_config)
{
	int status, readData[2] = {0};
	//Power up / reset request
	status = drv_i2c_write(slave_config, EM_RESET_REQUEST_REGISTER, EM_RESET_REQUEST_FLAG);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data to request register\r\n");
		return STATUS_FAIL;
	}
	
	//wait for the device to respond if interrupt is configured
	delay_ms(510);	//if interrupt is not configured
	//while(gpio_pin_is_low(PIO_PA21_IDX));
	//int pin_flg = pio_get_pin_value(PIO_PA21_IDX);
	//if (int_flg != 1)
	//{
		//return STATUS_FAIL;
	//}
	status = drv_i2c_read(slave_config, EM_SENTRAL_STATUS_REGISTER, &readData[0], 1);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in reading status register\r\n");
		return STATUS_FAIL;
	}
	else
	{
		if (readData[0] == EM_EEPROM_SUCCESS_MASK)
		{
			debugPrintString("Successfully loaded EEPROM\r\n");
		}
		else if (readData[0] == EM_NO_EEPROM_ERROR_MASK)
		{
			debugPrintString("No EEPROM detected\r\n");
			return STATUS_FAIL;
		}
		else if (readData[0] == EM_EEPROM_UPLOAD_ERROR_MASK)
		{
			debugPrintString("Error in upload\r\n");
			return STATUS_FAIL;
		}
		else
		{
			debugPrintString("Corrupt status data\r\n");
			return STATUS_FAIL;
		}
	}
	
	#ifdef HPR
	//read the algorithm control register
	status = drv_i2c_read(slave_config, EM_ALGORITHM_CONTROL_REGISTER, &readData[0], 1);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in reading alogrithm control register\r\n");
		return STATUS_FAIL;
	}
	readData[0] |= EM_HPR_OUTPUT_ENABLE_MASK;	//switch to head, pitch, roll mode
	//write back to register
	status = drv_i2c_write(slave_config, EM_ALGORITHM_CONTROL_REGISTER, readData[0]);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing\r\n");
		return STATUS_FAIL;
	}
	#endif
	
	//mag rate
	status = drv_i2c_write(slave_config, EM_MAG_RATE_CONFIG_REGISTER, EM_MAG_OUPUT_DATA_RATE);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data\r\n");
		return STATUS_FAIL;
	}
	
	//accel rate
	status = drv_i2c_write(slave_config, EM_ACCEL_RATE_CONFIG_REGISTER, EM_ACCEL_OUPUT_DATA_RATE);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data\r\n");
		return STATUS_FAIL;
	}
	
	//gyro rate
	status = drv_i2c_write(slave_config, EM_GYRO_RATE_CONFIG_REGISTER, EM_GYRO_OUPUT_DATA_RATE);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data\r\n");
		return STATUS_FAIL;
	}
	
	//enable events
	status = drv_i2c_write(slave_config, EM_INTERRUPT_CONFIG_REGISTER, (EM_RESET_INT_FLAG));
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data\r\n");
		return STATUS_FAIL;
	}
	
	////run request
	//status = drv_i2c_write(slave_config, EM_RUN_REQUEST_REGISTER, EM_RUN_REQUEST_FLAG);
	//if (status != TWI_SUCCESS)
	//{
		//debugPrintString("Error in writing data\r\n");
		//return STATUS_FAIL;
	//}
	return STATUS_PASS;
}

status_t task_em_start(task_em_config_t* emConfig)
{
	//Run request for both device
	int status = drv_i2c_write(emConfig->slaveConfigArray[0] , EM_RUN_REQUEST_REGISTER, EM_RUN_REQUEST_FLAG);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data\r\n");
		return STATUS_FAIL;
	}
	
	status = drv_i2c_write(emConfig->slaveConfigArray[1] , EM_RUN_REQUEST_REGISTER, EM_RUN_REQUEST_FLAG);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data\r\n");
		return STATUS_FAIL;
	}
	enableRecording = TRUE;
	return STATUS_PASS;
}

status_t task_em_stop(task_em_config_t* emConfig)
{
	//Run request for both device
	enableRecording = FALSE;
	int status = drv_i2c_write(emConfig->slaveConfigArray[0] , EM_RUN_REQUEST_REGISTER, EM_CLEAR_RUN_REQUEST_FLAG);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("Error in writing data\r\n");
		return STATUS_FAIL;
	}
	
	status = drv_i2c_write(emConfig->slaveConfigArray[1] , EM_RUN_REQUEST_REGISTER, EM_CLEAR_RUN_REQUEST_FLAG);
	if (status != TWI_SUCCESS)
	{
		debugPrintString("error in writing data\r\n");
		return STATUS_FAIL;
	}
	return STATUS_PASS;
}