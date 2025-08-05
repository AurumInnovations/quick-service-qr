#include "app_def.h"
#include "driver/mf_driverlib.h"
#include "pub\osl\inc\osl_filedef.h"
#include "pub/osl/inc/osl_init.h"
#include "pub/inc/taskdef.h"
#include "ucosii/ucos_ii.h"
#include "driver/mf_driverlib.h"
#include "pub/osl/inc/osl_BaseParam.h"
#include "pub/common/misc/inc/mfmalloc.h"
#include "pub\tracedef.h"
#include "pub/osl/inc/osl_filedef.h"
#include "sdk_ntag.h"

#pragma data_alignment=8
#define LOGOIMG "data\\logo2.bmp"

#ifdef DEV_MF67_A10
#define APP_VER "paydemo-V2.1.0"
#else
#define APP_VER "paydemo-V1.1.8.5"
#endif

#define APP_TASK_MIN_PRIO	30
#define _APP_TASK_SIZE		(1024+1024)			// Thread stack size
#define _APP_TASK_PRIO		(APP_TASK_MIN_PRIO + 3)	// Thread priority
static unsigned int pTaskStk[_APP_TASK_SIZE];					// Thread stack

#ifdef PN5180
#define _NTAG_TASK_SIZE		1024
#define _NTAG_TASK_PRIO		(APP_TASK_MIN_PRIO + 1)
static unsigned int pTaskStk_lib[_NTAG_TASK_SIZE];
static unsigned int pTaskStk_app[_NTAG_TASK_SIZE];
int ntag_on = 0;
OS_EVENT  *rfid_lib_sem;
OS_EVENT  *rfid_app_sem;
#define CREAT_LIB_LOCK		rfid_lib_sem =OSSemCreate((unsigned short)1);
#define CREAT_APP_LOCK		rfid_app_sem =OSSemCreate((unsigned short)1);
#define LOCK_LIB			{unsigned char nErr;  OSSemPend((OS_EVENT *)rfid_lib_sem,TIMEOUT_INFINITE, &nErr);}
#define LOCK_APP			{unsigned char nErr;  OSSemPend((OS_EVENT *)rfid_app_sem,TIMEOUT_INFINITE, &nErr);}
#define UNLOCK_LIB			OSSemPost((OS_EVENT *)rfid_lib_sem);
#define UNLOCK_APP			OSSemPost((OS_EVENT *)rfid_app_sem);

int rfid_lib_lock(int status)
{
	if(status) {
		LOCK_LIB
	} else {
		UNLOCK_LIB
	}
	return 0;
}

int rfid_app_lock(int status)
{
	if(status) {
		LOCK_APP
	} else {
		UNLOCK_APP
	}
	return 0;
}

int rfid_delay(int ms)
{
	Sys_Delay(1);
}

void PN5180_init()
{
	CREAT_LIB_LOCK
	CREAT_APP_LOCK
	mf_rfid_hce_syncobj(rfid_lib_lock, rfid_app_lock, rfid_delay);
}

void start_task_lib(void *pdata)
{
	int i = 1;
	while (1)
	{
		while (ntag_on)
		{
			if (i)
			{
				mf_rfid_hce_auto_coll();
				i = 0;
			}
			mf_rfid_hce_lib();
// 			APP_TRACE("hcelib alive");
			Sys_Delay(1);
		}
		if (ntag_on == 0)
		{
			APP_TRACE("hcelib sleep");
			Sys_Delay(500);
			if (!i)
			{
				i = 1;
			}	
		}
	}
}

void start_task_app(void* pdata)
{
	while (1)
	{
		while (ntag_on)
		{
			mf_rfid_hce_app();
			Sys_Delay(1);
		}
		if (ntag_on == 0)
		{
// 			APP_TRACE("applib sleep");
			Sys_Delay(500);
		}
	}
}
#endif
static void showlogo()
{
	int logowidth;
	int logoheight;
	int logoleft;
	int logotop;
	int logocolor;
	char * pbmp;
#if 1
	pbmp = gui_load_bmp_ex(LOGOIMG, &logowidth , &logoheight, &logocolor);
	if (pbmp != 0){
		gui_begin_batch_paint();
		gui_set_win_rc();
		gui_clear_dc();   
        logoleft = (gui_get_width()-logowidth)/2;		
		logotop = (gui_get_height()-logoheight)/2;
		gui_out_bits_ex(logoleft, logotop, pbmp , logowidth , logoheight , 0 , logocolor);
		Util_Free(pbmp);
		gui_end_batch_paint(); 
		Sys_Delay(1000); 
	} 
#endif
}

static void app_init()
{
	Sys_setAppVision(APP_VER);	// Set the application version
	showlogo();			//Display boot logo
	//set_func_init();
	mqtt_proc_init();
 	sdk_main_page();     
}

static void _AppWorkTask(void * pParam)
{
	//unsigned char *buf = malloc(256);
	mf_usb_set_cfg_mode(1);//set 1 to enable hid

	//osl_log_sound_set(0);
	
    //sc_set_otp_str("985KP#52");
#ifdef WIN32
	Sys_init();
	set_malloc_log(0);
#else
#ifdef SOCKET_SUPPORT
	Sys_init_mini(1);
#else
	Sys_init_mini(0);
#endif
	set_malloc_log(0);
#endif
#ifdef DEV_MF67_A10
	Sys_qr_set_buff();
	tms_init();
#endif
	//mf_usb_cdc_setfifo(buf, 256);//set buffer size
	app_set_bt_name();
#ifdef BLUETOOTH_MODE_SET
	bluetooth_set_pairing_mode(0);//set bluetooth mode
	bt_yc_set_patch_2480();
#else
	//bluetooth_set_patch2();
	bt_yc_set_patch_5864();
#endif
	
	bluetooth_init();
#ifdef PN5180
	PN5180_init();
	OSTaskCreate(start_task_lib, (void *)102, (OS_STK *)pTaskStk_lib, _NTAG_TASK_PRIO);
	OSTaskCreate(start_task_app, (void *)105, (OS_STK *)pTaskStk_app, _NTAG_TASK_PRIO + 1);
	mf_rfid_hce_open();

#endif
	app_init();	
}

void app_main()
{
	//mf_usb_set_cfg_mode(1);//disable hid(only cdc)
	Sys_driverlib_init();			 // Driver initialization					 // ucosii initialization
	// Create an application thread
	Sys_TaskCreate(_AppWorkTask,_APP_TASK_PRIO,(char *)pTaskStk, _APP_TASK_SIZE);				// 	ucosii run
	while(1) Sys_Delay(1000);
}

#ifndef WIN32
void main()
{
	if(Sys_get_is_1903() == 0){
		app_main(0);
	}
	else{
		mf_start_apps(app_main);
	}
}
#endif