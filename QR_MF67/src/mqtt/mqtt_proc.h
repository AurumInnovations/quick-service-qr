#ifndef __MQTT_PROC_H__
#define __MQTT_PROC_H__

#include "../mpos_func.h"

typedef enum {
	status_Connecting,
	status_WaitNetwork,
	status_Recving,
	status_Paused,
}e_status;

int mqtt_proc_init();
void mqtt_config_init();
void resetMQTT();
void mqtt_get_status_text(char* buffer, int len);
e_status mqtt_get_status(void);

#endif