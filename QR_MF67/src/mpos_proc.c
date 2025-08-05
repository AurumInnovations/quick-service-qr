#include "driver\uart.h"
//#include "ucosii\ucos_ii.h"
//#include "pub\osl\inc\osl_time.h"
//#include "pub\common\misc\inc\mfmalloc.h"
#include "app_def.h"
#include "settings/admin_set_func.h"

#define MAX_UART_BUF		512
#define RECV_BUFF_SIZE		512

char *usb_buff = 0;
char usb_buff1[16] = {0};


static unsigned char UART_GetChar(int nSert)
{
	char cRet;

	UartRecv( nSert, &cRet,1 , 0);

	return cRet;
}

void init_com()
{
	int uartHand = 0;
    int usbMode = 0;

	usbMode = get_setting_int(SERIAL_MODE);
#ifdef DEV_MF67_A10
    //osl_log_pause();//close log out
	if (usbMode == 1) //CDC
	{
		uartHand = UartOpen(APP_COM, 115200, UART_DATABIT8, UART_STOPBIT1, UART_NOPARITY);
	}
	else // HID
		uartHand = UartOpen(HID_COM, 115200, UART_DATABIT8, UART_STOPBIT1, UART_NOPARITY);
    APP_TRACE("#########uart_open:%d,%d#########",MPOS_PORT_COM,uartHand);
    usb_buff = (unsigned char *)Util_Malloc(MAX_UART_BUF);
    mf_appport_setfifo(usb_buff, MAX_UART_BUF);
#else
    osl_log_pause();//close log out
	uartHand = UartOpen(MPOS_PORT_COM, 115200, UART_DATABIT8, UART_STOPBIT1, UART_NOPARITY);
	APP_TRACE("#########uart_open:%d,%d#########",MPOS_PORT_COM,uartHand);
	usb_buff = (unsigned char *)Util_Malloc(MAX_UART_BUF);
	mf_appport_setfifo(usb_buff, MAX_UART_BUF);
#endif        
}

void close_com()
{
    int usbMode = 0;

	usbMode = get_setting_int(SERIAL_MODE);
	if(usb_buff != 0)
    {	
#ifdef DEV_MF67_A10
        if (usbMode == 1)
		{
			UartClose(APP_COM);
		}
		else
			UartClose(HID_COM);
#else
		UartClose(MPOS_PORT_COM);
#endif
        free(usb_buff);
#ifdef DEV_MF67_A10        
		if (usbMode == 1)
		{
			mf_appport_setfifo(usb_buff1, 8);
		}
		else
        	mf_usb_hid_setfifo(usb_buff1, 8);        
#else
        mf_appport_setfifo(usb_buff1, 8);
#endif        
        osl_log_resume();
    }
}

int comm_uart_get_mode()
{
#ifdef WIN32
	return 1;
#endif


	if(bluetooth_is_connect()){
		return 0;
	}
	
	return 1;
}
int comm_uart_count()
{
	int ret;
	if(comm_uart_get_mode() == 0)
		ret = bluetooth_recv_count();
	else
		ret = UartGetRXBufCount(MPOS_PORT_COM);
	return ret;
}

int comm_uart_recv(char * buff, int size)
{
	int ret;
	if(comm_uart_get_mode() == 0){
		ret = bluetooth_recv(buff, size);
	}
	else{
		ret = UartRecv(MPOS_PORT_COM, buff, size, 0);
	}
	return ret;
}

void _mpos_proc()
{
	int i;
	int recv_count; 
	int data_len = 0;
	int index = 0;
	unsigned char check_sum = 0;
    unsigned char m_recv_buff[RECV_BUFF_SIZE+1];	
    unsigned char recv_tmp_buff[RECV_BUFF_SIZE+1];
	
	memset(m_recv_buff, 0x00, sizeof(m_recv_buff));
	while(1)
	{
		//Sys_Sleep(10);
		bluetooth_run_proc();
		if(comm_uart_count()>0)
		{
			recv_count = 0;
			check_sum = 0;
			memset(recv_tmp_buff, 0x00, sizeof(recv_tmp_buff));
			recv_count = comm_uart_recv(recv_tmp_buff, MAX_UART_BUF);
			memcpy(m_recv_buff + index,recv_tmp_buff,recv_count);
			APP_TRACE_BUFF_TIP(m_recv_buff, recv_count + index, "recv from host");
			if(m_recv_buff[0] == STX_CODE_1 && m_recv_buff[1]==STX_CODE_2)	//mf protocol
			{
				if (m_recv_buff[2] == 0x30)
					data_len = mpos_pub_get_len(&m_recv_buff[2]) + 6;
				else
					data_len = mpos_pub_get_ll_len(&m_recv_buff[2]) + 6;
				if (recv_count + index < data_len)
				{
					index += recv_count;
					APP_TRACE("continue recving");
					Sys_Delay(100);
					continue;
				}
				mpos_pub_check_sum_update(&check_sum , m_recv_buff + 2 , data_len - 3);
				if(m_recv_buff[data_len - 2] == ETX_CODE && m_recv_buff[data_len - 1] == check_sum)	// mpos tlv protocol
				{
					APP_TRACE("MPOS protocol");
					data_len = mpos_pub_get_ll_len(&m_recv_buff[2]) + 2;
					if(data_len > RECV_BUFF_SIZE-5)
						break;
					mpos_func_proc(m_recv_buff + 4, data_len - 6,PROC_MPOS_FORMAT);		
				}
				else  //json protocol
				{
					APP_TRACE("JSON protocol");
					data_len = mpos_pub_get_len(&m_recv_buff[2]);
					if(data_len > RECV_BUFF_SIZE-5)
						break;
					mpos_func_proc(m_recv_buff + 6, data_len,PROC_JSON_FORMAT);
				}
			}			
		}
		break;
	}
	
	return ;
}

static void _Serial_Mode_Paint(int uMode)
{
	gui_begin_batch_paint();

	gui_clear_dc();

	gui_text_out(0, GUI_LINE_TOP(0), uMode == 1 ? "SerialMode:CDC" : "SerialMode:HID");

	gui_page_op_paint( "Cancel" , "OK");

	gui_end_batch_paint();
}

static int handle_key_press(int pressKey, int* pUsbMode) {
	switch (pressKey) {
		case GUI_KEY_UP: case GUI_KEY_DOWN: case GUI_KEY_LEFT:
		case GUI_KEY_RIGHT: case GUI_KEY_XING: case GUI_KEY_JING:
			*pUsbMode = !*pUsbMode;
			_Serial_Mode_Paint(*pUsbMode);
			return 0;

		case GUI_KEY_OK:
			set_setting_int(SERIAL_MODE, *pUsbMode);
		case GUI_KEY_QUIT:
			return 1;
	}
}

void SerialModeSwitch()
{
	int presskey;
	st_gui_message pMsg;
	int usbMode = get_setting_int(SERIAL_MODE);
	_Serial_Mode_Paint(usbMode);

	while(1)
	{
		if (gui_get_message(&pMsg, 100) != 0) 
			continue;
		if (pMsg.message_id != GUI_KEYPRESS) 
			continue;
		if(handle_key_press(pMsg.wparam, &usbMode))
			return;
	} 
}


