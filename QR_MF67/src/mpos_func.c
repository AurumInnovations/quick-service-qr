#include "driver\uart.h"
#include "driver\mf_rtc.h"
#include "app_def.h"
#include "pub\osl\inc\osl_time.h"
#include "driver\mf_simple_rfid.h"
#include "pub\common\misc\inc\mfmalloc.h"
#include "scan_page.h"
#include <string.h>

static st_qr_data mpos_qr_data = {0};

extern int g_playtag;
int g_reset_flag = 0;
int g_show_qr = 0;
int g_ntag_init = 0;

st_qr_data  * mpos_func_get_qr_data()
{
	return &mpos_qr_data;
}

st_qr_data * mpos_func_clear_qr_data()
{
	if (mpos_qr_data.bmp!= NULL)
	{
		FREE(mpos_qr_data.bmp);
		mpos_qr_data.bmp = NULL;
	}
	memset(mpos_qr_data.qrdata, 0, sizeof(mpos_qr_data.qrdata));
}
int comm_pub_atoi(char *pbuff , int len)
{
	char tmp[12];
	int data;
	if(len > 10) return 0;

	memcpy(tmp , pbuff ,len);
	tmp[len] = 0;
	
	data = atoi(tmp);
	return data;
}
void comm_pub_set_ll_len(unsigned char *pbuff , int len)
{
	pbuff[0] = (len / 1000) * 16 + (len / 100 % 10);
	pbuff[1] = (len / 10 % 10) * 16 + (len % 10);
}

int comm_pub_get_ll_len(unsigned char *pbuff)
{
	int len = 0;
	len = (pbuff[0] & 0xf0) / 16 * 1000 + (pbuff[0] & 0x0f) * 100;
	len +=	(pbuff[1] & 0xf0) / 16 * 10 + (pbuff[1] & 0x0f);
	return len;
}
void dev_reset(st_pkt_info *pkt_info)
{
	unsigned char state[1];
	int i = 0,j;

	for(i = 0 ; i < 3; i ++){
		for(j = 0; j < 3; j ++){
			gui_post_message(GUI_KEYPRESS, GUI_KEY_BACKSPACE, 0);
		}
		Sys_Sleep(100);
	}
	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));

	state[0] = 0x00;
	mpos_pub_send_pkt_json(pkt_info);
}

void m1_card_proc(st_pkt_info *pkt_info)
{	
	unsigned char *pbuff = pkt_info->pbuff;
	int func = pbuff[0];
	unsigned char *data = Util_Malloc(512);
	int dlen = 0;
	int ret;
	int cmd ;
	int block;
	int operand;
	int rlen;
	int slen;

	APP_TRACE("m1_card_proc0:%d\r\n", func);

	pbuff ++;


	memset(data, 0, 512);
	if(func == 1){
		ret = Icc_RfMfclOpen();
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 2){
		ret = Icc_RfMfclClose();
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 3){
		ret = Icc_Rf_uid(data + 2);
		APP_TRACE("getuid:%d\r\n", ret);
		dlen = ret < 0 ? 0 : ret;
		comm_pub_set_ll_len(data, dlen);
		dlen += 2;
	}
	else if(func == 4){
		ret = Icc_RfMfclAtqa();
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 5){
		ret = Icc_RfMfclSetKey(pbuff);
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 6){
		memcpy(&cmd, pbuff, 4);
		memcpy(&block, pbuff + 4, 4);	
		APP_TRACE("cmd:%x\r\n", cmd);
		APP_TRACE("block:%x\r\n", block);
		ret = Icc_RfMfclAuth(cmd, block);
		APP_TRACE("Icc_RfMfclAuth:%d\r\n", ret);
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 7){
		memcpy(&block, pbuff, 4);	
		ret = Icc_RfMfclRead(block, data + 2, &rlen);
		dlen = (ret == 0) ? rlen : 0;
		comm_pub_set_ll_len(data, dlen);
		dlen += 2;
	}
	else if(func == 8){
		memcpy(&block, pbuff, 4);	
		slen = comm_pub_get_ll_len(pbuff + 4);
		ret = Icc_RfMfclWrite(block, pbuff + 6, slen);
		APP_TRACE("Icc_RfMfclWrite:%d,%d,%d\r\n", block, slen, ret);
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 9){
		memcpy(&block, pbuff, 4);	
		memcpy(&operand, pbuff+ 4, 4);	
		ret = Icc_RfMfclIncrement(block, operand);
		APP_TRACE("Icc_RfMfclIncrement:%d,%d,%d\r\n", block, operand, ret);
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 10){
		memcpy(&block, pbuff, 4);	
		memcpy(&operand, pbuff+ 4, 4);	
		ret = Icc_RfMfclDecrement(block, operand);
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 11){
		memcpy(&block, pbuff, 4);	
		ret = Icc_RfMfclTransfer(block);
		memcpy(data, &ret , 4);
		dlen = 4;
	}
	else if(func == 12){
		memcpy(&block, pbuff, 4);	
		ret = Icc_RfMfclRestore(block);
		memcpy(data, &ret , 4);
		dlen = 4;
	}

	APP_TRACE("m1_card_proc2:%d,%02X %02X %02X %02X\r\n", dlen, data[0],data[1],data[2],data[3]);
	mpos_pub_send_pkt(data, dlen , pkt_info);
        Util_Free(data);
}

void rf_card_power(st_pkt_info *pkt_info)
{	
	unsigned char *pbuff = pkt_info->pbuff;
	int card_type = pbuff[0];
	int cmd = pbuff[1];
	char data[16]={0};
	int dlen = 0;
	int ret;

	APP_TRACE("[%d]rf_card_power card_type:%d, cmd:%d\r\n", Sys_GetTick(), card_type, cmd);

	if(card_type == 2)
	{
		if (cmd == 1)
		{
			ret = Icc_rfid_tcl_open();
			if (ret == 0)
			{
				data[0] = 0;
				dlen = 1;
			} 
			else
			{
				data[0] = 1;
				dlen = 1;
			}
		}
		else if (cmd == 2)
		{
			ret = Icc_rfid_tcl_close();
			if (ret == 0)
			{
				data[0] = 0;
				dlen = 1;
			} 
			else
			{
				data[0] = 1;
				dlen = 1;
			}
		}
		else
		{
			data[0] = 1;
			dlen = 1;
		}
	}
	else if(card_type == 3)
	{
		if (cmd == 1)
		{
			ret = Icc_rfid_felica_open();
			if (ret == 0)
			{
				data[0] = 0;
				dlen = 1;
			} 
			else
			{
				data[0] = 1;
				dlen = 1;
			}
		}
		else if (cmd == 2)
		{
			ret = Icc_rfid_felica_close();
			if (ret == 0)
			{
				data[0] = 0;
				dlen = 1;
			} 
			else
			{
				data[0] = 1;
				dlen = 1;
			}
		}
		else
		{
			data[0] = 1;
			dlen = 1;
		}
	}
	else 
	{
		data[0] = 1;
		dlen = 1;
	}

	APP_TRACE("[%d]rf_card_power:%d,%02X\r\n", Sys_GetTick(), dlen, data[0]);
	mpos_pub_send_pkt(data, dlen , pkt_info);
}

void rf_card_proc(st_pkt_info *pkt_info)
{	
	unsigned char *pbuff = pkt_info->pbuff;
	int card_type = pbuff[0];
	unsigned char *data = Util_Malloc(512);
	int dlen = 0;
	int ret = 0;
	int rlen = 0;
	int slen = 0;
	unsigned char *rxbuf;

	memset(data, 0, 512);
	APP_TRACE("[%d]rf_card_proc: cardtype:%d\r\n", Sys_GetTick(), card_type);

	pbuff ++;

	slen = comm_pub_get_ll_len(pbuff);
	if(card_type == 2)
	{		
		if (slen > 0)
		{
			ret = Icc_Rfid_tcl_exchange(pbuff+2, (unsigned short)slen, &rxbuf, (unsigned short *)&rlen);
		}
		comm_pub_set_ll_len(data, rlen);
		memcpy(data+2, rxbuf , rlen);
		dlen = rlen+2;
	}
	else if(card_type == 3)
	{
		if (slen > 0)
		{
			ret = Icc_rfid_felica_exchange(pbuff+2, slen, rxbuf, (unsigned short *)&rlen, 100);
		}
		comm_pub_set_ll_len(data, rlen);
		memcpy(data+2, rxbuf , rlen);
		dlen = rlen+2;
	}
	else 
	{
		comm_pub_set_ll_len(data, 0);
		dlen = 2;
	}

	APP_TRACE("[%d]rf_card_proc:dlen=%d", Sys_GetTick(), dlen);
	APP_TRACE_BUFF(data, dlen);
	mpos_pub_send_pkt(data, dlen , pkt_info);
	Util_Free(data);
}

void dev_reset_ex(st_pkt_info *pkt_info)
{
	unsigned char state[1];
	int i = 0;
	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
	state[0] = 0x00;
	
	mpos_pub_send_pkt(state , sizeof(state) , pkt_info);
}

void set_date_time(st_pkt_info *pkt_info)
{
	unsigned char state[1];
	char dttmp[30]={0};
	DATE_TIME_T	datetime;
	unsigned char *pbuff = pkt_info->pbuff;
	char ret[1] = {0};
	char szProductID[64] = {0};
	datetime.nYear = comm_pub_atoi(pbuff , 4) ;
	datetime.nMonth = comm_pub_atoi(pbuff + 4 , 2) ;
	datetime.nDay = comm_pub_atoi(pbuff + 6 , 2) ;
	datetime.nHour = comm_pub_atoi(pbuff + 8, 2) ;
	datetime.nMinute = comm_pub_atoi(pbuff + 10, 2) ;
	datetime.nSecond = comm_pub_atoi(pbuff + 12, 2) ;
	sprintf((char *)dttmp , "%4d%02d%02d%02d%02d%02d" , 
		datetime.nYear , datetime.nMonth , datetime.nDay,
		datetime.nHour , datetime.nMinute , datetime.nSecond);
	Sys_SetDateTime(dttmp);

	state[0] = 0x00;
	if(pkt_info->format == PROC_MPOS_FORMAT)
		mpos_pub_send_pkt(state , 1 , pkt_info);
	else
		mpos_pub_send_pkt_json(pkt_info);
}
void custom_show_text_proc(st_pkt_info *pkt_info)
{
	int data_len = 0;
	unsigned char state[1];
	unsigned char *pbuff = pkt_info->pbuff;
	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
	g_reset_flag = 1;
	mpos_qr_data.showTime= pbuff[0];
	data_len = comm_pub_get_ll_len(pbuff+1);
	data_len = (data_len >= 64) ? 63 : data_len;
	memcpy(mpos_qr_data.text, pbuff + 3, data_len);
	APP_TRACE("qr text[%d]:%s", data_len, mpos_qr_data.text);
	state[0] = 0x00;
	mpos_pub_send_pkt(state , 1 , pkt_info);
    g_playtag = 1;	
}
//"URL|text"
void custom_show_QR_proc(st_pkt_info *pkt_info)
{
	int data_len = 0;
	unsigned char state[1];
	unsigned char *pbuff = pkt_info->pbuff;
	int index = 0;
	int index2 = 0;
	int fcount = 0; //count of "|"

	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
	mpos_qr_data.showTime= pbuff[0];
	data_len = comm_pub_get_ll_len(pbuff + 1);
	while (index < data_len)
	{
		if (memcmp(pbuff + 3 + index, "|", 1) != 0)
		{
			switch(fcount)
			{
			case 0: //URL
				memcpy(mpos_qr_data.qrdata + index2, pbuff + 3 + index, 1);
				break;
// 			case 1: //amount
// 				memcpy(mpos_qr_data.amt + index2, pbuff + 3 + index, 1);
// 				break;
			case 1: //text
				memcpy(mpos_qr_data.prompt + index2, pbuff + 3 + index, 1);
				break;
// 			case 3: //flag
// 				memcpy(mpos_qr_data.position, pbuff + 3 + index, 1);
// 				break;
// 			case 4: //font size
// 				memcpy(mpos_qr_data.fontsize, pbuff + 3 + index, 1);
// 				break;
			default:
				break;
			}
			index2++;
		}	
		else
		{
			fcount++;
			index2 = 0;
		}
		index++;
	}
	//memcpy(mpos_qr_data.qrdata, pbuff + 3, data_len);
	APP_TRACE("qr data[%d]:%s", data_len, mpos_qr_data.qrdata);
	if (strlen(mpos_qr_data.prompt) > 0)
		APP_TRACE("qr prompt:%s", mpos_qr_data.prompt);
	state[0] = 0x00;
	mpos_pub_send_pkt(state , 1 , pkt_info);
	g_show_qr = 1;
}

void scan_qr_proc(st_pkt_info *pkt_info)
{
	char data[256+1] = {0};
	
	int ret = scan_page_proc("Scan", data, sizeof(data), 120*1000);
	
	mpos_pub_send_pkt(data , strlen(data) , pkt_info);
}

void custom_show_bmp_proc(st_pkt_info *pkt_info)
{
	int data_len = 0;
	unsigned char state[1];
	unsigned char *pbuff = pkt_info->pbuff;
	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
	mpos_qr_data.showTime = pbuff[0];
	mpos_qr_data.bmpWidth = pbuff[1];
	if (mpos_qr_data.bmpWidth > 120)
	{
		state[0] = 0x02;
		mpos_pub_send_pkt(state , 1 , pkt_info);
		return;
	}
	data_len = comm_pub_get_ll_len(pbuff + 2);
	if (mpos_qr_data.bmp != NULL) {
		FREE(mpos_qr_data.bmp);
		mpos_qr_data.bmp = NULL;
	}
	mpos_qr_data.bmp = (char*)MALLOC(data_len + 1);
	memcpy(mpos_qr_data.bmp, pbuff + 4, data_len);
	APP_TRACE("bmp width[%d]", mpos_qr_data.bmpWidth);
	APP_TRACE("bmp data[%d]", data_len);
	state[0] = 0x00;
	mpos_pub_send_pkt(state , 1 , pkt_info);
}

void custom_set_ntag_proc(st_pkt_info *pkt_info)
{
	int data_len = 0;
	unsigned char state[1];
	unsigned char data[256] = {0};
	unsigned char *pbuff = pkt_info->pbuff;
	data_len = comm_pub_get_ll_len(pbuff + 1);
	memcpy(data, pbuff + 3, data_len);
	APP_TRACE("Ndef message[%d]:%s", data_len, data);
	set_ntag_block(data, data_len);
	g_ntag_init = 0;
	state[0] = 0x00;
	mpos_pub_send_pkt(state , 1 , pkt_info);
}

void mpos_show_QR_proc(st_pkt_info *pkt_info)
{
	int data_len = 0;
	unsigned char state[1];
	unsigned char *pbuff = pkt_info->pbuff;
	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
	data_len = comm_pub_get_ll_len(pbuff);
	memcpy(mpos_qr_data.qrdata, pbuff + 2, data_len);
	APP_TRACE("qr data:%s",mpos_qr_data.qrdata);
	state[0] = 0x00;
	mpos_pub_send_pkt(state , 1 , pkt_info);
}

void set_qrdata(st_pkt_info *pkt_info)
{
	unsigned char state[1];
	int n = 0;
	struct json_object *rootobj=0;
        char *temp = (char *)malloc(1024);	
	char szSha1Data[21] = {0};
	char szDate[20] = {0};
	char tmp[30]={0};
	struct rtc_time dt;
	
	Sys_GetDateTime(tmp);
	sscanf(tmp,"%04d%02d%02d%02d%02d%02d",&dt.tm_year, &dt.tm_mon, &dt.tm_mday,&dt.tm_hour, &dt.tm_min, &dt.tm_sec);


	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
        sprintf(szDate,"%02d%02d%02d",dt.tm_year-2000, dt.tm_mon, dt.tm_mday);
        rootobj = (struct json_object *)json_tokener_parse((char *)pkt_info->pbuff);
        if ((int)rootobj > 0)
        {
            memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
            mpay_pub_get_json(rootobj, "A", temp, 16);
            strcpy(mpos_qr_data.amt, temp);

            memset(temp, 0, sizeof(temp));
            mpay_pub_get_json(rootobj, "D", temp, 1024);
            //strcat(temp,szDate);
            strcpy(mpos_qr_data.qrdata, temp);
            //mf_sha1(temp, strlen(temp), mpos_qr_data.qrdata);
   
            json_object_put(rootobj);
        }
        free(temp);
	state[0] = 0x00;
        mpos_pub_send_pkt_json(pkt_info);
}

void set_text(st_pkt_info *pkt_info)
{
	unsigned char state[1];
	int n = 0;
	struct json_object *rootobj=0;
	char temp[64] = {0};
        
	memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
        rootobj = (struct json_object *)json_tokener_parse((char *)pkt_info->pbuff);
        if ((int)rootobj > 0)
        {
            memset(&mpos_qr_data, 0, sizeof(mpos_qr_data));
            mpay_pub_get_json(rootobj, "T", temp, 64);
            strcpy(mpos_qr_data.text, temp);
            g_playtag = 1;   
            mpos_qr_data.showTime = 5;
            g_reset_flag = 1;
            json_object_put(rootobj);
        }

	state[0] = 0x00;
        mpos_pub_send_pkt_json(pkt_info);
}

void set_voice(st_pkt_info *pkt_info)
{

}
void set_systime(st_pkt_info *pkt_info)
{
	unsigned char state[1];
	char temp[32] = {0};
	char tmp[32]={0};
	struct json_object *rootobj=0;
	struct rtc_time date_time;	

        rootobj = (struct json_object *)json_tokener_parse((char *)pkt_info->pbuff);
        if ((int)rootobj > 0)
        {
                mpay_pub_get_json(rootobj, "DT", temp, 20);
                json_object_put(rootobj);
        }

	Sys_GetDateTime(tmp);
	sscanf(tmp,"%04d%02d%02d%02d%02d%02d",
		&date_time.tm_year, &date_time.tm_mon, &date_time.tm_mday,
		&date_time.tm_hour, &date_time.tm_min, &date_time.tm_sec);

	date_time.tm_year = mpos_pub_atoi(temp, 4);
	date_time.tm_mon = mpos_pub_atoi(temp+4, 2);
	date_time.tm_mday = mpos_pub_atoi(temp+6, 2);
	date_time.tm_hour = mpos_pub_atoi(temp+8, 2);
	date_time.tm_min = mpos_pub_atoi(temp+10, 2);
	date_time.tm_sec = mpos_pub_atoi(temp+12, 2);
	sprintf(tmp , "%4d%02d%02d%02d%02d%02d" , 
		date_time.tm_year , date_time.tm_mon , date_time.tm_mday,
		date_time.tm_hour , date_time.tm_min , date_time.tm_sec);
	Sys_SetDateTime(tmp);

	state[0] = 0x00;
        mpos_pub_send_pkt_json(pkt_info);
}


//STX(0x4D46)  Data length	 Instruction number  Command number Serial number Response code Variable data	ETX(0x02)	LRC	
// 2bytes		2 bytes			 1 byte				2 bytes			1 byte		 2 bytes	   variable		 1 byte	    1byte
int mpos_func_proc(unsigned char * data, int len,int flag)
{
    int ret ;
    st_pkt_info pkt_info;
    extern st_qr_data g_qr_data;
    Sys_power_key_set_light();
    if(flag == PROC_JSON_FORMAT)
        ret = mpos_pub_pkt_check_json(data , len , &pkt_info);
    else if(flag == PROC_MPOS_FORMAT)
        ret = mpos_pub_pkt_check(data , len , &pkt_info);
    APP_TRACE("ret:%d----func:%04x",ret,pkt_info.func);

    if(ret != 0) 
        return ret;
    if(pkt_info.func == SET_DATE_COMM){		// set datetime	
        set_date_time(&pkt_info);
    }
    else if(pkt_info.func == CUSTOM_SHOW_TEXT_COMM){		// show text
        custom_show_text_proc(&pkt_info);
    }
	else if (pkt_info.func == CUSTOM_SHOW_QR)      //show QR code
	{
		custom_show_QR_proc(&pkt_info);
	}
	else if (pkt_info.func == CUSTOM_SCAN_QR)		// scan QR code
	{
		scan_qr_proc(&pkt_info);
	}
	else if (pkt_info.func == CUSTOM_SHOW_BMP)      //show bmp
	{
		custom_show_bmp_proc(&pkt_info);
	}
	else if(pkt_info.func == CUSTOM_SET_NTAG){		// set NTAG value	
		custom_set_ntag_proc(&pkt_info);
	}
    else if(pkt_info.func == RESET_COMM)
    {
        dev_reset_ex(&pkt_info);
    }
    else if(pkt_info.func == M1_CARD_PROC_COMM)
    {
        m1_card_proc(&pkt_info);
    }
	else if(pkt_info.func == RF_CARD_POWER)
	{
		rf_card_power(&pkt_info);
	}
	else if(pkt_info.func == RF_CARD_PROC_COMM)
	{
		rf_card_proc(&pkt_info);
	}
    else if (MPOS_FUNC_RESET == pkt_info.func)
    {
        dev_reset(&pkt_info);
    }
    else if (MPOS_FUNC_SET_TIME == pkt_info.func)
    {	
        set_systime(&pkt_info);
    }
    else if (MPOS_FUNC_SET_QR == pkt_info.func)
    {
        set_qrdata(&pkt_info);
    }
    else if (MPOS_FUNC_SET_TEXT == pkt_info.func)
    {
        set_text(&pkt_info);
    }
    else if (MPOS_FUNC_SET_VOICE == pkt_info.func)
    {
        set_voice(&pkt_info);
    }

    gui_post_message(GUI_GUIPAINT, 0, 0);	

    return 0;
}
