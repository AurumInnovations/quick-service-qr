#include "driver/mf_misc.h"
//#include "pub/osl/inc/osl_system.h"
//#include "pub/osl/inc/osl_BaseParam.h"
#include "../app_def.h"

static void _keySound_Paint(int value)
{
	gui_begin_batch_paint();
	
	gui_clear_dc();

	gui_text_out(0, GUI_LINE_TOP(0), value == 1 ? "KeySound:On" : "KeySound:Off");

	//xgui_TextOut(0, XGUI_LINE_TOP_2 , "Up to switch");

	gui_page_op_paint( "Cancel" , "OK");
	
	gui_end_batch_paint();
}

int keySoundSet_Show_test()
{
	int presskey;
	st_gui_message pMsg;
	int nOpen = Sys_GetKeySound();
	_keySound_Paint(nOpen);

	while(1){
		if (gui_get_message(&pMsg, 100) == 0) {

			if (pMsg.message_id == GUI_KEYPRESS) {
				presskey = pMsg.wparam;

				switch(presskey) {
				case GUI_KEY_UP: case GUI_KEY_DOWN:case GUI_KEY_LEFT:case GUI_KEY_RIGHT:case GUI_KEY_XING:case GUI_KEY_JING:
					nOpen = 1 - nOpen;
					_keySound_Paint(nOpen);
					break;
				case GUI_KEY_OK:
					Sys_SetKeySound(nOpen);
					break;
				case GUI_KEY_QUIT:
					break;
				}

				if ( presskey == GUI_KEY_QUIT || GUI_KEY_OK == presskey )
				{
					break;
				}
			}
		}
	} 
	
	return nOpen;
}

static void tts_volume_page_paint(int value)
{
	char acBuf[32+1] ={0};
	gui_begin_batch_paint();
	gui_clear_dc();

	sprintf(acBuf,"volume:%d",value);
	gui_text_out(0, GUI_LINE_TOP(0),acBuf);
	gui_text_out(0, GUI_LINE_TOP(1),"Up or Down");

	gui_page_op_paint( "Cancel" , "Ok");
	gui_end_batch_paint();
}

void tts_volume_page()
{
	int presskey;
	int iIndex;
	st_gui_message pMsg;

	iIndex = Sys_GetTtsVolume();
	tts_volume_page_paint(iIndex);
	while(1)
	{
		if (gui_get_message(&pMsg, 100) == 0) 
		{

			if (pMsg.message_id == GUI_KEYPRESS) 
			{
				presskey = pMsg.wparam;

				switch(presskey) {
				case GUI_KEY_UP:
					if(iIndex != 9)
					{
						iIndex++;
					}
					
					tts_volume_page_paint(iIndex);
					break;
				case GUI_KEY_DOWN:
					if(iIndex != 0)
					{
						iIndex--;
					}

					tts_volume_page_paint(iIndex);
					break;
				}
				
				if (presskey == GUI_KEY_UP || GUI_KEY_DOWN == presskey)
				{
					Sys_SetTtsVolume(iIndex);
					Play_Voice("payok");
				}
				else if(presskey == GUI_KEY_QUIT || GUI_KEY_OK == presskey)
				{
					break;
				}
			}
		}
	} 

	return;
}
