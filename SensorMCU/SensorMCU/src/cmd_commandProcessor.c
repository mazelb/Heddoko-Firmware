/*
 * cmd_commandProcessor.c
 *
 * Created: 3/21/2016 1:33:38 PM
 *  Author: sean
 */ 
#include <asf.h>
#include <string.h>
#include "cmd_commandProcessor.h"
#include "common.h"
#include "arm_math.h"
#include "pkt_packetParser.h"
#include "drv_i2c.h"
#include "imu.h"

extern sensorSettings_t settings;
extern drv_twi_config_t twiConfig;
extern slave_twi_config_t em7180Config;

volatile cmd_debugStructure_t debugStructure = 
{
	.accelReadErrorCount = 0,
	.gyroReadErrorCount = 0,
	.magReadErrorCount = 0,
	.quatReadErrorCount = 0,
	.statusMask = 0,
	.receiveErrorCount = 0
};

volatile uint8_t outputDataBuffer[100] = {0};
volatile imuFrame_t imuFrameData =
{
	.Quaternion_x = 0.1,
	.Quaternion_y = 0.2,
	.Quaternion_z = 0.3,
	.Quaternion_w = 0.4,
	.Magnetic_x = 1,
	.Magnetic_y = 2,
	.Magnetic_z = 3,
	.Acceleration_x = 4,
	.Acceleration_y = 5,
	.Acceleration_z = 6,
	.Rotation_x = 7,
	.Rotation_y = 8,
	.Rotation_z = 9
};


int resetAndInitialize(slave_twi_config_t* slave_config)
{
	int status, readData[20] = {0};
	//Power up / reset request
	status = drv_i2c_write(slave_config, EM_RESET_REQUEST_REGISTER, EM_RESET_REQUEST_FLAG);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}	
	//Read SW1_Int over and over again until it is high, then the device is ready to go. 
	//TODO remove this delay
	delay_ms(1000);	//wait for the device to complete EEPROM upload
	
	status = drv_i2c_read(slave_config, EM_SENTRAL_STATUS_REGISTER, &readData[0], 1);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	else
	{
		debugStructure.statusMask = readData[0]; 
		if (readData[0] == EM_EEPROM_SUCCESS_MASK)
		{
		}
		else if (readData[0] == EM_NO_EEPROM_ERROR_MASK)
		{
			return STATUS_FAIL;
		}
		else if (readData[0] == EM_EEPROM_UPLOAD_ERROR_MASK)
		{
			return STATUS_FAIL;
		}
		else
		{
			return STATUS_FAIL;
		}
	}
	
	#ifdef HPR
	//read the algorithm control register
	status = drv_i2c_read(slave_config, EM_ALGORITHM_CONTROL_REGISTER, &readData[0], 1);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	readData[0] |= EM_HPR_OUTPUT_ENABLE_MASK;	//switch to head, pitch, roll mode
	//write back to register
	status = drv_i2c_write(slave_config, EM_ALGORITHM_CONTROL_REGISTER, readData[0]);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	#endif
	
	//mag rate
	status = drv_i2c_write(slave_config, EM_MAG_RATE_CONFIG_REGISTER, EM_MAG_OUPUT_DATA_RATE);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	
	//accel rate
	status = drv_i2c_write(slave_config, EM_ACCEL_RATE_CONFIG_REGISTER, EM_ACCEL_OUPUT_DATA_RATE);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	
	//gyro rate
	status = drv_i2c_write(slave_config, EM_GYRO_RATE_CONFIG_REGISTER, EM_GYRO_OUPUT_DATA_RATE);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	
	//enable events
	status = drv_i2c_write(slave_config, EM_INTERRUPT_CONFIG_REGISTER, EM_RESET_INT_FLAG);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	
	//run request
	status = drv_i2c_write(slave_config, EM_RUN_REQUEST_REGISTER, EM_RUN_REQUEST_FLAG);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	return STATUS_PASS;
}

void sendImuDataFrame()
{
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_FRAME_RESP;
	outputDataBuffer[2] = settings.sensorId;
	memcpy(outputDataBuffer+3,&imuFrameData,34);
	pkt_SendRawPacket(outputDataBuffer, 37);	
}
void sendSetImuIdResponse()
{
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_SET_IMU_ID_RESP;
	memcpy(outputDataBuffer+2,settings.serialNumber,16);
	pkt_SendRawPacket(outputDataBuffer, 18);	
}

void sendButtonPressEvent()
{
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_BUTTON_PRESS;
	memcpy(outputDataBuffer+2,settings.serialNumber,16);
	pkt_SendRawPacket(outputDataBuffer, 18);	
}


void sendGetStatusResponse()
{
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_STATUS_RESP;
	outputDataBuffer[2] = settings.sensorId;
	memcpy(outputDataBuffer+3,&debugStructure,25);
	pkt_SendRawPacket(outputDataBuffer, 27);	
}

void updateImuData()
{
	uint8_t statusRegister = 0x00; 
	status_t status = drv_i2c_read(&em7180Config, 0x35, &statusRegister, 1);
	if(status == STATUS_PASS)
	{	
		//check the IMU status to confirm it's operational
		if ((statusRegister & 0x01 > 0) || (statusRegister & 0x02 > 0) )
		{
			//reset the IMU, and skip this 
			resetAndInitialize(&em7180Config); 		
		}
		else
		{
			if(drv_i2c_read(&em7180Config, 0x00, &imuFrameData.Quaternion_x, 16) != STATUS_PASS)
			{
				debugStructure.quatReadErrorCount++;	
			}	
			if(drv_i2c_read(&em7180Config, 0x00, &imuFrameData.Magnetic_x, 12) != STATUS_PASS)
			{
				debugStructure.magReadErrorCount++;
			}
			if(drv_i2c_read(&em7180Config, 0x00, &imuFrameData.Acceleration_x, 12) != STATUS_PASS)
			{
				debugStructure.accelReadErrorCount++;
			}
			if(drv_i2c_read(&em7180Config, 0x00, &imuFrameData.Rotation_x, 12) != STATUS_PASS)
			{
				debugStructure.gyroReadErrorCount++;
			}	
		}
	}
	
}


//
/* all packets have the format
 <type(1B)><command id(1B)><payload(size dependant on command type)> 
*/
void cmd_processPacket(rawPacket_t* packet)
{
	//make sure the packet has enough bytes in it
	if(packet->payloadSize < 2)
	{
		//if there's less than two bytes... its not a valid packet
		return; 
	}
	//check which type of packet it is.
	//we only care about master control packets. 
	if(packet->payload[0] == PACKET_TYPE_MASTER_CONTROL)
	{
		switch(packet->payload[1])
		{
			case PACKET_COMMAND_ID_UPDATE:
				//call function here to update the IMU data
				updateImuData(); 
			break;
			case PACKET_COMMAND_ID_GET_FRAME:
				//check if the ID matches the one assigned
				if(packet->payload[2] == settings.sensorId)
				{
					//send out the IMU data
					sendImuDataFrame();
				}
			break; 
			case PACKET_COMMAND_ID_SET_IMU_ID:
				//confirm the packet size. 
				if(packet->payloadSize != 19)
				{
					//invalid packet size
					return; 
				}
				//check that the serial numbers match
				if(memcmp(settings.serialNumber,(packet->payload)+2,16) == 0)
				{
					settings.sensorId = packet->payload[18];
					//send the response
					sendSetImuIdResponse();
				}				
			break;
			case PACKET_COMMAND_ID_SETUP_MODE:
				//This byte determines whether the device is in setup mode or not. 
				if(packet->payload[2] == 1)
				{
					settings.setupModeEnabled = true;
				}
				else
				{
					settings.setupModeEnabled = false; 
				}				
			break;
			case PACKET_COMMAND_ID_GET_STATUS:
				//check if the ID matches the one assigned
				if(packet->payload[2] == settings.sensorId)
				{
					//send out the IMU data
					sendGetStatusResponse();
				}
			break; 			
			
		}
	}
}
