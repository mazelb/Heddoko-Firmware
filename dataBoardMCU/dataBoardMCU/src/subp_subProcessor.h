/*
 * subp_subProcessor.h
 *
 * Created: 6/20/2016 10:33:59 AM
 *  Author: sean
 */ 


#ifndef SUBP_SUBPROCESSOR_H_
#define SUBP_SUBPROCESSOR_H_

#include "common.h"

#define PACKET_QUEUE_LENGTH 10

void subp_subProcessorTask(void *pvParameters);

#endif /* SUBP_SUBPROCESSOR_H_ */