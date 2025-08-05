#pragma once

//#define USEHID 1

#ifdef WIN32
  #define MPOS_PORT_COM	COM11
#else
  // #ifdef DEV_MF67_A10
  //   #ifdef USEHID  
  //     #define MPOS_PORT_COM	HID_COM
  //   #else
  //     #define MPOS_PORT_COM	APP_COM
  //   #endif
  // #else
    #define MPOS_PORT_COM	APP_COM
  // #endif
#endif
typedef struct __st_pkt_info{
	int flag;
	int func;
	int seq;
	char * pbuff;
	int len;
	int ret;
	int format;
}st_pkt_info;



#define	STX_CODE_1 			0x4D
#define	STX_CODE_2 			0x46
#define	ETX_CODE			0x02
#define	ETX_CODE_json			"ED"


void mpos_pub_check_sum_update(unsigned char * check_value , unsigned char *buff , int length);
int mpos_pub_send_pkt(unsigned char * pdata  , int len , st_pkt_info  * pkt);
int mpos_pub_pkt_check(unsigned char * pdata , int len , st_pkt_info  * pkt);
int mpos_pub_pkt_check_json(unsigned char * pdata , int len , st_pkt_info  *pkt);
void mpos_pub_pack_str(unsigned char * pbuff , unsigned char * str ,int size);

int mpos_pub_get_ll_len(unsigned char *pbuff);
void mpos_pub_set_ll_len(unsigned char *pbuff , int len);
int mpos_pub_get_len(unsigned char *pbuff);
void mpos_pub_set_len(unsigned char *pbuff , int len);
int mpos_pub_unpack_ll_data(unsigned char *pbuff , unsigned char * pdata , int size);

unsigned int mpos_pub_get_int_2(unsigned char *pbuff);
unsigned int mpos_pub_get_int_4(unsigned char *pbuff);
void mpos_pub_set_int_2(unsigned int count , unsigned char *pbuff);
void mpos_pub_set_int_4(unsigned int count , unsigned char *pbuff);

int mpos_pub_atoi(char *pbuff , int len);

int mpos_pub_pkt_tlv_bin(char * buff, int offset, int tag, char * val, int length);
int mpos_pub_pkt_tlv_int2(char * buff, int offset,int tag, int val);
int mpos_pub_pkt_tlv_int(char * buff, int offset,int tag, int val);
int mpos_pub_pkt_tlv_str(char * buff, int offset,int tag, char * val);

int mpos_pub_pkt_tlv_get(char *buff, int size, int tag1, char * val);








