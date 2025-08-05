#include "admin_set_func.h"
#include "pub/osl/inc/osl_time.h"
#include "pub\osl\inc\osl_BaseParam.h"

#define SET_INI_FILENAME	"xxxx\\setting.ini"
#define SET_INI_SECTION		"set"

//#define SET_TEST 1

typedef  const struct _st_setting_def{
	char * key;
	char * val;
}st_setting_def;

static char *getitembuff(const char *key);
static char *getitemdefval(const char *key);

static const st_setting_def m_set_str[]={
	{ "Init",			"0",				},
#ifdef DEV_MF67_A10
	//Setting Parameters
	{SERIAL_MODE, "0"},
#endif
	//MQTT Parameters
	{MQTT_HOST_IP, "192.168.86.249"},
	{MQTT_HOST_PORT, "1883"},
	{MQTT_HOST_OPEN, "1"},
	{MQTT_HOST_INIT, "1"},
	{MQTT_HOST_UPDATE, "1"},
};

extern long long int atoll(const char* s);

int get_setting_int(const char * key)
{
	char value[32] = {0};
	get_setting_str( key,value,sizeof(value) );	
	return atoi(value);
}
long long get_setting_longlong( const char * key )
{
	char value[32] = {0};
	get_setting_str( key,value,sizeof(value) );	
	return atoll(value);
}

void set_setting_int(const char *key , int val)
{
	write_profile_int(SET_INI_SECTION , key, val , SYSTEM_PROFILE);
}

void get_setting_str(const char * key , char *val , int len)
{
	char *def = getitemdefval( key );
	if ( def == 0 ) def = "";

	read_profile_string(SET_INI_SECTION , key , val , len , def , SYSTEM_PROFILE);
}

 void set_setting_str(const char *key , const char * val)
 {
 	write_profile_string(SET_INI_SECTION , key, val , SYSTEM_PROFILE);
 }

static char *getitemdefval(const char *key)
{
	int i = 0;
	for ( ; i < sizeof(m_set_str)/sizeof(m_set_str[0]) ; i++ )
	{
		if ( strcmp( key,m_set_str[i].key ) == 0 )
		{
			return m_set_str[i].val;
		}
	}
	if ( key[0] == 'b' || key[0] == 'n')
	{
		return "0";
	}

	return 0;
}

void admin_set_mqtt_open(int mOpen)
{
	set_setting_int(MQTT_HOST_OPEN, mOpen);
}