#pragma once

typedef struct __pload_data{
	int len;
	const char * pyload;
}pload_data;

void set_ntag_block(unsigned char *buff, int length);
void set_ntag_url(const char *url);
void ntag_config();
void sdk_ntag_test(void);
int ntag_proc_init();
char get_url_type(const char * url, pload_data *pload);

#define URI_ID_0x00                 0x00
#define URI_ID_0x01                 0x01                //"http://www."
#define URI_ID_0x02                 0x02                //"https://www."
#define URI_ID_0x03                 0x03                //"http://"
#define URI_ID_0x04                 0x04				//"https://"
#define URI_ID_0x05                 0x05				//"tel:"
#define URI_ID_0x06                 0x06				//"mailto:"
#define URI_ID_0x07                 0x07				//"ftp://anonymous:anonymous@"
#define URI_ID_0x08                 0x08				//"ftp://ftp."
#define URI_ID_0x09                 0x09				//"ftps://"
#define URI_ID_0x0A                 0x0A				//"sftp://"
#define URI_ID_0x0B                 0x0B				//"smb://"
#define URI_ID_0x0C                 0x0C				//"nfs://"
#define URI_ID_0x0D                 0x0D				//"ftp://"
#define URI_ID_0x0E                 0x0E				//"dav://"
#define URI_ID_0x0F                 0x0F				//"news:"
#define URI_ID_0x10                 0x10				//"telnet://"
#define URI_ID_0x11                 0x11				//"imap:"
#define URI_ID_0x12                 0x12				//"rtsp://"
#define URI_ID_0x13                 0x13				//"urn:"
#define URI_ID_0x14                 0x14				//"pop:"
#define URI_ID_0x15                 0x15				//"sip:"
#define URI_ID_0x16                 0x16				//"sips:"
#define URI_ID_0x17                 0x17				//"tftp:"
#define URI_ID_0x18                 0x18				//"btspp://"
#define URI_ID_0x19                 0x19				//"btl2cap://"
#define URI_ID_0x1A                 0x1A				//"btgoep://"
#define URI_ID_0x1B                 0x1B				//"tcpobex://"
#define URI_ID_0x1C                 0x1C				//"irdaobex://"
#define URI_ID_0x1D                 0x1D				//"file://"
#define URI_ID_0x1E                 0x1E				//"urn:epc:id:"
#define URI_ID_0x1F                 0x1F				//"urn:epc:tag"
#define URI_ID_0x20                 0x20				//"urn:epc:pat:"
#define URI_ID_0x21                 0x21				//"urn:epc:raw:"
#define URI_ID_0x22                 0x22				//"urn:epc:"
#define URI_ID_0x23                 0x23				//"urn:nfc:"

//#define PN5180 1

void PN5180_ntag_test();