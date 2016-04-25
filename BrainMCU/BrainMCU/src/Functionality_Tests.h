/*
 * Functionality_Tests.h
 *
 * Created: 9/16/2015 3:22:49 PM
 *  Author: Hriday Mehta
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 


#include <asf.h>

#ifndef FUNCTIONALITY_TESTS_H_
#define FUNCTIONALITY_TESTS_H_
/*	Error Codes	*/
#define SUCCESS			0
#define INVALID_DRIVE	1
#define CANNOT_OPEN		2
#define WRITE_FAILED	3
#define READ_FAILED		4
#define CANNOT_CLOSE	5
#define UNKNOWN_ERROR	255
int SDWriteTest	(void);
int SDReadTest	(void);


#endif /* FUNCTIONALITY_TESTS_H_ */