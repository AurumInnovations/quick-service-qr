#include "app_def.h"
#include "driver\mf_supply_power.h"
#include "driver\mf_magtek.h"

#include "AppPub\upaypage\ap_sign_page.h"
#include "libapi_xpos\inc\libapi_gui.h"

int rf = 0;

int test_devinfo( void )
{
	int index = 0;
	char szNum[64];
	char pBuf[512];

	battery_attr_t batterAttr;
	Sys_power_battery_attr(&batterAttr);


	index += sprintf(pBuf + index, "Voltage:%0.2fv\n", batterAttr.voltage/1000.00);
#ifdef WIN32
#else
	index += sprintf(pBuf + index, "%s:", Sys_Batter_Charge() == 1 ? "Electricity" : "Battery");
#endif
	switch(batterAttr.status)
	{
	case POWER_SUPPLY_STATUS_CHARGING:
		index += sprintf(pBuf + index, "%s\n", "Charging");
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		index += sprintf(pBuf + index, "%s\n", "Discharge");
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		index += sprintf(pBuf + index, "%s\n", "No Charge");
		break;
	case POWER_SUPPLY_STATUS_FULL:
		index += sprintf(pBuf + index, "%s\n", "Full");
		break;
	default:
		index += sprintf(pBuf + index, "%s\n", "Unknown");
		break;
	}

	index += sprintf(pBuf + index, "Signal:%d\n",atc_csq());
	if(atc_cpin() > 0){
		index += sprintf(pBuf + index, "SIM:Normal\n");
	}
	else{
		index += sprintf(pBuf + index, "SIM:Abnormal\n");
	}


	index += sprintf(pBuf+index,"Sys:%s\r\n",Sys_getDeviceVision());
	index += sprintf(pBuf+index,"UBOOT:%s\r\n",mf_boot_ver());
	index += sprintf(pBuf+index,"Driver:%s\r\n",mf_hal_ver());
	index += sprintf(pBuf+index,"FATFS:%s\r\n",mf_fatfs_ver());
	index += sprintf(pBuf+index,"FAL:%s\r\n",mf_fal_ver());
	index += sprintf(pBuf+index,"Hardware:%d\r\n",mf_hardware_ver());

	Sys_GetTermSn(szNum);
	index += sprintf(pBuf+index,"SN:%s\r\n", szNum );

	Sys_get_psn(szNum);
	index += sprintf(pBuf+index,"PSN:%s\r\n", szNum );


	index += sprintf(pBuf+index,"Model_type:%s\r\n",(Sys_model_type() == UCOMM_GPRS) ? "WIRELESS" : "WIFI" );

	if(Sys_model_type() == UCOMM_GPRS){
		index += sprintf(pBuf+index,"Model:%s\r\n",atc_model_ver());
		index += sprintf(pBuf+index,"IMEI:%s\r\n",comm_atc_imei());
		index += sprintf(pBuf+index,"IMSI:%s\r\n",comm_atc_imsi());


	}
	else if(Sys_model_type() == UCOMM_WIFI){
#ifndef WIN32
		index += sprintf(pBuf+index,"Wifi:%s\r\n",wifi_model_ver());
		index += sprintf(pBuf+index,"MAC:%s\r\n",wifi_get_local_mac());
#endif
	}

	gui_messagebox_show("" ,pBuf, "", "", 1000000);

	return 0;
	
}


int sdk_test_m1()
{
	st_gui_message pMsg;
	int rc;
	static int m1_init = 1;
	if (m1_init)
	{
		Icc_rfid_init();
		m1_init = 0;
	}
	
	gui_post_message(GUI_GUIPAINT, 0 , 0);
	while(1){

		if (gui_get_message(&pMsg, 500) == 0) {
			if (pMsg.message_id == GUI_GUIPAINT) {

				gui_begin_batch_paint();
				gui_set_win_rc();
				gui_clear_dc();
				gui_set_title("M1");
				gui_textout_line_left( "Please press your" , GUI_LINE_TOP(1));
				gui_textout_line_left( "M1 card" , GUI_LINE_TOP(2));

				gui_page_op_paint("" , "exit");
				gui_end_batch_paint();
			}
			if (pMsg.message_id == GUI_KEYPRESS) {
				if (pMsg.wparam == GUI_KEY_OK)	{
					break;
				}
			}
		}
		else
		{
			int rc;
			unsigned char uid[16];
			int uidlen = 0;
            int cmd = 0;
            int sector = 0;
            int block = 0;
            char tempbuf[64] = {0};
            unsigned char databuff[16] = {0};
            int datalen = 0;
                        
			rc = Icc_RfMfclOpen();//Find card
			if(rc >= 0) {

				uidlen = Icc_Rf_uid(uid);//get card sn

				if(uidlen >=0){
					Ex_Str_HexToAsc(uid , uidlen*2 , 0, tempbuf );
					gui_messagebox_show("CardID:" , tempbuf, "" , "confirm" , 0);
					Icc_RfMfclSetKey( "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" );
					cmd = 0x60;//0x60:A key authentication;0x61:B key authentication
					sector = 1;//0--15
					rc = Icc_RfMfclAuth(cmd, sector);//Authenticate sector
					if(rc == 0)
					{
						block = sector*4+0;//0--2, fourth blcok is key data  
						rc = Icc_RfMfclRead(block, databuff, &datalen);//Reads the data for the specified block
						if(rc == 0)
						{
							Ex_Str_HexToAsc(databuff , datalen*2 , 0, tempbuf );
							gui_messagebox_show("Card info:" , tempbuf, "" , "confirm" , 0);
							memcpy(databuff, "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x11\x22\x33\x44\x55\x66", 16);
							datalen = 16;
							rc = Icc_RfMfclWrite(block, databuff, datalen);//Writes the data for the specified block
							if(rc == 0)
							{
								gui_messagebox_show("RF M1" , "Write data succ", "" , "confirm" , 0);                                                                                                                                    
							}
							else
							{
								gui_messagebox_show("RF M1" , "Write data fail", "" , "confirm" , 0);                                                                                                                                       
							}
						}
						else
						{
							gui_messagebox_show("RF M1" , "Read data fail", "" , "confirm" , 0);                                                                                      
						}
                    }
					else
                    {
						gui_messagebox_show("RF M1" , "Authentication fail", "" , "confirm" , 0);
					}
				}
				else
                {
					gui_messagebox_show("RF M1" , "Get cardid fail", "" , "confirm" , 0);
					break;
				}
				gui_post_message(GUI_GUIPAINT, 0 , 0);
			}
		}
	}
	Icc_RfMfclClose();
        
	return 0;
}

int sdk_test_rf()
{
	int rc;
	unsigned char *rxbuf = 0;
	unsigned short rxlen = 0;
	unsigned char uid[16];
	int uidlen = 0;
	unsigned char cmd1[19]={"\x00\xa4\x04\x00\x0e\x32\x50\x41\x59\x2e\x53\x59\x53\x2e\x44\x44\x46\x30\x31"};

	rc = Icc_rfid_tcl_open();			// Open the card reader,Check whether the card is powered on successfully.
	rf = 1;
	if(rc >= 0) {
		uidlen = Icc_Rf_uid(uid);	// Read cpuid

		if(uidlen >=0){
			rc = Icc_Rfid_tcl_exchange(cmd1, sizeof(cmd1), &rxbuf, &rxlen);		// Tpdu data exchange
			if(rc == 0 ){
				gui_messagebox_show("RF IC" , "Send apdu succ", "" , "confirm" , 0);
			}
			else{
				gui_messagebox_show("RF IC" , "Send apdu ok", "" , "confirm" , 0);
			}
		}
		else{
			gui_messagebox_show("RF IC" , "Get uid fail", "" , "confirm" , 0);
		}
		Icc_rfid_tcl_close();

	}
	else{
		gui_messagebox_show("RF IC" , "Power on fail", "" , "confirm" , 0);
	}
//     Icc_rfid_tcl_close();

    return 0;
}

int sdk_test_felica()
{
	int rc;
	unsigned char rxbuf[256]={0};
	unsigned short rxlen = 0;
	unsigned char cmd[6]={"\x06\x00\xFF\xFF\x01\x00"};

	rc = Icc_rfid_felica_open();			// Open the card reader,Check whether the card is powered on successfully.
	if(rc >= 0) {	
		rc = Icc_rfid_felica_exchange(cmd, sizeof(cmd), rxbuf, &rxlen, 100);// select card
		if (rc == 0)
		{
			gui_messagebox_show("RF Felica" , "Select card OK", "" , "confirm" , 0);
		} 
		else
		{
			gui_messagebox_show("RF Felica" , "Select card fail", "" , "confirm" , 0);
		}
	}
	else{
		gui_messagebox_show("RF Felica" , "Power on fail", "" , "confirm" , 0);
	}

	Icc_rfid_felica_close();

	return 0;
}
