/*
 * mgr_managerTask.h
 *
 * Created: 2/29/2016 3:49:33 PM
 *  Author: sean
 */ 


#ifndef MGR_MANAGERTASK_H_
#define MGR_MANAGERTASK_H_

//enumeration of events
typedef enum
{
	SYS_EVENT_POWER_SWITCH,
	SYS_EVENT_JACK_DETECT,
	SYS_EVENT_LOW_BATTERY,
	SYS_EVENT_POWER_UP_COMPLETE
}mgr_systemEvents_t;

//structure of event message
typedef struct
{
	mgr_systemEvents_t sysEvent;
	uint16_t data;
}mgr_eventMessage_t;

void mgr_managerTask(void *pvParameters);

#endif /* MGR_MANAGERTASK_H_ */