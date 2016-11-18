/*
 * common.h
 *
 * Created: 9/21/2015 8:34:24 AM
 *  Author: sean
 * @brief: Includes all the generic values used by all files
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#ifndef COMMON_H_
#define COMMON_H_

#define VERSION "V0.1b"
#define SENSOR_ID_DEFAULT 0
#define ALL_INTERRUPT_MASK  0xffffffff
#define TRUE 1
#define FALSE 0

//#define ENABLE_DEBUG_DATA	//enable free running counter instead of actual imu Data.

typedef enum 
{
	STATUS_PASS = 0,
	STATUS_FAIL = 1,
	STATUS_EOF = 2 //end of file, used in getChar	
}status_t;
#define SETTINGS_NVM_PAGE 0x3F00
#define SETTINGS_MASTER_KEY 0xb01dfaca
typedef struct
{
	uint32_t settingsKey; //key that validates if the settings have been loaded.  
	uint8_t sensorId;	
	uint8_t serialNumber[16]; //serialnumber of the processor
	uint8_t setupModeEnabled; //send button press messages when this is set to 1
	uint8_t enableHPR; //Heading pitch roll enabled = 1, quaternion = 0
	uint32_t baud;		//does nothing for now... baud rate changing is disabled. 
	uint8_t magRate;
	uint8_t accelRate;
	uint8_t gyroRate;
	uint8_t algoControlReg; //default settings for the algorithm controller. 
	uint32_t sensorRange[2]; //Sensor range for mag and acceleration = byte[0], gyro = byte[1]
	uint8_t warmUpValid; 
	uint8_t loadWarmupOnBoot; //load the warmup parameters from NVM memory on boot. 
	uint8_t loadRangesOnBoot; //
}sensorSettings_t;



//Time conversions defines
#define SECONDS									1000	//converts seconds to milli-seconds
#define MINS									60 * 1000	//converts minutes to milli-seconds

int itoa(int value, char* sp, int radix);
void writeSettings();
status_t loadSettings();

#endif /* COMMON_H_ */