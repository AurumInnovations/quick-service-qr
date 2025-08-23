#include "ucosii\ucos_ii.h"
#include "mqtt_proc.h"
#include "mqtt_embed/MQTTClient/src/MQTTClient.h"
#include "../app_def.h"

#define SET_INI_SECTION			"set"

#define DEFAULT_PRODUCT_KEY			"p33zc05ydKrIERdd"
#define DEFAULT_PRODUCT_SECRET		"s8cZ75jxQSG5uh2q"
#define DEFAULT_MQTT_HEART_TIME	5*60

typedef struct st_mqtt_config
{
	//int plamtform;
	int keepAliveInterval;
	int port;
	char ip[64];
	char hmac_key[17];
	char hmac_secret[17];
	char clientID[32];
	char username[32];
	char mqPassword[128];
	char topic[128];//make topic buffer big enough to receive
}st_mqtt_config;

static st_mqtt_config g_mqttConfig;
static MQTTClient c;
static Network n;
static e_status s_status;
static int s_paused;
static unsigned char sendbuf[256];
static unsigned char readbuf[512];
static unsigned char s_statustext[64];
static MQTTClient *s_client=0;

static int m_payload_lock = -1;
#define MQTT_LOCK	 if(m_payload_lock == -1){m_payload_lock =(int)OSSemCreate((unsigned short)1);}  {unsigned char nErr; OSSemPend((OS_EVENT *)m_payload_lock,TIMEOUT_INFINITE, &nErr); }
#define	MQTT_UNLOCK	 OSSemPost((OS_EVENT *)m_payload_lock);

void resetMQTT()
{
    APP_TRACE("resetMQTT");
    if(s_client != 0)
    {
      MQTTDisconnect(s_client);
      s_client = 0;
      NetworkDisconnect(&n); 
    }
}

static void json_get_str_by_key(cJSON* root, const char *key,char *val,int len)
{
  int len_temp = 0;
  cJSON* temp = cJSON_GetObjectItem(root, key);

  if(temp != NULL)
  {
    len_temp = strlen(temp->valuestring);
    len_temp = len_temp > len ? len : len_temp;
    memcpy(val,temp->valuestring,len_temp);
  }
}

static int json_get_int_by_key(cJSON* root, const char *key)
{
  int ret = 0;
  cJSON* temp = cJSON_GetObjectItem(root, key);

  if(temp != NULL)
  {
    ret = atoi(temp->valuestring);
  }

  return ret;
}

//QR payload example
//{"amt":"1.00","qrCode":"https://www.google.com/","showTime":"10","orderNo":"1660891251"}
void mqtt_comm_messageArrived_qr(MessageData* md)
{   
  MQTTMessage* m = md->message;

  if ( m->payloadlen>0)
  {
    MQTT_LOCK
    unsigned char *s_payload = 0;
    unsigned int s_payloadlen = 0;

    s_payloadlen= m->payloadlen;
    s_payload = MALLOC(s_payloadlen+1);	
    memcpy(s_payload, (char*)m->payload ,s_payloadlen );
    s_payload[s_payloadlen] = 0;
    APP_TRACE("mqtt_comm_messageArrived_qr recv:\r\n %s\r\n",s_payload);

    cJSON* proot = cJSON_Parse(s_payload);
    FREE(s_payload);
    if(proot != NULL)                   
    {                            
      st_qr_data* mpos_qr_data = mpos_func_get_qr_data();

      json_get_str_by_key(proot, "qrCode", mpos_qr_data->qrdata, sizeof(mpos_qr_data->qrdata));                                
      json_get_str_by_key(proot, "amt", mpos_qr_data->amt, sizeof(mpos_qr_data->amt));
      json_get_str_by_key(proot, "text", mpos_qr_data->text, sizeof(mpos_qr_data->text));
      mpos_qr_data->showTime = json_get_int_by_key(proot, "showTime");                                
    }

    cJSON_Delete(proot);
    MQTT_UNLOCK
    }
}

int mqtt_proc_Pause()
{
	s_paused = 1;
	while ( s_status == status_Recving )
	{
		Sys_Sleep(100);
	}

}

int mqtt_proc_Resume()
{
	s_paused = 0;
}

static int HMACcalculate(char *in, char* key, char * out)
{ 
	unsigned char md[32]; //32 bytes
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type( MBEDTLS_MD_SHA256 );

	mbedtls_md_hmac(md_info,(const unsigned char*)key, strlen(key), (const unsigned char*)in , strlen(in), md);
	Ex_Str_HexToAsc(md, 64, 0, (unsigned char*)out);

	return 0;
}

void mqtt_config_init()
{
	char cid[32] = {0};
	char hmac_payload[128] = {0};

	memset(&g_mqttConfig, 0x00, sizeof(st_mqtt_config));

	g_mqttConfig.keepAliveInterval = get_setting_int(MQTT_HEART_TIME);
	if (g_mqttConfig.keepAliveInterval <= 0)
	{
		g_mqttConfig.keepAliveInterval = DEFAULT_MQTT_HEART_TIME;
	}

	get_setting_str(MQTT_PRODUCT_KEY, (char*)g_mqttConfig.hmac_key, 16);
	if (strlen(g_mqttConfig.hmac_key) < 3)
	{
		strcpy((char*)g_mqttConfig.hmac_key, DEFAULT_PRODUCT_KEY);
	}

	get_setting_str(MQTT_PRODUCT_SECRET, (char*)g_mqttConfig.hmac_secret, 16);
	if (strlen(g_mqttConfig.hmac_secret) < 3)
	{
		strcpy((char*)g_mqttConfig.hmac_secret, DEFAULT_PRODUCT_SECRET);
	}

	memset(cid, 0x00, sizeof(cid));
	Sys_GetTermSn(cid);
	if (strlen(cid) < 3)
	{
		memcpy(cid, "202302060000001", 15);
	}

	strcpy((char*)g_mqttConfig.clientID, cid);
	strcpy((char*)g_mqttConfig.username, cid);
	snprintf(g_mqttConfig.topic, sizeof(g_mqttConfig.topic), "/ota/%s/%s/update", g_mqttConfig.hmac_key, g_mqttConfig.clientID);
	
	memset(hmac_payload, 0, sizeof(hmac_payload));
	sprintf(hmac_payload, "clientId{%s}.{%s}%s%s%s", 
		g_mqttConfig.clientID, g_mqttConfig.clientID, g_mqttConfig.clientID, g_mqttConfig.hmac_key, "2524608000000");
	HMACcalculate(hmac_payload, (char*)g_mqttConfig.hmac_secret, g_mqttConfig.mqPassword);
	APP_TRACE("mqtt payload:%s", hmac_payload);
	//APP_TRACE_BUFF_TIP(g_mqttConfig.mqPassword, 128, "password");
}

static int mqtt_comm_run() 
{                
	int rc = 0;
	char ip[64]={0};
	int port= 0;
	static int s_show_once = 0;
	
  strcpy(s_statustext,"Connecting..");
	s_status = status_Connecting;

	APP_TRACE("mqtt_comm_run Connecting\r\n");
	get_setting_str(MQTT_HOST_IP,ip,sizeof(ip));
	port =get_setting_int(MQTT_HOST_PORT);

	memset(&n,0x00,sizeof(n));
	n.tls = 1;
  NetworkInit(&n);       
#ifndef CLIENT_CRT
	rc = NetworkConnect(&n, ip, port);
 #else

#endif
	APP_TRACE("mqtt_comm_run rc from net:%d\r\n", rc);
                                     
	if (rc == 0)
  {
    char topic0[64] = {0};
    cJSON *root;
    char *out;
    char curr_date[20] = {0};
    char curr_time[10] = {0};
    char dsn[32] = {0};

    Sys_Sleep(300);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    MQTTClientInit(&c, &n, 20000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

    get_yyyymmdd_str(curr_date);
    get_hhmmss_str(curr_time);   
    strcat(curr_date, curr_time);                         
    root=cJSON_CreateObject();
    cJSON_AddStringToObject(root,"type","ping");
    cJSON_AddStringToObject(root,"time",curr_date);
                
    out=cJSON_Print(root);
    cJSON_Delete(root);

    Sys_GetTermSn(dsn);
    data.clientID.cstring = dsn;
    data.keepAliveInterval = g_mqttConfig.keepAliveInterval;
    data.cleansession = 0;
    data.username.cstring = g_mqttConfig.username;
    data.password.cstring = g_mqttConfig.mqPassword;
    data.willFlag         = 1;
    data.will.topicName.cstring = g_mqttConfig.topic;
    data.will.message.cstring   = out;
    data.will.retained          = 0;
    data.will.qos              =  0;

    rc = MQTTConnect(&c, &data);
    s_client = &c;
    APP_TRACE("mqtt_comm_run rc from connect:%d\r\n", rc);		                              
                
    rc = MQTTSubscribe(&c, g_mqttConfig.topic, 0, mqtt_comm_messageArrived_qr);
     
    Sys_Delay(1000);  
                
    if(rc == 0) 
    {
      APP_TRACE("mqtt subscribe success!\r\n");  
      while( MQTTIsConnected(&c) )
      {					
        rc = MQTTYield(&c, 1000);
        if ( rc != SUCCESS)
        {
          break;
        }
        Sys_Sleep(300);
        if ( s_paused == 1)
        {
          break;
        }
      }
		  MQTTDisconnect(&c);
    }                
    else
    {
		  sprintf(s_statustext,"Connect,%d",rc);
    }
    NetworkDisconnect(&n);
	}	
	else
  {
    printf(s_statustext,"Network anomaly,%d",rc);
    if (s_show_once == 0)
    {
				s_show_once = 1;
				gui_messagebox_show("Device", "Disconnected", "", "confirm" , 3000);
    }
  }
    
	s_client = 0;
	return 0;
}

static void mqtt_comm_task(void * p) 
{
	mqtt_config_init();
  while(1)
	{
		if ( s_paused == 1)
		{
			if ( s_status != status_Paused )
			{
				s_status = status_Paused;
				sprintf(s_statustext,"Paused..");
			}
			Sys_Sleep(100);
			continue;
		}
		//Wait one second and try again
		Sys_Sleep(1000);

		//Waiting network..
		if ( net_func_link_state() == 0 ){
			sprintf(s_statustext,"Wait network..");
			s_status = status_WaitNetwork;
			Sys_Sleep(1000);
			continue;
		}
    
    if(net_func_link_state() == 1)
    {	
      mqtt_comm_run(); //mqtt run
    }
    else
    {           
			Sys_Sleep(1000);
		}
	}
}

#pragma data_alignment=8
#define _MQTT_TASK_SIZE		1024
#define _MQTT_TASK_PRIO		(_APP_TASK_MIN_PRIO + 4)
static unsigned int pTaskStk[_MQTT_TASK_SIZE];

int mqtt_proc_init()
{
	s_paused = 0;
	s_status = status_Connecting;

	sprintf(s_statustext,"Loading..");

	osl_task_create((void*)mqtt_comm_task,_MQTT_TASK_PRIO,pTaskStk ,_MQTT_TASK_SIZE );
        
	return 0;
}


