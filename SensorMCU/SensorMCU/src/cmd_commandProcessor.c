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
extern slave_twi_config_t eepromConfig;
extern void reConfigure_uart();

volatile cmd_debugStructure_t debugStructure = 
{
	.accelReadErrorCount = 0,
	.gyroReadErrorCount = 0,
	.magReadErrorCount = 0,
	.quatReadErrorCount = 0,
	.statusMask = 0,
	.receiveErrorCount = 0
};

volatile uint8_t outputDataBuffer[256] = {0};
volatile imuFrame_t imuFrameData =
{
	.Quaternion_x = 0x40066666,
	.Quaternion_y = 0x40066666,
	.Quaternion_z = 0x40066666,
	.Quaternion_w = 0x40066666,
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
volatile uint32_t configParameterValues[4] = {0xDEADBEEF,0xDEADBEEF,0xDEADBEEF,0xDEADBEEF};
volatile uint8_t configTest[5] = {0xE8,0x03,0x08,0x00,0xCA};//{0x00,0x08,0x03,0xE8,0xCA};//{0xE8,0x03,0x08,0x00,0xCA};
extern uint32_t warmUpParameterValues[35]; 	

//set range parameters
void setRangeParameters(uint32_t* rangeParameters); 
void setWarmUpParameters(uint32_t* warmUpParameters);



__attribute__((optimize("O0"))) int resetAndInitialize(slave_twi_config_t* slave_config)
{
	int status, readData[20] = {0};
	//Power up / reset request
	status = drv_i2c_write(slave_config, EM_RESET_REQUEST_REGISTER, EM_RESET_REQUEST_FLAG);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}	
	//Read interrupt pin from Sentral over and over again until it is high, then the device is ready to go. 
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
	

	
	//#ifdef HPR
	if(settings.enableHPR == 1)
	{
		settings.algoControlReg |= EM_HPR_OUTPUT_ENABLE_MASK;	//switch to head, pitch, roll mode
	}
	status = drv_i2c_write(slave_config, EM_ALGORITHM_CONTROL_REGISTER, settings.algoControlReg);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}	
	
	//mag rate
	status = drv_i2c_write(slave_config, EM_MAG_RATE_CONFIG_REGISTER, settings.magRate);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	
	//accel rate
	status = drv_i2c_write(slave_config, EM_ACCEL_RATE_CONFIG_REGISTER, settings.accelRate);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	
	//gyro rate
	status = drv_i2c_write(slave_config, EM_GYRO_RATE_CONFIG_REGISTER, settings.gyroRate);
	if (status != STATUS_PASS)
	{
		return STATUS_FAIL;
	}
	
	//enable events
	status = drv_i2c_write(slave_config, EM_INTERRUPT_CONFIG_REGISTER, EM_RESET_INT_FLAG | EM_ERROR_INT_FLAG);
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
		
	if(settings.warmUpValid && settings.loadWarmupOnBoot)
	{
		setWarmUpParameters(warmUpParameterValues);
	}
	if(settings.loadRangesOnBoot == 1)
	{
		setRangeParameters(settings.sensorRange);
	}
	return STATUS_PASS;
}

void sendImuDataFrame()
{
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_FRAME_RESP;
	outputDataBuffer[2] = settings.sensorId;
	memcpy(outputDataBuffer+3,&imuFrameData,35);
	//delay_us(100);
	pkt_SendRawPacket(outputDataBuffer, 38);	
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

void sendConfiguration()
{
	uint8_t algoControl = 0;
	//create the return packet
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_CONFIG_RESP;
	outputDataBuffer[2] = settings.sensorId;
	//read the control register before sending it. 
	drv_i2c_read(&em7180Config, EM_ALGORITHM_CONTROL_REGISTER, &algoControl, 1);
	outputDataBuffer[3] = algoControl;
	outputDataBuffer[4] = settings.loadWarmupOnBoot;
	outputDataBuffer[5] = settings.loadRangesOnBoot;	
	memcpy(outputDataBuffer+6,settings.sensorRange,8);
	pkt_SendRawPacket(outputDataBuffer, 14);
}

void sendGetStatusResponse()
{
	uint8_t data = 0;
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_STATUS_RESP;
	outputDataBuffer[2] = settings.sensorId;
	if(drv_i2c_read(&em7180Config, EM_MAG_RATE_ACTUAL_REGISTER, &data, 1) == STATUS_PASS)
	{
		debugStructure.magReadErrorCount = data;
	}
	if(drv_i2c_read(&em7180Config, EM_ACCEL_RATE_ACTUAL_REGISTER, &data, 1) == STATUS_PASS)
	{
		debugStructure.accelReadErrorCount = data;
	}
	if(drv_i2c_read(&em7180Config, EM_GYRO_RATE_ACTUAL_REGISTER, &data, 1) == STATUS_PASS)
	{
		debugStructure.gyroReadErrorCount = data;
	}
	//read the sentral status registers and update the status parameter.	
	if(drv_i2c_read(&em7180Config, 0x35, &debugStructure.statusMask, 4) != STATUS_PASS)
	{
		debugStructure.statusMask = 0xDEADBEEF; 
	}

	memcpy(outputDataBuffer+3,&debugStructure,24);
	//delay_us(100);
	pkt_SendRawPacket(outputDataBuffer, 27);	
}

__attribute__((optimize("O0"))) status_t readParameter(uint8_t parameterNumber, bool firstRead, uint32_t* parameterValue)
{
	status_t status = STATUS_FAIL; 
	sen_requestParam_t returnedParameter = {0,0}; 
	int32_t maxReadAttempts = 20; 	

	//set the parameter request register to the value we want to read
	drv_i2c_write(&em7180Config, EM_PARAM_REQUEST, parameterNumber);		
	delay_us(30000); //delay to give the sentral time to process
	//if this is the first read, then set the algorithm control value to 0x80 (parameter transfer bit)
	if(firstRead == true)
	{
		drv_i2c_write(&em7180Config,EM_ALGORITHM_CONTROL_REGISTER, 0x80); //settings.algoControlReg | 
	}
	delay_us(30000); //delay to give the sentral time to process
	while(maxReadAttempts-- > 0) 
	{
		//read the packet acknowledge over and over until we know that the value is valid
		if(drv_i2c_read(&em7180Config,EM_PARAM_ACKNOWLEDGE,&returnedParameter, 1) == STATUS_PASS)
		{
			if(returnedParameter.paramNumber == parameterNumber)
			{
				//yay the value is valid, set status to pass and return the parameter
				status = STATUS_PASS;				
				if(drv_i2c_read(&em7180Config,EM_READ_PARAM_BYTE_0,parameterValue, 4) == STATUS_PASS)
				{
					break;	
				}								
			}
		}		
	}	
	return status; 	
}


__attribute__((optimize("O0"))) status_t writeParameter(uint8_t parameterNumber, bool firstWrite, uint32_t parameterValue)
{
	status_t status = STATUS_FAIL;
	sen_loadParam_t loadParameter = {parameterValue,parameterNumber | 0x80}; //set the first bit, to make sure it's a write 
	int32_t maxReadAttempts = 10;
	uint8_t paramAck = 0;
	uint8_t buffer[5] = {0};
	int i = 0;
	memcpy(buffer,&loadParameter, sizeof(sen_loadParam_t));

	
	//write all the parameter bytes including the parameter number.
	drv_i2c_write_bytes(&em7180Config, EM_LOAD_PARAM_BYTE_0, buffer,5);		
	delay_us(30000); //delay to give the sentral time to process
	//if this is the first write, then set the algorithm control value to 0x80 (parameter transfer bit)
	if(firstWrite == true)
	{
		drv_i2c_write(&em7180Config,EM_ALGORITHM_CONTROL_REGISTER, 0x80); 
	}
	delay_us(30000); //delay to give the sentral time to process
	while(maxReadAttempts-- > 0)
	{
		//read the packet acknowledge over and over until we know that the value is valid
		if(drv_i2c_read(&em7180Config,EM_PARAM_ACKNOWLEDGE,&paramAck, 1) == STATUS_PASS)
		{
			if(paramAck == (parameterNumber | 0x80))
			{
				//yay! the value is valid, set status to pass
				status = STATUS_PASS;
				break;
			}
		}
		delay_us(500); //delay to give the sentral time to process
	}
	return status;
}

void updateAllConfigParameters()
{
	//get parameters 74-80
	status_t status = STATUS_PASS; 
	uint8_t parameterNumber = 0x4A; 
	int i = 0;
	bool firstValue = true; 
	for(i = 0; i<4 ; i++)
	{
		status |= readParameter(parameterNumber, firstValue, &(configParameterValues[i]));
		//reset the first value
		if(firstValue)
		{
			firstValue = false; 
		}
		parameterNumber++;		
	}
	if(status == STATUS_PASS)
	{
		memcpy(settings.sensorRange,configParameterValues,8); //copy the ranges to the settings structure. 	
	}
	//reset the parameter request register
	drv_i2c_write(&em7180Config, EM_PARAM_REQUEST, 0x00);	
	//reset the algorithm control register
	drv_i2c_write(&em7180Config,EM_ALGORITHM_CONTROL_REGISTER, settings.algoControlReg); //send the configured algo control register.  	
}
void sendConfigurationParameters()
{
	//create the return packet
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_CONFIG_PARAM_RESP;
	outputDataBuffer[2] = settings.sensorId;
	memcpy(outputDataBuffer+3,configParameterValues,16);
	pkt_SendRawPacket(outputDataBuffer, 19);	
}
void setRangeParameters(uint32_t* rangeParameters)
{
	uint8_t parameterNumber = 0x4A;
	int i = 0;
	bool firstValue = true;
	for(i = 0; i<2 ; i++)
	{
		writeParameter(parameterNumber, firstValue, rangeParameters[i]);
		//reset the first value
		if(firstValue)
		{
			firstValue = false;
		}
		parameterNumber++;
		delay_us(1000);
	}
	//reset the parameter request register
	drv_i2c_write(&em7180Config, EM_PARAM_REQUEST, 0x00);	
	//reset the algorithm control register
	drv_i2c_write(&em7180Config,EM_ALGORITHM_CONTROL_REGISTER, settings.algoControlReg);
}

void setWarmUpParameters(uint32_t* warmUpParameters)
{
	uint8_t parameterNumber = 0x01;
	int i = 0;
	bool firstValue = true;
	for(i = 0; i<35 ; i++)
	{
		writeParameter(parameterNumber, firstValue, warmUpParameters[i]);
		//reset the first value
		if(firstValue)
		{
			firstValue = false;
		}
		parameterNumber++;
		delay_us(1000);
	}
	//reset the parameter request register
	drv_i2c_write(&em7180Config, EM_PARAM_REQUEST, 0x00);	
	//reset the algorithm control register
	drv_i2c_write(&em7180Config,EM_ALGORITHM_CONTROL_REGISTER, settings.algoControlReg);

}
status_t updateWarmStartUpValues()
{
	uint8_t parameterNumber = 0x01;
	int i = 0;
	bool firstValue = true;
	uint8_t algoStatus = 0;
	status_t status = STATUS_PASS; 
	for(i = 0; i<35 ; i++)
	{
		status |= readParameter(parameterNumber, firstValue, &(warmUpParameterValues[i]));
		//reset the first value
		if(firstValue)
		{
			firstValue = false;
		}
		parameterNumber++;
	}
	if(status == STATUS_PASS)
	{
		//verify that the cal was stable when the parameters were read. 
		status = drv_i2c_read(&em7180Config, 0x38, &algoStatus, 1);
		if(algoStatus & (1<<3) > 0)
		{
			//magnetic cal is complete
			settings.warmUpValid = 1; //set warm up valid to true
		}
		else
		{
			settings.warmUpValid = 0;
		}
	}
	else
	{
		//the warm up parameters are not valid. 
		settings.warmUpValid = 0;
	}
	
	return status; 		
}

void sendWarmStartUpValues()
{
	//create the return packet
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR;
	outputDataBuffer[1] = PACKET_COMMAND_ID_GET_WARMUP_PARAM_RESP;
	outputDataBuffer[2] = settings.sensorId;
	memcpy(outputDataBuffer+3,warmUpParameterValues,140);
	pkt_SendRawPacket(outputDataBuffer, 143);	
}


void updateImuData()
{
	uint8_t statusRegister = 0x00; 
	uint8_t algoStatus = 0x00; 
	
	status_t status = STATUS_PASS;
	status = drv_i2c_read(&em7180Config, 0x35, &statusRegister, 1);
	status |= drv_i2c_read(&em7180Config, 0x34, &algoStatus, 1);
	if(status == STATUS_PASS)
	{	
		//check the IMU status to confirm it's operational
		if ((statusRegister & 0x01 > 0) || (statusRegister & 0x02 > 0) || algoStatus != 0x01 ) // 
		{
			//reset the IMU, and skip this 
			resetAndInitialize(&em7180Config); 		
		}
		else
		{
			if(drv_i2c_read(&em7180Config, 0x00, &imuFrameData.Quaternion_x, 16) != STATUS_PASS)
			{
				status |= STATUS_FAIL;
				//debugStructure.quatReadErrorCount++;	
			}	
			if(drv_i2c_read(&em7180Config, 0x12, &imuFrameData.Magnetic_x, 12) != STATUS_PASS)
			{
				status |= STATUS_FAIL;
				//debugStructure.magReadErrorCount++;
			}
			if(drv_i2c_read(&em7180Config, 0x1A, &imuFrameData.Acceleration_x, 12) != STATUS_PASS)
			{
				status |= STATUS_FAIL;
				//debugStructure.accelReadErrorCount++;
			}
			if(drv_i2c_read(&em7180Config, 0x22, &imuFrameData.Rotation_x, 12) != STATUS_PASS)
			{
				status |= STATUS_FAIL;
				//debugStructure.gyroReadErrorCount++;
			}	
		}
	}
	//read the algorithm status and update the LEDs accordingly. 
	status |= drv_i2c_read(&em7180Config, 0x38, &algoStatus, 1);
	if(status != STATUS_PASS)
	{
		algoStatus |= 1<<7; //set the bad frame flag
	}
	
	//check if the frame status has changed since the last time is was read...
	if(imuFrameData.frameStatus != algoStatus)
	{	
		//check for comms error
		if((algoStatus & (1<<7)) > 0)
		{
			//there was a failure reading the sensor data set LED to Yellow. 
			port_pin_set_output_level(LED_RED_PIN,LED_ACTIVE);
			port_pin_set_output_level(LED_GREEN_PIN,LED_ACTIVE);
			port_pin_set_output_level(LED_BLUE_PIN,LED_INACTIVE);			
		}
		//check the calibration stable bit
		else if((algoStatus & (1<<3)) > 0)
		{
			//check the magnetic transient bit
			if((algoStatus & (1<<4)) > 0)
			{
				//there is a magnetic transient set to turquoise 
				port_pin_set_output_level(LED_RED_PIN,LED_INACTIVE);
				port_pin_set_output_level(LED_GREEN_PIN,LED_ACTIVE);
				port_pin_set_output_level(LED_BLUE_PIN,LED_ACTIVE);
			}
			else
			{			
				//check for board stillness
				if((algoStatus & (1<<2)) > 0)
				{
					//board is still, no magnetic transient, cal is stable set to blue
					port_pin_set_output_level(LED_RED_PIN,LED_INACTIVE);
					port_pin_set_output_level(LED_GREEN_PIN,LED_INACTIVE);
					port_pin_set_output_level(LED_BLUE_PIN,LED_ACTIVE);				
				}
				else
				{
					//no magnetic transient, cal is stable, not still, set to green
					port_pin_set_output_level(LED_RED_PIN,LED_INACTIVE);
					port_pin_set_output_level(LED_GREEN_PIN,LED_ACTIVE);
					port_pin_set_output_level(LED_BLUE_PIN,LED_INACTIVE);	
				}
			
			}
		}
		else
		{
			//Cal is not stable, set to red. 
			port_pin_set_output_level(LED_RED_PIN,LED_ACTIVE);
			port_pin_set_output_level(LED_GREEN_PIN,LED_INACTIVE);
			port_pin_set_output_level(LED_BLUE_PIN,LED_INACTIVE);			
		}
		imuFrameData.frameStatus = algoStatus; 
	}
	
}
volatile uint8_t passThroughEnabled = FALSE; 
void togglePassthrough(uint8_t enable)
{
	uint8_t regResult = 0x00; 
	status_t status = STATUS_PASS; 
	if(enable == 1)
	{
		status |= drv_i2c_write(&em7180Config,EM_ALGORITHM_CONTROL_REGISTER, 0x01); //enable stand by
		delay_us(30000); 
		status |= drv_i2c_read(&em7180Config, 0x38, &regResult, 1); 
		//if(regResult & 0x01)
		//{
			//
		//}
		status |= drv_i2c_write(&em7180Config,0xA0, 0x01); //enable passthrough
		status |= drv_i2c_read(&em7180Config, 0x9E, &regResult, 1);
		if(regResult & 0x01)
		{
			passThroughEnabled = TRUE; 
		} 
	}
	else
	{
		drv_i2c_write(&em7180Config,0xA0, 0x00); //disable the passthrough
		resetAndInitialize(&em7180Config); //reset the sentral
		passThroughEnabled = FALSE; 
	}
} 

void getEepromPacket(uint16_t address)
{	
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR; 
	outputDataBuffer[1] = PACKET_COMMAND_ID_EEPROM_PACKET; 	
	outputDataBuffer[2] = (address & 0xFF);
	outputDataBuffer[3] = (address >> 8);
	if(drv_i2c_read_16bit(&eepromConfig, address, &outputDataBuffer[4],64) == STATUS_PASS)
	{
		//send out the packet
		pkt_SendRawPacket(outputDataBuffer, 68);	
	}	
} 

void writeEepromPacket(uint8_t* data, uint16_t length)
{
	status_t status = STATUS_PASS; 
	outputDataBuffer[0] = PACKET_TYPE_IMU_SENSOR; 
	//this function expects that the first 2 bytes are the address of where the packet should go. 
	outputDataBuffer[1] = PACKET_COMMAND_ID_WRITE_EEPROM_RESP;
	//write the address to the front of the response packet	
	outputDataBuffer[2] = data[0]; //LSB
	outputDataBuffer[3] = data[1]; //MSB
	//check the size of the packet, make sure it's not a corrupt packet that somehow crept through. 
	if(length != 66)
	{
		status = STATUS_FAIL; 
	} 
	
	if(status == STATUS_PASS)
	{
		//reverse the byte order for the EEPROM
		uint8_t swapByte = data[0];
		data[0] = data[1];
		data[1] = swapByte;
		status = drv_i2c_write_bytes_raw(&eepromConfig,data, 66);
	}	
	
	if(status == STATUS_PASS) //the packet should be 2 address bytes + 64 bytes of data. 
	{
		outputDataBuffer[4] = 0x01; //set the flag to 1 to indicate that the write was successful
	} 
	else
	{
		outputDataBuffer[4] = 0x00; //set the flag to 0 to indicate that the write FAILED
	}
	delay_us(10000);
	pkt_SendRawPacket(outputDataBuffer, 5);	
}

#ifdef SUPPORT_DEBUG_COMMANDS
void updateImuDataFake()
{
	imuFrameData.Acceleration_x++;
	imuFrameData.Acceleration_y++;
	imuFrameData.Acceleration_z++;
	imuFrameData.Magnetic_x++;
	imuFrameData.Magnetic_y++;
	imuFrameData.Magnetic_z++;
	imuFrameData.Rotation_x++;
	imuFrameData.Rotation_y++;
	imuFrameData.Rotation_z++;
	imuFrameData.Quaternion_w++;
	imuFrameData.Quaternion_x++;
	imuFrameData.Quaternion_y++;
	imuFrameData.Quaternion_z++;
}



void resetImuDataFake()
{
	imuFrameData.Quaternion_x = 0x40066666;
	imuFrameData.Quaternion_y = 0x40066666;
	imuFrameData.Quaternion_z = 0x40066666;
	imuFrameData.Quaternion_w = 0x40066666;
	imuFrameData.Magnetic_x = 1;
	imuFrameData.Magnetic_y = 2;
	imuFrameData.Magnetic_z = 3;
	imuFrameData.Acceleration_x = 4;
	imuFrameData.Acceleration_y = 5;
	imuFrameData.Acceleration_z = 6;
	imuFrameData.Rotation_x = 7;
	imuFrameData.Rotation_y = 8;
	imuFrameData.Rotation_z = 9;
}
#endif
//
/* all packets have the format
 <type(1B)><command id(1B)><payload(size dependant on command type)> 
*/
void cmd_processPacket(rawPacket_t* packet)
{
	static uint16_t address = 0;
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
			#ifdef SUPPORT_DEBUG_COMMANDS
			case PACKET_COMMAND_ID_RESET_FAKE:
				resetImuDataFake();	//sent by the Master to reset the data structure for sync
			break;
			case PACKET_COMMAND_ID_UPDATE_FAKE:
				updateImuDataFake();	//increments the count of variables in the structure
			break;
			#endif
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
					//save the settings to NVM
					writeSettings(); 
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
					//send out the status structure
					sendGetStatusResponse();
				}
			break; 		
			case PACKET_COMMAND_ID_UPDATE_CONFIG_PARAM:			
				updateAllConfigParameters();			
			break;
			case PACKET_COMMAND_ID_GET_CONFIG_PARAM:
				//check if the ID matches the one assigned
				if(packet->payload[2] == settings.sensorId)
				{
					//send out the config parameters
					sendConfigurationParameters();
				}
			break;
			case PACKET_COMMAND_ID_UPDATE_WARMUP_PARAM:
				updateWarmStartUpValues();
			break;
			case PACKET_COMMAND_ID_GET_WARMUP_PARAM:
				//check if the ID matches the one assigned
				if(packet->payload[2] == settings.sensorId)
				{
					//send out the config parameters
					sendWarmStartUpValues();
				}
			break;			
			case PACKET_COMMAND_ID_CHANGE_BAUD:
				/*	don't check for ID, all sensors should work in same config	*/
				// 4 bytes are received in the order of LSB first
				settings.baud = ((uint32_t)packet->payload[2]) | ((uint32_t)packet->payload[3] << 8) 
								| ((uint32_t)packet->payload[4] << 16) | ((uint32_t)packet->payload[5] << 24);
				//this will not work anymore! need to fix it later on. 
				//reConfigure_uart();
			break;
			case PACKET_COMMAND_ID_ENABLE_HPR:
				/*	don't check for ID, all sensors should work in same config	*/
				if(packet->payload[2] == 1)
				{
					settings.enableHPR = 1;
					resetAndInitialize(&em7180Config);
				}
				else
				{
					settings.enableHPR = 0;
					resetAndInitialize(&em7180Config);
				}
			break;
			case PACKET_COMMAND_ID_SET_RATES:
			settings.magRate = packet->payload[2];
			settings.accelRate = packet->payload[3];
			settings.gyroRate = packet->payload[4];
			resetAndInitialize(&em7180Config);
			break;
			case PACKET_COMMAND_ID_SET_CONFIG:
				if(packet->payload[2] == settings.sensorId)
				{
					settings.algoControlReg = packet->payload[3]; 					
					settings.loadWarmupOnBoot = packet->payload[4]; 
					settings.loadRangesOnBoot = packet->payload[5]; 
					memcpy(settings.sensorRange,&(packet->payload[6]),8);
					resetAndInitialize(&em7180Config);
				}
			break;
			case PACKET_COMMAND_ID_GET_CONFIG:
				if(packet->payload[2] == settings.sensorId)
				{
					sendConfiguration();
				}
			break;
			case PACKET_COMMAND_ID_SAVE_TO_NVM:
				if(packet->payload[2] == settings.sensorId)
				{
					//write all the configuration parameters to NVM
					writeSettings();
				}
			break;
			case PACKET_COMMAND_ID_TOGGLE_PASSTHROUGH:
			if(packet->payload[2] == settings.sensorId)
			{
				togglePassthrough(packet->payload[3]);
			}
			break;	
			case PACKET_COMMAND_ID_READ_EEPROM_PACKET:
			if(packet->payload[2] == settings.sensorId)
			{
				address = packet->payload[4];
				address = address<<8;
				address += packet->payload[3];
				getEepromPacket(address); 
			}
			break;
			case PACKET_COMMAND_ID_WRITE_EEPROM_PACKET:
			if(packet->payload[2] == settings.sensorId)
			{
				writeEepromPacket(&packet->payload[3], packet->payloadSize - 3); 					
			}
			break;			
		}
	}
}
