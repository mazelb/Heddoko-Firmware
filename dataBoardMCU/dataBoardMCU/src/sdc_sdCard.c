/*
 * sdc_sdCard.c
 *
 * Created: 5/24/2016 11:17:49 AM
 * Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


//Static function forward declarations
bool sdCardPresent();
//initializes the SD card, and mounts the drive
status_t initializeSdCard();
//uninitializes the SD card, and puts the drive in an uninitialized state. 
status_t unInitializeSdCard();


