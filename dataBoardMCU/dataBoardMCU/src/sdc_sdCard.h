/*
 * sdc_sdCard.h
 *
 * Created: 5/24/2016 11:18:08 AM
 * Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#ifndef SDC_SDCARD_H_
#define SDC_SDCARD_H_
#include <asf.h>

typedef struct 
{
	
	FIL fileObj,
	bool fileOpen,
	uint8_t* bufferPointerA,
	uint8_t* bufferPointerB,
		
}sdc_file_t;



#endif /* SDC_SDCARD_H_ */