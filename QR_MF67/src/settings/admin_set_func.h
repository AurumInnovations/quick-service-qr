#pragma once

#define TERM_MERCHANT_ID		"sMerchantID"
#define TERM_TERMINAL_ID		"sTerminalID"
#define TERM_MERCHANT_NAME		"sMerchantName"
#define TERM_SERIAL_NUM			"sSerialNum"
#define TERM_BATCH_NUM			"sBatchNum"

#define MQTT_HOST_IP			"sMqttHostIp"
#define MQTT_HOST_PORT			"nMqttHostPort"
#define MQTT_HOST_OPEN			"nMqttHostOpen"
#define MQTT_HOST_INIT			"nMqttHostInit"
#define MQTT_HOST_UPDATE		"nMqttHostUpdate"
#define MQTT_CLIENT_ID			"sMqttClientID"
#define	MQTT_PASSWORD			"sMqttPassword"
#define MQTT_PRODUCT_KEY		"sMqttProductKey"
#define MQTT_PRODUCT_SECRET		"sMqttProductSecret"
#define MQTT_HEART_TIME			"nMqttHeartTime"

#define SERIAL_MODE             "SerialMode"

long long get_setting_longlong(const char * key);
int  get_setting_int(const char * key);
void set_setting_int(const char *key , int val);
void get_setting_str(const char * key , char *val , int len);
void set_setting_str(const char *key ,const  char * val);

void set_func_init(void);

const char * getCurrentUserName();
void setCurrentUserName( const char *val );


const char * getMerchantID();
void setMerchantID( const char *val );


const char * getTerminalID();
void setTerminalID( const char *val );


const char * getMerchantName();
void setMerchantName( const char *val );


const char * getSerialNum();
void setSerialNum( const char *val );


// const char * getBatchNum();
// void setBatchNum( const char *val );
// 
// 
// const char * getAcqName();
// void setAcqName( const char *val );

void admin_set_mqtt_open(int mOpen);
