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

#define FIRMWARE_BLOCK_SIZE 512

void __attribute__((optimize("O0"))) runBootloader(); 

#endif /* BOOTLOADER_H_ */