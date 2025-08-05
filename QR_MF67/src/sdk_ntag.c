#include "app_def.h"
#include "sdk_ntag.h"
#include "driver/mf_simple_rfid.h"

static unsigned char g_ntag_buff[256] = {0};
static int g_ntag_len = 0;

// This function is called by the underlying NFC driver to get the data to send.
unsigned char* ntag_get_block(int *len)
{
	*len = g_ntag_len;
	return g_ntag_buff;
}

// This function sets the raw NDEF data that the NFC chip will emulate.
void set_ntag_block(unsigned char *buff, int length)
{
	if (length > sizeof(g_ntag_buff))
	{
		APP_TRACE("NFC data too long, truncating.");
		length = sizeof(g_ntag_buff);
	}
	memset(g_ntag_buff, 0, sizeof(g_ntag_buff));
	memcpy(g_ntag_buff, buff, length);
	g_ntag_len = length;
	APP_TRACE("NFC block set with %d bytes.", length);
}

// This function analyzes a URL string and separates the prefix from the payload.
char get_url_type(const char * url, pload_data *pload)
{
	char type = URI_ID_0x00; // No prefix
	int len = 0;
	const char *p = url;

	if (memcmp(p, "http://www.", 11) == 0)
	{
		type = URI_ID_0x01;
		len = 11;
	}
	else if (memcmp(p, "https://www.", 12) == 0)
	{
		type = URI_ID_0x02;
		len = 12;
	}
	else if (memcmp(p, "http://", 7) == 0)
	{
		type = URI_ID_0x03;
		len = 7;
	}
	else if (memcmp(p, "https://", 8) == 0)
	{
		type = URI_ID_0x04;
		len = 8;
	}
	else if (memcmp(p, "tel:", 4) == 0)
	{
		type = URI_ID_0x05;
		len = 4;
	}
	else if (memcmp(p, "mailto:", 7) == 0)
	{
		type = URI_ID_0x06;
		len = 7;
	}
	else
	{
		len = 0;
	}

	pload->pyload = (char *)(url + len);
	pload->len = strlen(url) - len;
	return type;
}

// This is the new function that takes a URL, creates an NDEF message,
// and sets it for the NFC chip to emulate.
void set_ntag_url(const char *url)
{
	unsigned char ndef_msg[256];
	int offset = 0;
	pload_data pdata;
	char type;

	if (!url || strlen(url) == 0) {
		return;
	}

	// 1. Get the URL prefix type and the payload
	type = get_url_type(url, &pdata);

	// 2. Construct the NDEF message (TLV format for Type 4 tags)
	ndef_msg[offset++] = 0x03; // NDEF Message TLV
	ndef_msg[offset++] = 0x00; // Length (will be filled in later)
	ndef_msg[offset++] = 0xD1; // NDEF Record Header (TNF=Well-Known, SR, MB, ME)
	ndef_msg[offset++] = 0x01; // Type Length ('U')
	ndef_msg[offset++] = 0x00; // Payload Length (will be filled in later)
	ndef_msg[offset++] = 'U';  // Type: URI
	ndef_msg[offset++] = type; // URI Identifier Code
	memcpy(&ndef_msg[offset], pdata.pyload, pdata.len);
	offset += pdata.len;

	// 3. Fill in the lengths
	ndef_msg[4] = (unsigned char)(pdata.len + 1); // Payload Length
	ndef_msg[1] = (unsigned char)(offset - 2);    // NDEF Message Length

	// 4. Set the final message for the NFC chip
	set_ntag_block(ndef_msg, offset);
}

// This function is likely used for initial configuration or testing.
void ntag_config()
{
    set_ntag_url("https://www.morefun-et.com");
}

extern int ntag_on; // This global is defined in main.c and controls the NFC tasks

void PN5180_ntag_test()
{
	st_gui_message pMsg;
    
    // Set some default data for the test
    ntag_config(); 
    
    // Turn on the NFC tasks
    ntag_on = 1; 
    
    gui_begin_batch_paint();
    gui_clear_dc();
    gui_set_title("NFC Test");
    gui_textout_line_center("Tap phone to test NFC", GUI_LINE_TOP(3));
    gui_page_op_paint("Cancel", "");
    gui_end_batch_paint();
    
    while(1)
    {
        if (gui_get_message(&pMsg, 100) == 0) 
        {
            if (pMsg.message_id == GUI_KEYPRESS && pMsg.wparam == GUI_KEY_QUIT)
            {
                break; // Exit on Cancel key
            }
            gui_proc_default_msg(&pMsg);
        }
    }
    
    // Turn off the NFC tasks
    ntag_on = 0; 
    
    // Repaint the main screen
    gui_post_message(GUI_GUIPAINT, 0, 0);
}

int ntag_proc_init()
{
    // Initialization for the NFC process, if any, would go here.
    // The main tasks are already created in main.c
    return 0;
}