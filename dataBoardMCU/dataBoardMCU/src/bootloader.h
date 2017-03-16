/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
/*
 * bootloader.h
 *
 * Created: 11/9/2015 8:12:23 AM
 *  Author: sean
 */ 


#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_
#define FIRMWARE_FILE_HEADER_BYTES 0x55AA55AA
typedef struct
{
    uint32_t fileHeaderBytes;
    uint32_t dbBinLength;
    uint32_t dbCRC;
    uint32_t pbBinLength;
    uint32_t pbCRC;
}firmwareHeader_t;

void runBootloader(); 

#endif /* BOOTLOADER_H_ */