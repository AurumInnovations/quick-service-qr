#include <stdio.h>
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_system.h"
#ifdef DEV_MF67_A10
int scan_page_proc(char *title, char *buff, int size,  int timeover)
{
	int ret=0;
	int len=0;
	st_gui_message pmsg;

	// Display scan
	//gui_begin_batch_paint();
	//gui_clear_dc();

	//gui_text_out((gui_get_width() - gui_get_text_width(title)) / 2, GUI_LINE_TOP(0), title);
	//gui_text_out(0, GUI_LINE_TOP(1), "scan...");
	//gui_end_batch_paint();

	//Sys_scaner_open();			// Open the scanning device
	Sys_scaner_start();

	while(1){
		if (gui_get_message(&pmsg, 100) == 0) {
			
			if (pmsg.message_id == GUI_KEYPRESS) {				
				if (pmsg.wparam == GUI_KEY_QUIT){
					ret = -1;
					break;
				}
			}
			else if(pmsg.message_id == GUI_SCAN_OK){

				Util_Beep(1);
				//gui_messagebox_show("", (char *)pmsg.wparam, "" , "ok", 0);	
				break;
			}
			else{
				gui_proc_default_msg(&pmsg);
			}
		}	
	}
	len = strlen((char *)pmsg.wparam);
	if (len <= size)
	{
		memcpy(buff, pmsg.wparam, len);
	}
	else
		memcpy(buff, pmsg.wparam, size);

	Sys_scaner_stop();
	//Sys_scaner_close();			// Close the scanning device
	return ret ;
}
#endif
