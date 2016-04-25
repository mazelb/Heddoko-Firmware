/**
 * \file
 *
* Copyright Heddoko(TM) 2015, all rights reserved
 * \brief 
 *
 */
/*
 * Board_Init.h
 *
 * Created: 9/16/2015 5:35:13 PM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#include <asf.h>

#ifndef BOARD_INIT_H_
#define BOARD_INIT_H_


void powerOnInit(void);
void initAllUarts();
void initAllTwi();
void deInitAllUarts(); 

#endif /* BOARD_INIT_H_ */