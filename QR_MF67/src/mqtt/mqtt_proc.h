#ifndef __MQTT_PROC_H__
#define __MQTT_PROC_H__

typedef enum {
	status_WaitNetwork,
	status_Connecting,
	status_Recving,
	status_Paused,
}e_status;

int mqtt_proc_init();
int mqtt_proc_Pause();
int mqtt_proc_Resume();

const char * mqtt_status_text();

e_status mqtt_status();

void resetMQTT();

#endif