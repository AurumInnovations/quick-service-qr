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
	{MQTT_HOST_IP, "test-mqtt.funicom.com.cn"},
	{MQTT_HOST_PORT, "32517"},
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
	write_profile_int(SET_INI_SECTION , key, val , SET_INI_FILENAME);
}

void get_setting_str(const char * key , char *val , int len)
{
	char *def = getitemdefval( key );
	if ( def == 0 ) def = "";

	read_profile_string(SET_INI_SECTION , key , val , len , def , SET_INI_FILENAME);
}

void set_setting_str(const char *key , const char * val)
{
	char *buff =  getitembuff(key);
	if ( buff != 0 )
	{
		strcpy(buff,val);
	}
	write_profile_string(SET_INI_SECTION , key, val , SET_INI_FILENAME);
}

void set_func_init(void)
{
	int i;
	int init = get_setting_int("Init");
	if ( init != 1){
		for (i = 0 ; i < sizeof(m_set_str) / sizeof(st_setting_def) ; i ++){
			set_setting_str(m_set_str[i].key , m_set_str[i].val);
		}
		set_setting_int("Init" , 1);
	}
}


//////////////////////////////////////////////////////////////////////////

#define FUNIMP(funname,len ) \
	static char funname[len + 1] = {0}; \
	const char * get##funname(){ \
		if ( strlen(funname) == 0 )\
		{\
			get_setting_str( "s"#funname ,funname, len + 1);\
		}\
		return funname;\
	}\
	void set##funname(const char *val){ strcpy(funname,val);set_setting_str( "s"#funname,val); }


FUNIMP( CurrentUserName,2 ) 
FUNIMP( MerchantID,15 ) 
FUNIMP( TerminalID,8 ) 
FUNIMP( MerchantName,40 ) 
FUNIMP( SerialNum,6 ) 
FUNIMP( BatchNum,6 ) 

//////////////////////////////////////////////////////////////////////////
#define  BUFF( funname  )  "s"#funname,funname

static char *s_itemBuff[] = {
	BUFF( CurrentUserName ),
	BUFF( MerchantID ),
	BUFF( TerminalID ),
	BUFF( MerchantName ),
	BUFF( SerialNum ),
	BUFF( BatchNum ),
};

static char *getitembuff(const char *key)
{
	int i = 0;
	for ( ; i < sizeof(s_itemBuff)/sizeof(s_itemBuff[0]) ; i+=2 )
	{
		if ( strcmp( key,s_itemBuff[i]) == 0 )
		{
			return s_itemBuff[i+1];
		}
	}
	return 0;
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