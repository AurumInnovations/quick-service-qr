
#include "driver/mf_driverlib.h"
#include "driver/mf_simple_rfid.h"
#include "app_def.h"
#include "driver/uart.h"
#include "driver/mf_serial.h"
#include "bluetooth/inc/bluetooth.h"


#define LOGOIMG "data\\logo2.bmp"

int g_playtag = 0;

static void sdk_show_msg(char *title, char *msg)
{
	gui_begin_batch_paint();				
	gui_clear_dc();			//  Clear the entire screen
	gui_textout_line_center(title , GUI_LINE_TOP(0));		// Display the content of title at line 0
	gui_text_out(0, GUI_LINE_TOP(2), msg);   // Display msg content on line 2
	gui_end_batch_paint();  
}

void sdk_driver_led()
{
	int i;
	char msg[32];

	for(i =0 ; i < 4 ; i ++){
		sprintf(msg, "test led:%d", i);
		sdk_show_msg("led", msg);
		Util_Led(i, 1);		// open
		Sys_Sleep(500);
		Util_Led(i, 0);		// close
		Sys_Sleep(500);
	}
	gui_post_message(GUI_GUIPAINT, 0 , 0);
	
}


#ifdef DEV_MF67_A10
void sdk_driver_barcode()
{
	char msg[128];
	int ret;
	int com = MF_UART_COM22;
	unsigned char rbuf[256]={0};
	int count = 0;
	int flag = 0;
	int tick1 = Sys_GetTick();

	sdk_show_msg("barcode", "scan...");

	comm_UartOpen(com, 115200, MF_UART_WordLength_8b, MF_UART_StopBits_1, MF_UART_Parity_No);
	comm_UartClear(com);

	while(Sys_CheckTimeover(tick1 , 5000) == 0){
		ret = UartGetRXBufCount(com);
		if(ret > 0){
			ret = UartRecv(com, rbuf + count, sizeof(rbuf) - count, 10);
			count += ret;
			flag = 1;
		}
		else if(flag == 1){
			break;
		}
		Sys_Sleep(50);
	}
	
	comm_UartClose(com);

	if(count>0) {
		gui_messagebox_show("barcode" , rbuf, "" , "confirm" , 30000);
	}
	
}
#endif
void VoiceTest()
{
	Play_Voice("payok");//payment suceesfull
	Sys_Sleep(1000);
	Play_Voice("payerr");//payment failure
	gui_messagebox_show("Voice" , "Play Ok or not", "" , "confirm" , 30000);
	//Play_Voice("NoSpike");
}

void pagepaint(char *title, char *msg, int logo)
{
	int len = 0;
	char temp[20] = {0};
	int logowidth;
	int logoheight;
	int logoleft;
	int logotop;
	int logocolor;

	char * pbmp;

	len = strlen(msg);

	gui_begin_batch_paint();
	gui_set_win_rc();
	gui_clear_dc();


	if (logo == 1)
	{
		pbmp = (char *)gui_load_bmp_ex(LOGOIMG , &logowidth , &logoheight, &logocolor);

		logoleft = (gui_get_width()-logowidth)/2;
		logotop = GUI_LINE_TOP(1)+5;

		if (pbmp != 0){
			gui_out_bits_ex(logoleft, logotop, pbmp , logowidth , logoheight , 1 , logocolor);
			Util_Free(pbmp);
		}	
	} 
	else
	{
		gui_set_title(title);

		if (len > 0 && len <= 15)
		{
			gui_textout_line_center(msg, GUI_LINE_TOP(3));
		}
		else if (len>15 && len<=30)
		{
			memcpy(temp, msg, 15);
			gui_textout_line_center(temp, GUI_LINE_TOP(3));
			gui_textout_line_center(msg+15, GUI_LINE_TOP(4));
		}
		else if (len>30 && len<=45)
		{
			memcpy(temp, msg, 15);
			gui_textout_line_center(temp, GUI_LINE_TOP(3));
			memcpy(temp, msg+15, 15);
			gui_textout_line_center(temp, GUI_LINE_TOP(4));
			gui_textout_line_center(msg+30, GUI_LINE_TOP(5));
		}
		else if (len > 45)
		{
			memcpy(temp, msg, 15);
			gui_textout_line_center(temp, GUI_LINE_TOP(3));
			memcpy(temp, msg+15, 15);
			gui_textout_line_center(temp, GUI_LINE_TOP(4));
			memcpy(temp, msg+30, 15);
			gui_textout_line_center(temp, GUI_LINE_TOP(5));
			gui_textout_line_center(msg+45, GUI_LINE_TOP(6));
		}	
	}

	gui_end_batch_paint();
}

void app_set_bt_name()
{
	char btname[32]={0};
	char tid[33] = {0};

	Sys_GetTermSn(tid);
	if (strlen(tid) < 10)
		sprintf(tid, "8173004834");
	comm_get_bt_name(btname);
	APP_TRACE("get btname : %s", btname);
	if(strlen(btname) > 10 || memcmp(btname, tid, 10) != 0){
		memset(btname, 0, sizeof(btname));
		snprintf(btname, 11, "%s", tid + strlen(tid) - 10);
		APP_TRACE("set btname : %s", btname);
		comm_set_bt_name(btname);
	}
}

void BluetoothTest()
{
#define RECV_BUFF_SIZE		1280
#define RECV_TMP_SIZE		128

	static unsigned char * m_recv_buff;
	static int m_recv_index = 0;
	static unsigned int m_last_recv_tick = 0;
	char buffer[256] = {0};
	st_gui_message pMsg;

	m_recv_buff = (unsigned char *)Util_Malloc(RECV_BUFF_SIZE+1);
	Sys_Sleep(100);
	
	strcpy(buffer, "Waiting to Recv");
	gui_post_message(GUI_GUIPAINT, 0 , 0);

	while(1)
	{
		if (gui_get_message(&pMsg, 500) == 0) {

			if (pMsg.message_id == GUI_GUIPAINT) {
				pagepaint("BluetoothTest", buffer, 0);
			}
			else if (pMsg.message_id == GUI_KEYPRESS){
				if (pMsg.wparam == GUI_KEY_QUIT)
				{
					break;
				}
			}
			else{
				gui_proc_default_msg(&pMsg);
			} 
		}

		bluetooth_run_proc();

		if(bluetooth_recv_count() <=0 ){
			if(Sys_CheckTimeover(m_last_recv_tick , 5000) != 0){
				m_recv_index = 0;
				m_last_recv_tick = Sys_GetTick();	
			}
			continue;
		}

		m_last_recv_tick = Sys_GetTick();

		memset(m_recv_buff, 0x00, RECV_BUFF_SIZE);
		m_recv_index = bluetooth_recv(m_recv_buff, RECV_TMP_SIZE);
		if (m_recv_index > 0)
		{
			sprintf(buffer, "Recv:%s", m_recv_buff);
			gui_post_message(GUI_GUIPAINT, 0 , 0);
            bluetooth_send(m_recv_buff, m_recv_index);
		}			
	}

	Util_Free(m_recv_buff);
}

void SerialPortTest()
{
	st_gui_message pMsg;
	st_qr_data  * qr_data = mpos_func_get_qr_data();
	char buffer[20] = {0};
	char time_cur[20];
	char time_last[20];
	char msg[128] = {0};
	int logo = 1;

	get_hhmmss_str(time_last);

	init_com();
	strcpy(buffer, "Waiting to Recv");
	//strcpy(qr_data->amt, "1.00");
	//strcpy(qr_data->qrdata, "http://en.morefun-et.com");
	//strcpy(mpos_qr_data->text, "payment successful!");
	gui_post_message(GUI_GUIPAINT, 0 , 0);

	while(1)
	{
		if (gui_get_message(&pMsg, 500) == 0) {

			if (pMsg.message_id == GUI_GUIPAINT)
			{
				if (strlen(qr_data->qrdata) > 0)	
				{
					Util_led_digit_show(qr_data->amt);	
					showQr(qr_data->amt, qr_data->qrdata);
				} 
				else if (strlen(qr_data->text) > 0)
				{
					if (strstr(qr_data->text, "success") != 0 && g_playtag == 1)
					{
						Play_Voice("payok");
						Util_led_digit_show("1");	
					} 
					else if (strstr(qr_data->text, "fail") != 0 && g_playtag == 1)
					{
						Play_Voice("payerr");
						Util_led_digit_show("0");	
					} 
					g_playtag = 0;

					sprintf(msg, "\n\n%s", qr_data->text);

					gui_messagebox_show(" ",  msg, "" , "" ,  500);
				}
				else
				{
					Util_led_digit_show("");	
		
					pagepaint("SerialPortTest", buffer, 0);
					
				}
			}
			else if (pMsg.message_id == GUI_KEYPRESS){
				if (pMsg.wparam== GUI_KEY_QUIT)
				{
					break;
				}
			}
			else{
				gui_proc_default_msg(&pMsg);
			} 
		}

		_mpos_proc();

		get_hhmmss_str(time_cur);
		if ( strcmp(time_last,time_cur) != 0 ){
			strcpy(time_last, time_cur );
			gui_post_message(GUI_GUIPAINT, 0 , 0);
		}
	}

	close_com();
}
void sdk_Beep(int num)
{
	while( num > 0 )
	{
		Util_buzzer_control_ext(100);
		num--;
		if ( num > 0)
		{
			Sys_Sleep(300);
		}
	}
}

