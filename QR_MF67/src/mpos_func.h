#pragma once

#include "mpos_pub.h"

typedef struct __st_qr_data{
	st_pkt_info pkt_info;

	char amt[20];	  //The transaction amount
	char qrdata[512]; //qr data
	char prompt[16]; //msg prompt below qr
	char text[64];	//Payment success message
	char showTime;
	char* bmp;
	int bmpWidth;
	//int x;		//byte
	//int y;		//Row 1-8
	//int size;		//0(small),1(normal),2(big)
}st_qr_data;


#define	MPOS_RESULT_SUCC			"00"	//Successful processing
#define	MPOS_RESULT_NOT_SUPPORTED	"01"	//Instruction number is not supported
#define	MPOS_RESULT_PARAM_ERR		"02"	//Parameter error
#define MPOS_RESULT_LRC_ERR			"03"  	//LRC Verification failed
#define MPOS_RESULT_TIMEOUT			"04"	//Time out
#define MPOS_RESULT_OTHER_ERR		"05"	//Other errors


#define PROC_JSON_FORMAT			0x01
#define PROC_MPOS_FORMAT			0x02


#define	MPOS_FUNC_RESET			0x01
#define	MPOS_FUNC_SET_TIME		0x02
#define	MPOS_FUNC_SET_QR		0x03
#define	MPOS_FUNC_SET_TEXT		0x04
#define	MPOS_FUNC_SET_VOICE		0x05
#define	MPOS_FUNC_RF_POWER		0x06
#define	MPOS_FUNC_RF_EXCHANGE	0x07

#define	SET_DATE_COMM		        0x1D04  //set time
#define	CUSTOM_SHOW_TEXT_COMM	    0x800F	//set text
#define CUSTOM_SHOW_BMP				0x8010  //show bmp
#define	RESET_COMM					0x1D08  //reset
#define	M1_CARD_PROC_COMM	        0x2001  //M1 card
#define	RF_CARD_POWER				0x1C08  //RF power on/off
#define	RF_CARD_PROC_COMM			0x1C09  //RF command transmission
#define CUSTOM_SHOW_QR              0x800C  //show QR
#define	CUSTOM_SET_NTAG		        0x900E	//set NTAG value
#define	CUSTOM_SCAN_QR              0x810C	//scan QR


enum{
	MPOS_PAY_ITEM_TYPE = 0,				// Transaction type  01 Set QR code data 02 Set device date time 03 Payment success message    
	MPOS_PAY_ITEM_TIMEOVER,				// Overtime time
	MPOS_PAY_ITEM_AMT,				// The transaction amount
	MPOS_PAY_ITEM_QRDATA,				// QR data
	MPOS_PAY_ITEM_TEXT,				// Payment success message
	MPOS_PAY_ITEM_DATETIME,				// Device date time

	//	MPOS_PAY_ITEM_RESULE = 50,		// Transaction result
};

st_qr_data  * mpos_func_get_qr_data();
st_qr_data * mpos_func_clear_qr_data();
int mpos_func_proc(unsigned char * data , int len,int flag);