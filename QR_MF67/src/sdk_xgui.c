#include "app_def.h"
#include "driver/mf_rtc.h"
#include "pub/common/misc/inc/mfmalloc.h"
#include "pub/osl/inc/osl_BaseParam.h"
#include "test/test_pub.h"
#include "driver/mf_supply_power.h"
#include "atc/inc/atc_pub.h"
#include "sdk_ntag.h"
#include "sdk_driver.h"


LIB_EXPORT int tms_update(const char *file);

#define LOGOIMG "data\\logo.bmp"

#define MAIN_MENU_PAGE	"Main"


#define XM_FILE_DOWNLOAD_PAGE			0x000E0100
static unsigned int nDPressTick = 0;
static int msg_arrived = 0;

extern int g_playtag;
extern int g_show_qr;
extern int g_ntag_init;

enum{
	DRIVER_TYPE_MAG,
	DRIVER_TYPE_ICC,
	DRIVER_TYPE_RF,
};

static st_gui_menu_item_def _menu_def[] = {
	{MAIN_MENU_PAGE ,	"Test",			""},
	{MAIN_MENU_PAGE ,	"Version",		""},
	{MAIN_MENU_PAGE ,	"Settings",		""},

	{"Test" ,	"DevInfo",		""},
#ifdef DEV_MF67_A10
	{"Test",	"RF Card",		""},
#endif
#ifndef PN5180
	{"Test",	"M1 Card",		""},
#endif
	{"Test",	"Felica Card",	""},
#ifdef SOCKET_SUPPORT
	{"Test",	"Http",		    ""},
	{"Test",	"Https",		""},
#endif
	{"Test",	"ShowQr",		""},
#ifdef DEV_MF67_A10
	{"Test",	"ScanCode",		""},
#endif
	{"Test",	"File",		    ""},
	//{"Test",	"ShowBmp",		""},	
	{"Test",    "Led",          ""},
	{"Test",    "Voice",        ""},
    {"Test",    "Buzzer",       ""},
	{"Test",    "Bluetooth",    ""},
	{"Test",    "SerialPort",   ""},
#ifdef DEV_MF67_A10
	{"Test",    "TMS",   ""},
#endif
#ifdef PN5180
	{"Test",    "NTAG test",   ""},
#endif
#ifdef SOCKET_SUPPORT
	{"Settings",	"Wifi Set",		"Remote set"},
	{"Settings",	"Wifi Scan",	""},
#endif
#ifdef DEV_MF67_A10
	{"Settings",	"Net Select",	""},
	{"Settings",	"Serial Mode",	""},
#endif
	{"Settings",	"Key Sound",	""},
	{"Settings",	"TTS Volume",	""},
	{"Settings",	"Lcd Light",	""},
	{"Settings",	"Power Time",	""}, 
	{"Settings",	"Open Log",		""},	
};

void set_msg_arrived(int arrived)
{
	msg_arrived = arrived; 
}

static int getversions( char *buff)
{
	int i = 0;

	i += sprintf(buff + i, "Api:%s\r\n", libapi_version());
	i += sprintf(buff + i, "Apppub:%s\r\n", apppub_version());
	i += sprintf(buff + i, "Atc:%s\r\n", atc_version());
	i += sprintf(buff + i, "Json:%s\r\n", json_version());
	i += sprintf(buff + i, "Net:%s\r\n", net_version());
	i += sprintf(buff + i, "Power:%s\r\n", power_version());
	i += sprintf(buff + i, "Producttest:%s\r\n", producttest_version());
	i += sprintf(buff + i, "Pub:%s\r\n", pub_version());
	i += sprintf(buff + i, "Wifi:%s\r\n", wifi_version());
	i += sprintf(buff + i, "Xgui:%s\r\n", xgui_version());

	return i;
}

int extract_value(const char *input, const char *key, char *output, size_t output_size) {
	if (!input || !key || !output || output_size == 0) {
		return -1; // parameter error
	}

	char *copy = strdup(input);
	if (!copy) {
		return -1;
	}

	char *saveptr = NULL;
	char *token = strtok(copy, ";");
	int found = 0;

	while (token != NULL) {
		char *colon = strchr(token, ':');
		if (colon) {
			*colon = '\0'; 
			char *current_key = token;
			char *current_value = colon + 1;

			if (strcmp(current_key, key) == 0) {
				strncpy(output, current_value, output_size - 1);
				output[output_size - 1] = '\0';
				APP_TRACE("extract_value key[%s]:%s", key, output);
				found = 1;
				break;
			}
		}
		token = strtok(NULL, ";");
	}

	free(copy);
	return found ? 0 : -1;
}

void wifi_connect_scan()
{
	st_wifi_ap_list ap_list[16]={0};
	int count = 0;
	int index = 0;
	int ret = 0;
	int ret2 = 0;
	char ssid[64] = {0};
	char pwd[64] = {0};
	char* scanCode = NULL;

	scanCode = MALLOC(256);
	memset(scanCode, 0, 256);
	ret = scan_page_proc("Scan" , scanCode,  256, 5000);
	APP_TRACE("================= scancode:%s, ret:%d =================", scanCode, ret);

	ret = extract_value(scanCode, "S", ssid, sizeof(ssid));
	ret2 = extract_value(scanCode, "P", pwd, sizeof(pwd));

	FREE(scanCode);
	if (ret == 0 && ret2 == 0)
	{
		count = comm_wifi_list_ap(ap_list);
		APP_TRACE("================= count wifi:%d =================", count);
		for(index = 0; index < count ; index++)
		{
			APP_TRACE("================= index[%d],ssid:%s =================", index, ap_list[index].ssid);
			if (strcmp(ssid, ap_list[index].ssid) == 0)
				break;
		}
		APP_TRACE("================= ssid:%s =================", ap_list[index].ssid);
		ret = comm_wifi_link_ap(&ap_list[index], pwd);
		APP_TRACE("===== wifi connect ret %d =====", ret);
		if (ret == 0)
		{
			char msg[128] = {0};
			int index = 0;

			index += sprintf(msg, "Wifi:%s\n", ssid);
			index += sprintf(msg + index, "Connect success!");
			
			gui_messagebox_show("Prompt", msg, "", "OK", 1000 * 10);
		}
		Sys_Delay(100);
	}
	else
	{
		gui_messagebox_show("Error", "Incorrect code, please check and try again", "", "OK", 1000 * 10);
	}
}

static int _menu_proc(char *pid)
{
	int ret;
	char buff[20];
	int pos = 0;

	if (strcmp(pid , "Version") == 0)
	{	
		char sversion[256]={0};
		char btname[32]={0};
		char tid[33] = {0};

		comm_get_bt_name(btname);

		getversions(sversion);
#ifndef DATAVAN
		gui_messagebox_show( "Bluetooth Name" , btname, "" , "confirm" , 0);
#endif
		gui_messagebox_show( "App Version" , (char *)APP_VISION , "" , "confirm" , 0);
		gui_messagebox_show( "SDK Version" , sversion, "" , "confirm" , 0);
	}
	else if (strcmp(pid , "DevInfo") == 0)
	{
		test_devinfo();
		//rsa_test();
	}
	else if (strcmp(pid , "M1 Card") == 0)
	{
		sdk_test_m1();
	}
	else if (strcmp(pid, "RF Card") == 0)
	{
		sdk_test_rf();
	}
	else if (strcmp(pid, "Felica Card") == 0)
	{
		sdk_test_felica();
	}
	else if (strcmp(pid , "Key Sound") == 0)
	{
		keySoundSet_Show_test();
	}
        else if(strcmp(pid , "TTS Volume") == 0)
        {
                tts_volume_page();
        }
	else if (strcmp(pid , "Lcd Light") == 0)
	{
		lcdLightSet_Show();
	}
	else if (strcmp(pid , "Power Time") == 0)
	{
		PowerDownTimeSet_Show();
	}
	else if (strcmp(pid , "ShowQr") == 0)
	{
		showQr2("0.01");
	}
#ifdef DEV_MF67_A10	
	else if(strcmp(pid, "ScanCode") == 0)
	{
		upay_barscan();
	}
#endif
#ifdef SOCKET_SUPPORT
	else if (strcmp(pid , "Http") == 0)	{
		sdk_http_test();
	}
	else if (strcmp(pid , "Https") == 0)	{
		sdk_https_test();
	}	
#endif
	else if (strcmp(pid , "File") == 0){
		fileTest(); 
	}
	else if (strcmp(pid , "ShowBmp") == 0)
	{
		set_ntag_url("https://www.google.com/");
		showbmptest();
	}
	else if(strcmp(pid, "Led") == 0){
		sdk_driver_led();
	}
	else if (strcmp(pid, "Open Log") == 0)
	{
		LogOutSet_Show();
	}
	else if (strcmp(pid, "Voice") == 0)
	{
		VoiceTest();
	}
	else if (strcmp(pid, "Bluetooth") == 0)
	{
		BluetoothTest();
	}
	else if (strcmp(pid, "SerialPort") == 0)
	{
		SerialPortTest();
	}
	else if (strcmp(pid, "Serial Mode") == 0)
	{
		SerialModeSwitch();
	}
#ifdef SOCKET_SUPPORT
	else if (strcmp(pid, "Wifi Set") == 0)
	{
		gui_main_menu_show_ex("Wifi Menu", "Wifi Menu", 60000);
	}
	else if (strcmp(pid, "Wifi Scan") == 0)
	{
		wifi_connect_scan();
	}
#endif
    else if(strcmp(pid, "Buzzer") == 0)
    {
         Util_Beep(2);
		 gui_messagebox_show("Buzzer" , "Buzzer Ok or not", "" , "confirm" , 30000);
    }
	else if(strcmp(pid, "Net Select") == 0){
		//gui_main_menu_show_ex("Net Select", "Net Select", 60000);
	}
#ifdef DEV_MF67_A10
	else if(strcmp(pid, "TMS") == 0){
		argot_action("#1#");
	}
#ifdef PN5180
	else if (strcmp(pid, "NTAG test") == 0)
	{
		PN5180_ntag_test();
	}
#endif
#endif
	return 0;
}




void get_yyyymmdd_str(char *buff)
{
	struct rtc_time dt;
	char tmp[20]={0};
	Sys_GetDateTime(tmp);
	sscanf(tmp,"%04d%02d%02d",&dt.tm_year, &dt.tm_mon, &dt.tm_mday);
	sprintf(buff , "%04d-%02d-%02d", dt.tm_year, dt.tm_mon, dt.tm_mday);
}


void get_hhmmss_str(char *buff)
{
	struct rtc_time date_time;
	char tmp[20]={0};
	Sys_GetDateTime(tmp);
	sscanf(tmp,"%04d%02d%02d%02d%02d%02d",&date_time.tm_year, &date_time.tm_mon, &date_time.tm_mday,&date_time.tm_hour, &date_time.tm_min, &date_time.tm_sec);
	sprintf(buff, "%02d:%02d:%02d", date_time.tm_hour, date_time.tm_min, date_time.tm_sec);
}

void standby_pagepaint()
{
	if (msg_arrived)
	{	
		int ret = gui_messagebox_show("", "Message Arrived", "", "", 60000);
		if (ret == 1)
			msg_arrived = 0;
	}
	else
	{
		char data[32]={0};
		int logowidth;
		int logoheight;
		int logoleft;
		int logotop;
		int logocolor;

		char * pbmp;	
		gui_begin_batch_paint();
		gui_set_win_rc();
		gui_clear_dc();
#if 1
		{
			pbmp = (char *)gui_load_bmp_ex(LOGOIMG , &logowidth , &logoheight, &logocolor);

			logoleft = (gui_get_width()-logowidth)/2;

			logotop = GUI_LINE_TOP(1);

			if (pbmp != 0){
				gui_out_bits_ex(logoleft, logotop, pbmp , logowidth , logoheight , 0 , logocolor);
				FREE(pbmp);
			}
		}
#endif
#ifndef DEV_MF67_A10
		get_yyyymmdd_str(data);	
		gui_textout_line_center(data, GUI_LINE_TOP(8));
		get_hhmmss_str(data);	
		gui_textout_line_center(data, GUI_LINE_TOP(9));
#else 
		get_yyyymmdd_str(data);	
		gui_textout_line_center(data, GUI_LINE_TOP(6));
		get_hhmmss_str(data);	
		gui_textout_line_center(data, GUI_LINE_TOP(7));	
#endif
		gui_end_batch_paint();
	}
	
}
/*
static int get_power()
{ 
	battery_attr_t batterAttr;
	int ret;
	int state = -1;

	ret = Sys_power_battery_attr(&batterAttr);	

	if (ret < 0){
		return state;
	}

	if (batterAttr.status == POWER_SUPPLY_STATUS_CHARGING){
		state = 5;
	}
	else{
		state = batterAttr.voltage_level-1;
		if (state < 0 || state > 5){
			state = 0;
		}
	}

	return state;
}
*/

//key press message。falg:1 key down?0 key up
int d_key_proc_func(int key ,int flag)
{
	if (key == GUI_KEY_DOWN || key == GUI_KEY_F2){
		if (flag == 1){
			nDPressTick =  Sys_TimerOpen(3000);	
		}
		else if (flag == 0)	{
			nDPressTick = 0;
		}
	}
	return 0;
}


unsigned int d_key_proc(void*p)
{
	if (nDPressTick > 0 && Sys_TimerCheck(nDPressTick) == 0){
		Sys_ClrKey();
		gui_post_message(XM_FILE_DOWNLOAD_PAGE, 0, 0);
		nDPressTick = 0;
	}
}

static void main_page_set_test_menu(int flag)
{
	write_profile_int("set", "Production test", flag, SYSTEM_PROFILE);
}

static void mpos_text_show(char* text, int text_len)
{
	char txt_temp[17] = {0};
	if (text_len < 17)
	{
		memcpy(txt_temp, text, text_len);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(5));
	}
	else if (text_len > 16 && text_len < 33)
	{
		memcpy(txt_temp, text, 16);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(4));
		memset(txt_temp, 0x00, sizeof(txt_temp));
		memcpy(txt_temp, text + 16, text_len - 16);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(5));
	}
	else if (text_len > 32 && text_len < 49)
	{
		memcpy(txt_temp, text, 16);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(3));
		memcpy(txt_temp, text + 16, 16);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(4));
		memset(txt_temp, 0x00, sizeof(txt_temp));
		memcpy(txt_temp, text + 32, text_len - 32);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(5));
	}
	else
	{
		memcpy(txt_temp, text, 16);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(3));
		memcpy(txt_temp, text + 16, 16);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(4));
		memcpy(txt_temp, text + 32, 16);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(5));
		memset(txt_temp, 0x00, sizeof(txt_temp));
		memcpy(txt_temp, text + 48, text_len - 45);
		gui_textout_line_center(txt_temp, GUI_LINE_TOP(6));
	}
}
#ifdef BLUETOOTH_MODE_SET
//Bluetooth set
static char m_code[16]={0};
static void bluetooth_pairing_func(int mode, char *code)
{
	if(strlen(code) < 15) strcpy(m_code, code);
	gui_post_message(XM_BT_LINK_RND, (unsigned int)m_code, 0);
	APP_TRACE("bt link code:%s\r\n", code);
	//bluetooth_set_gkey_state(1);
}

static void bluetooth_link_func(int mode)
{
	char buff[1]={0};
	st_pkt_info pkt_info = {0};

	pkt_info.func=0x4005;
	pkt_info.ret=0x00;
	pkt_info.seq=0x00;

	buff[0] = 0;
	Sys_Sleep(1000);
	mpos_pub_send_pkt(buff,1, &pkt_info);
	gui_post_message(XM_BT_LINK_SUCC, 0, 0);
	APP_TRACE("bt link mode:%d\r\n", mode);
}
#define INI_FILE	"data\\set.ini"
void data_func_set_pair_flag(int flag)
{
	write_profile_int("pair", "flag1", flag , INI_FILE);	
}

void bt_link_page()
{
	st_gui_message pMsg;
	char str[32]={0};
	char addr[16]={0};
	char name[32]={0};
	char pincode[8];
	unsigned int randnum;

	//bluetooth_set_conn(1);

	//main_set_led2(1, 500);
	Util_Led(1, 1);

	gui_post_message(GUI_GUIPAINT, 0 , 0);

	mf_get_random_number(4, &randnum);
	sprintf(pincode, "%06d", randnum % 1000000);
	bluetooth_set_pincode(pincode);
	_mpos_proc();

	while(1){
		_mpos_proc();
		if (gui_get_message(&pMsg, 10) == 0) {
			if (pMsg.message_id == GUI_GUIPAINT) {
				gui_begin_batch_paint();
				gui_clear_dc();

				bluetooth_get_addr(addr);
				bluetooth_get_name(name);
				//sprintf(str, "MAC:%s", addr);
#ifdef DEV_MF67_A10
				sprintf(str, "ID:%s", name);
				xgui_TextOut_Line_Center(str, 175);

				sprintf(str, "PIN:%s", pincode);
				xgui_TextOut_Line_Center(str, 220);
#else
				sprintf(str, "ID:%s", name);
				gui_textout_line_center(str, 85);

				sprintf(str, "PIN:%s", pincode);
				gui_textout_line_center(str, 110);
#endif

				gui_end_batch_paint();
			}
			//else if(pMsg.MessageId== XM_BT_LINK_RND){
			//	show_bt_rnd_page(pMsg.WParam);
			//	break;
			//}
			else if(pMsg.message_id== XM_BT_LINK_SUCC){
				_mpos_proc();
				data_func_set_pair_flag(1);
				gui_messagebox_show("Tips", "connect success", "", "OK", 10000);
				break;
			}
			else if (pMsg.message_id == GUI_KEYPRESS) {
				if (pMsg.wparam == GUI_KEY_OK ){
					break;
				}
				else if(pMsg.wparam == GUI_KEY_QUIT){
					break;
				}
			}
			else{
				gui_proc_default_msg(&pMsg);
			} 
		}
	}

	//main_set_led2(0, 0);
	Util_Led(1, 0);
}
/////
#endif /* BLUETOOTH_MODE_SET */

extern int g_reset_flag;
void sdk_main_page()
{
	st_gui_message pMsg;
	static int xgui_init_flag = 0;
	char time_cur[20];
	char time_last[20];
	int i;
	int showTime = -1;
	//st_qr_data * qr_data = NULL;
	st_qr_data  * mpos_qr_data = mpos_func_get_qr_data();
 	static char dinit = 0;
        int nTimer;
        unsigned int nErrorCode; 
		
		gui_set_font_mode(1);
		
		if(gui_get_width() == 240){
			gui_set_font(14);
			gui_set_text_zoom(1);
		}
		else{		
			gui_set_font(11);
		}
		

	get_hhmmss_str(time_last);
	APP_TRACE("enter_main page");
#if 1
        main_page_set_test_menu(1);//enable the production test menu
	
        //ntag_proc_init();
        
        sdk_timer_init();
#ifndef WIN32
    if (dinit == 0){
		dinit = 1;
		nTimer = Sys_TimerCreate((void*)d_key_proc, 0 , 100, TIMER_MODE_NORMAL_, &nErrorCode);
		Sys_TimerEnable(nTimer);
		Sys_Key_Proc_Func_Add((void*) d_key_proc_func);
	}
#endif
	if(xgui_init_flag == 0){
		xgui_init_flag = 1;
		gui_main_menu_func_add((void *)_menu_proc);		// Registration menu callback processing
		for(i = 0; i < GUI_MENU_ITEM_COUNT(_menu_def); i ++){	// Add menu items cyclically
			gui_main_menu_item_add(_menu_def + i);	
		}
	}
#endif
 	init_com();
#ifdef BLUETOOTH_MODE_SET
	//Bluetooth set
	bluetooth_set_pairing_func((void*)bluetooth_pairing_func);
	bluetooth_set_link_func((void*)bluetooth_link_func);
	//
#endif
	

	gui_post_message(GUI_GUIPAINT, 0 , 0);
	while(1)
	{
		if (gui_get_message(&pMsg, 5) == 0) 
		{
			if (pMsg.message_id == GUI_GUIPAINT) 
			{
				mpos_qr_data = mpos_func_get_qr_data();
				if(g_reset_flag > 0)
				{
					showTime = mpos_qr_data->showTime;
					g_reset_flag = 0;
					if (g_show_qr == 1)
					{
						g_show_qr = 0;
						Sys_rfid_emulate_deinit();
						g_ntag_init = 0;
					}
				}
				if (strlen(mpos_qr_data->qrdata) > 0)
 				{
					unsigned int tick = Sys_TimerOpen(1000);
					static unsigned int tick1 = 0;
					int showtime = (int)mpos_qr_data->showTime;

					if (showtime && tick1 == 0)
						tick1 = Sys_TimerOpen(showtime * 1000);
					else if (tick1 == 0)
						tick1 = Sys_TimerOpen(30 * 1000);

					if (g_ntag_init == 0)
					{
						g_ntag_init = 1;
						Sys_rfid_emulate_init();
						ntag_config();	
					}
					showQr(mpos_qr_data->amt, mpos_qr_data->qrdata);
					if (Sys_TimerCheck(tick1) == 0)	
					{
						mpos_func_clear_qr_data();
						g_show_qr = 0;
					}
					while(1)
					{
						Sys_rfid_emulate_process();
						if (Sys_TimerCheck(tick) == 0)
						{
							break;
						}
						if (gui_get_message(&pMsg, 5) == 0)
						{
							if (pMsg.message_id == GUI_KEYPRESS)
							{
								if (pMsg.wparam == GUI_KEY_OK || pMsg.wparam == GUI_KEY_QUIT || pMsg.wparam == GUI_KEY_BACKSPACE)
								{
									standby_pagepaint();
									mpos_func_clear_qr_data();
									g_show_qr = 0;
									Sys_rfid_emulate_deinit();
									g_ntag_init = 0;
									break;
								}
							}
						}
					}
 				}
				else if (mpos_qr_data->bmp != NULL)
				{
					unsigned int tick1;
					int x_bmp = 0;
					int y_bmp = 0;
					tick1 = Sys_TimerOpen(mpos_qr_data->showTime * 1000);
					x_bmp = (gui_get_width() - mpos_qr_data->bmpWidth) / 2;
					y_bmp = (gui_get_height() - mpos_qr_data->bmpWidth) / 2;
					
					while(1)
					{
						gui_begin_batch_paint();
						gui_set_win_rc();
						gui_clear_dc();
						gui_out_bits(x_bmp, y_bmp, mpos_qr_data->bmp , mpos_qr_data->bmpWidth , mpos_qr_data->bmpWidth , 0);
						gui_end_batch_paint();
						if (Sys_TimerCheck(tick1) == 0)	
						{
							mpos_func_clear_qr_data();
							break;
						}
						if (gui_get_message(&pMsg, 5) == 0)
						{
							if (pMsg.message_id == GUI_KEYPRESS)
							{
								if (pMsg.wparam == GUI_KEY_OK || pMsg.wparam == GUI_KEY_QUIT || pMsg.wparam == GUI_KEY_BACKSPACE)
								{
									mpos_func_clear_qr_data();
									standby_pagepaint();
									break;
								}
							}
						}
						Sys_Delay(200);
					}
				}
 				else if(strlen(mpos_qr_data->text) > 0 && showTime>0)
 				{                                       
                    gui_begin_batch_paint();
					gui_set_win_rc();
					gui_clear_dc();
					mpos_text_show(mpos_qr_data->text, strlen(mpos_qr_data->text));
					gui_end_batch_paint();
                                        
                    if ((strstr(mpos_qr_data->text, "Success")!=0 || strstr(mpos_qr_data->text, "success")!=0) && g_playtag == 1)
					{
						Play_Voice("payok");
					} 
					else if ((strstr(mpos_qr_data->text, "Fail")!=0 || strstr(mpos_qr_data->text, "Fail")!=0) && g_playtag == 1)
					{
						Play_Voice("payerr");
					} 
					g_playtag = 0;
 				}
 				else
				{
					standby_pagepaint();	
				}
			}
			else if (pMsg.message_id == GUI_KEYPRESS) 
			{
				argot_keyinput(pMsg.wparam);
				//APP_TRACE_BUFF_LOG(&pMsg.wparam, 1,"KEY INPUT:");
				//Sys_tms_AppBusy(1);
				//if (pMsg.wparam == GUI_KEY_OK || pMsg.wparam == GUI_KEY_QUIT)
				if (pMsg.wparam == GUI_KEY_QUIT)
				{
					gui_main_menu_show(MAIN_MENU_PAGE , 0);	
					gui_post_message(GUI_GUIPAINT, 0 , 0);
				}
			
				//Sys_tms_AppBusy(0);
			}
            //else if(pMsg.message_id == XM_FILE_DOWNLOAD_PAGE)
            //{
                //close_com();
                //argot_action("#5555#");                         
            //}
#ifdef BLUETOOTH_MODE_SET
			else if (pMsg.message_id == XM_BT_LINK_PAGE)
			{
				bt_link_page();
				gui_post_message(GUI_GUIPAINT, 0 , 0);
			}
#endif
			else
			{
				gui_proc_default_msg(&pMsg);
			} 
		}
		_mpos_proc();

		get_hhmmss_str(time_cur);
		if ( strcmp(time_last,time_cur) != 0 )
		{
			if(showTime > 0)
				showTime--;
			strcpy(time_last, time_cur );
			gui_post_message(GUI_GUIPAINT, 0 , 0);
		}
	}
}



