/*
 * mgr_managerTask.h
 *
 * Created: 2/29/2016 3:49:33 PM
 *  Author: sean
 */ 


#ifndef MGR_MANAGERTASK_H_
#define MGR_MANAGERTASK_H_

//enumeration for the power board states
typedef enum
{
	SYS_STATE_POWER_ON, 
	SYS_STATE_POWER_OFF_CHARGING,
	SYS_STATE_POWER_OFF	
}mgr_systemStates_t;

//enumeration of events
typedef enum
{
	SYS_EVENT_POWER_SWITCH,
	SYS_EVENT_JACK_DETECT,
	SYS_EVENT_LOW_BATTERY,
	SYS_EVENT_POWER_UP_COMPLETE,
	SYS_EVENT_USB_CONNECTED,
	SYS_EVENT_USB_DISCONNECTED,
	SYS_EVENT_POWER_DOWN_COMPLETE
}mgr_systemEvents_t;

//structure of event message
typedef struct
{
	mgr_systemEvents_t sysEvent;
	uint16_t data;
}mgr_eventMessage_t;

void mgr_managerTask(void *pvParameters);
mgr_systemStates_t mgr_getState();

#endif /* MGR_MANAGERTASK_H_ */