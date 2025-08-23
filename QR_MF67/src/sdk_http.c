#include "app_def.h"
#include "json/inc/json_tokener.h"

#define COMM_SOCK	m_comm_sock

static int m_connect_tick = 0;
static int m_connect_time = 0;
static int m_connect_exit = 0;
static int m_comm_sock = 1;

#define HTTP_ERR_NO200		-1
#define HTTP_ERR_OTHER		-2
#define HTTP_ERR_CANCEL		-3
#define HTTP_ERR_TIMEOUT	-4

// Pack the http header, the following code is the demo code, please refer to the http specification for the specific format.
static void http_pack(char * buff, char *ip, int port, char * msg)
{
	int index = 0;
	int msgl =strlen(msg);
	index += sprintf(buff + index , "POST %s HTTP/1.1\r\n", "/test");
	index += sprintf(buff + index , "HOST: %s:%d\r\n" , ip, port );
	index += sprintf(buff + index , "Connection: Keep-Alive\r\n");
	index += sprintf(buff + index , "Content-Length: %d\r\n", msgl);
	index += sprintf(buff + index , "\r\n");

	index += sprintf(buff + index , "%s", msg);
}

// Parse the field of http
static char *http_headvalue( char *heads ,const char *head )
{
	char *pret = (char *)strstr( heads ,head );
	if ( pret != 0 )	{
		pret += strlen(head);
		pret += strlen(": ");
	}
	return pret;

}

// Parsing the status code of http
static int http_StatusCode(char *heads)
{	
	char *rescode = (char *)strstr( heads ," " );
	if ( rescode != 0 )	{
		rescode+=1;
		return atoi( rescode);
	}
	else{
		
	}

	return -1;
}

// Parse the length of http
static int http_ContentLength(char *heads)
{
	char *pContentLength = http_headvalue( heads ,"Content-Length" );
	if ( pContentLength != 0)	{
		return atoi( pContentLength );
	}
	return -1;
}

void comm_page_set_page(const char * title , const char * msg ,int quit)
{
	gui_begin_batch_paint();
	gui_set_win_rc();
	gui_clear_dc();
	gui_set_title((char *)title);

	gui_text_out(0 , GUI_LINE_TOP(1)  , (char *)msg);
	if (quit == 1){
		gui_page_op_paint("Cancel" , "");
	}

	gui_end_batch_paint();
}

int comm_page_get_exit()
{
	st_gui_message pMsg;
	int ret = 0;
	int get_ret = 0;

	while(ret == 0){
		ret = gui_get_message(&pMsg, 0);

		if ( ret == 0 ){
			if (pMsg.message_id == GUI_KEYPRESS && pMsg.wparam == GUI_KEY_QUIT){
				get_ret = 1;
			}
			gui_proc_default_msg(&pMsg);
		}
	}

	return get_ret;
}

static int _connect_server_func_proc()
{
	char msg[32] = {0};
	int ret = 0;
	int num = 0;
	int time = 30;

	if(comm_page_get_exit() == 1){
		m_connect_exit = 1;
	}

	num = time - Sys_GetTickDiff(m_connect_tick) / 1000;
	
	if(num <= 0){
		ret = 1;
	}
	else if(m_connect_exit == 0){
		sprintf(msg , "Connect...(%d)" , num);
		comm_page_set_page("Http", msg , 1);
	}
	else{
		sprintf(msg , "Canceling...");
		comm_page_set_page("Http", msg , 0);
		ret = 2;
	}
	

	return ret ;
}
static int app_https_recv(int sock, char * pdata, int size)
{
	int ret;
	static int timer = 0;
	if ( Sys_TimerCheck( timer ) == 0)
	{
		comm_ssl_recv3(sock);//clear receive buffer
		timer = Sys_TimerOpen( 1000 );
	}
	ret = comm_ssl_recv(sock,(unsigned char *)pdata ,size);

	return ret;
}
// Http receiving processing, the step is to receive the byte stream from the sock to parse out the header status code and length and other fields, and then receive the http body according to the length
static int generic_http_recv(int sock, char *buff, int len, int timeover, int (*recv_func)(int, unsigned char*, int, unsigned int), const char* page_title)
{
	int index = 0;
	unsigned int tick1;
	char msg[32];
	char *phttpbody;
	int nHttpBodyLen;
	int nHttpHeadLen;

	tick1 = Sys_TimerOpen(timeover);

	// Receive http packets cyclically until timeout or reception is complete
	while (Sys_TimerCheck(tick1) > 0 && index < len ){	
		int ret;
		int num;

		num = Sys_TimerCheck(tick1) /1000;
		num = num < 0 ? 0 : num;
		sprintf(msg , "%s(%d)" , "Recving" , num);

		comm_page_set_page(page_title, msg , 1);
		ret = comm_page_get_exit();

		if(ret == 1){ 
			return -2;
		}
		// 
		ret = recv_func(sock, (unsigned char *)(buff + index), len - index, 700);

		if(ret >= 0){
			index += ret;

			buff[index] = '\0';
			phttpbody = (char *)strstr( buff ,"\r\n\r\n" ); // Http headend
			if(phttpbody!=0){
				//char *p;				
				int nrescode = http_StatusCode(buff);
				SYS_TRACE( "http rescode: %d",nrescode );
				if ( nrescode == 200){	

					nHttpHeadLen = phttpbody + 4 - buff;	

					nHttpBodyLen = http_ContentLength(buff);

					SYS_TRACE("HeadLen:%d  Content-Length: %d",nHttpHeadLen,nHttpBodyLen);

					//if(nHttpBodyLen<=0) return -1;

					if (index >= nHttpHeadLen+nHttpBodyLen){		
						//The receiving length is equal to the length of the head plus
						memcpy(buff , phttpbody + 4 , nHttpBodyLen);
						SYS_TRACE( "http recv body: %s", buff );
						return nHttpBodyLen;	// Receive completed, return to receive length
					}
					return ret;
				}
				else{  //not 200

					return 0;
				}
			}


		}
		else {
			return -1;
		}	
	}

	return -1;
}

static int http_recv(int sock, char *buff, int len, int timeover)
{
    return generic_http_recv(sock, buff, len, timeover, comm_sock_recv, "Http");
}

// Wrapper for app_https_recv to match the signature of comm_sock_recv
static int https_recv_wrapper(int s, unsigned char* b, int l, unsigned int t) {
	(void)t; // timeout is not used by app_https_recv
	return app_https_recv(s, (char*)b, l);
}

static int https_recv(int sock, char *buff, int len, int timeover)
{
	// Use the generic receiver with the https wrapper function
	return generic_http_recv(sock, buff, len, timeover, https_recv_wrapper, "Https");
}


static void http_test()
{
	char *msg = "hello world!";
	char *buff = Util_Malloc(2048);
	char *recv = Util_Malloc(2048);
	char *ip = "www.baidu.com";//119.75.217.109
	int port = 80;
	//char *ip = "120.40.105.37";//119.75.217.109
	//int port = 6608;	
    int ret = -1;
	int sock = SOCK_INDEX1;
	char apn[32]="";
	int i;
	int nret;	
	int nTime = 3;

	memset(buff, 0, 2048);
	memset(recv, 0, 2048);
	http_pack(buff, ip, port, msg);		//  Package http data

	comm_page_set_page("Http", "Link..." , 0);
	nret = comm_net_link("Link", apn ,30000);		// 30s
	if (nret != 0)
	{
		gui_messagebox_show("Http" , "Link Fail", "" , "confirm" , 0);
		return;
	}

	COMM_SOCK = comm_sock_create(sock);	//  Create sock


	for ( i = 0 ; i < nTime ; i ++)
	{		
		char tmp[32] = {0};

		m_connect_tick = Sys_TimerOpen(30000);
		m_connect_exit = 0;
		m_connect_time = i+1;

		SYS_TRACE("connect server ip=%s, port=%d\r\n", ip,  port);

		sprintf(tmp , "Connect server%dtimes" , i + 1);
		comm_page_set_page("Http", tmp , 1);

		ret = comm_sock_connect2(COMM_SOCK, ip, port,(void *)_connect_server_func_proc);	//  Connect to http server
		SYS_TRACE( "--------------------comm_sock_connect: %d ---------------------------\r\n", ret );
		if (ret == 0)			
			break;

		comm_sock_close(COMM_SOCK);
		Sys_Delay(500);
	}



	SYS_TRACE("connect server ret= %d,%s:%d\r\n" , ret , ip , port);

	if(ret == 0){
		comm_sock_send(COMM_SOCK , (unsigned char *)buff ,  strlen(buff));	// 		Send http request
		ret = http_recv(COMM_SOCK, recv, 1024, 30000);		// Receive http response
		if (ret >= 0)
		{
			sprintf(buff, "recv buff:%s", recv);
			SYS_TRACE("recv buff:%s", recv);
			gui_messagebox_show("Http" , buff, "" , "confirm" , 0);		
		}
		else
		{
			gui_messagebox_show("Http" , "Recv Fail", "" , "confirm" , 0);
		}
	}
	else {
		gui_messagebox_show("Http" , "Connect Fail", "" , "confirm" , 0);
	}

	comm_sock_close(COMM_SOCK);	   

	comm_net_unlink();

	Util_Free(buff);
	Util_Free(recv);
}


static void https_test()
{
	char *msg = "hello world!";//080022380000008000009A00000530081329000001081329053020390013
	char *buff = Util_Malloc(2048);
	char *recv = Util_Malloc(2048);
	char *ip = "www.baidu.com";//104.27.134.11in.tms.morefun-et.com
	int port = 443;
	int ret = - 1;
	char apn[32]="";
	int i;
	int nret;	
	int nTime = 3;

	//mbed_ssl_set_log(10); 
	memset(buff, 0, 2048);
	memset(recv, 0, 2048);	
	http_pack(buff, ip, port, msg);		//  Package http data
	comm_page_set_page("Https", "Link..." , 0);
	nret = comm_net_link( "Link" , apn,  30000);
	if (nret != 0)
	{
		gui_messagebox_show("Https" , "Link Fail", "" , "confirm" , 0);
		return;
	}

	COMM_SOCK = comm_sock_create(SOCK_INDEX1);	//  Create sock


	for ( i = 0 ; i < nTime ; i ++)
	{

		char tmp[32] = {0};

		m_connect_tick = Sys_TimerOpen(30000);
		m_connect_exit = 0;
		m_connect_time = i+1;

		SYS_TRACE("connect server ip=%s, port=%d\r\n", ip,  port);
		SYS_TRACE("--------------------comm_ssl_init---------------------------\r\n" );

		comm_ssl_init(COMM_SOCK,0,0,0,0);	
		sprintf(tmp , "Connect server%dtimes" , i + 1);
		comm_page_set_page("Https", tmp , 1);

		//c_ret = comm_ssl_connect(COMM_SOCK, ip, port);	//  Connect to http server
		ret = comm_ssl_connect2(COMM_SOCK, ip, port, (void *)_connect_server_func_proc);	//  Connect to http server
		SYS_TRACE( "--------------------comm_ssl_connect: %d ---------------------------\r\n", ret );

		if (comm_page_get_exit() || m_connect_exit == 1)	{
			SYS_TRACE("comm_func_connect_server Cancel" );
			comm_ssl_close(COMM_SOCK);
			ret = -1;
			break;
		}

		if (ret == 0){		
			break;
		}

		comm_ssl_close(COMM_SOCK);
		Sys_Delay(500);
	}	


	SYS_TRACE("connect server ret= %d,%s:%d\r\n" , ret , ip , port);

	if(ret == 0){
// 		sprintf(buff + strlen(buff), "GET https://%s:%d/ HTTP/1.1\r\n", ip , port);
// 		sprintf(buff + strlen(buff), "nHost: %s:%d\r\n", ip , port);
// 		sprintf(buff + strlen(buff), "Content-Length:0\r\n");
// 		sprintf(buff + strlen(buff), "Content-Type:application/x-www-form-urlencoded;charset=UTF-8\r\n\r\n");
		comm_ssl_send(COMM_SOCK , buff ,  strlen(buff));		// 		Send http request		
		SYS_TRACE("buff_send:%s",buff);
		ret = https_recv(COMM_SOCK, recv, 2048, 30000);		// Receive http response
		if (ret >= 0)
		{
			sprintf(buff, "recv buff:%s", recv);
			SYS_TRACE("recv buff:%s", recv);
			gui_messagebox_show("Https" , buff, "" , "confirm" , 0);
		}
		else
		{
			gui_messagebox_show("Https" , "Recv Fail", "" , "confirm" , 0);
		}
	}
	else
	{
		gui_messagebox_show("Https" , "Connect Fail", "" , "confirm" , 0);
	}		

	comm_ssl_close(COMM_SOCK);

	comm_net_unlink();
	Util_Free(buff);
	Util_Free(recv);
}

static void json_test()
{
	struct json_object *rootobj = 0;
	struct json_object *itemobj = 0;
	char str[128] = {0};
	char * key = "code";
	char *pdata = 0;

	strcpy(str, "{\"success\":\"false\",\"code\":\"1001\"}");
	rootobj = json_tokener_parse(str);

	if(((int)rootobj) >= 0){
		itemobj = (struct json_object *) json_object_object_get(rootobj, key);	

		if (((int)itemobj)> 0){
			pdata = json_object_to_json_string(itemobj);
		}

		json_object_put(rootobj);
	}
	
}


void sdk_http_test()
{
	http_test();	

}

void sdk_https_test()
{
	https_test();
}

// Helper to parse URL into host, port, and path
static int parse_url(const char *url, char *host, int host_len, int *port, char *path, int path_len) {
    char host_port[128] = {0};
    const char *p_host_start, *p_path_start, *p_port_sep;

    if (strstr(url, "://") == NULL) {
        return -1; // Invalid URL format
    }

    p_host_start = strstr(url, "://") + 3;
    p_path_start = strchr(p_host_start, '/');
    
    if (p_path_start) {
        int host_port_len = p_path_start - p_host_start;
        if (host_port_len >= sizeof(host_port)) return -1;
        memcpy(host_port, p_host_start, host_port_len);
        host_port[host_port_len] = '\0';
        
        strncpy(path, p_path_start, path_len - 1);
        path[path_len - 1] = '\0';
    } else {
        strncpy(host_port, p_host_start, sizeof(host_port) - 1);
        host_port[sizeof(host_port) - 1] = '\0';
        strcpy(path, "/");
    }

    p_port_sep = strchr(host_port, ':');
    if (p_port_sep) {
        int host_len_val = p_port_sep - host_port;
        if (host_len_val >= host_len) return -1;
        memcpy(host, host_port, host_len_val);
        host[host_len_val] = '\0';
        *port = atoi(p_port_sep + 1);
    } else {
        strncpy(host, host_port, host_len - 1);
        host[host_len - 1] = '\0';
        // Check protocol for default port
        if (strncmp(url, "https", 5) == 0) {
            *port = 443;
        } else {
            *port = 80;
        }
    }

    return 0;
}

void sdk_ota_update(const char *url)
{
    char host[128] = {0};
    int port = 80;
    char path[256] = {0};
    char *request_buff = NULL;
    char *recv_buff = NULL;
    int sock = -1;
    int ret = -1;
    int fp = -1;
    const char *save_path = "data\\update.pack";

    // 1. Parse URL
    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0) {
        gui_messagebox_show("Update Error", "Invalid URL", "", "OK", 5000);
        return;
    }

    // 2. Allocate buffers
    request_buff = Util_Malloc(512);
    recv_buff = Util_Malloc(2048);
    if (!request_buff || !recv_buff) {
        gui_messagebox_show("Update Error", "Memory allocation failed", "", "OK", 5000);
        goto cleanup;
    }

    // 3. Link network
    comm_page_set_page("System Update", "Connecting...", 0);
    if (comm_net_link("Link", "", 30000) != 0) {
        gui_messagebox_show("Update Error", "Network link failed", "", "OK", 5000);
        goto cleanup;
    }

    // 4. Connect to server
    sock = comm_sock_create(SOCK_INDEX1);
    comm_page_set_page("System Update", "Connecting to server...", 1);
    if (comm_sock_connect(sock, host, port) != 0) {
        gui_messagebox_show("Update Error", "Server connection failed", "", "OK", 5000);
        goto cleanup;
    }

    // 5. Send HTTP GET request
    sprintf(request_buff, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    comm_sock_send(sock, (unsigned char *)request_buff, strlen(request_buff));

    // 6. Receive response and download file
    comm_page_set_page("System Update", "Downloading...", 1);
    UFile_Remove((char*)save_path, FILE_PRIVATE); // Delete old update file
    if (UFile_OpenCreate((char*)save_path, FILE_PRIVATE, FILE_CREAT, &fp, 0) != UFILE_SUCCESS) {
        gui_messagebox_show("Update Error", "Failed to create update file", "", "OK", 5000);
        goto cleanup;
    }

    int total_bytes_read = 0;
    int content_length = -1;
    char *body_start = NULL;
    unsigned int tick1 = Sys_TimerOpen(120000); // 2 minute timeout for download

    while (Sys_TimerCheck(tick1) > 0) {
        if (comm_page_get_exit() == 1) {
            gui_messagebox_show("Update", "Cancelled by user", "", "OK", 3000);
            goto cleanup;
        }

        ret = comm_sock_recv(sock, (unsigned char *)recv_buff, 2048 - 1, 1000);
        if (ret > 0) {
            recv_buff[ret] = '\0';
            if (!body_start) {
                body_start = strstr(recv_buff, "\r\n\r\n");
                if (body_start) {
                    content_length = http_ContentLength(recv_buff);
                    body_start += 4;
                    int body_len = ret - (body_start - recv_buff);
                    if (body_len > 0) {
                        UFile_Write(fp, body_start, body_len);
                        total_bytes_read += body_len;
                    }
                }
            } else {
                UFile_Write(fp, recv_buff, ret);
                total_bytes_read += ret;
            }

            if (content_length > 0) {
                char progress_msg[64];
                sprintf(progress_msg, "Downloading... %d%%", (total_bytes_read * 100) / content_length);
                comm_page_set_page("System Update", progress_msg, 1);
            }

            if (content_length > 0 && total_bytes_read >= content_length) break;
        } else if (ret == 0) {
            break; // Connection closed by server
        }
    }

    UFile_Close(fp);
    fp = -1;

    if (total_bytes_read > 0 && (content_length == -1 || total_bytes_read == content_length)) {
        gui_messagebox_show("System Update", "Download complete. Installing...", "", "OK", 3000);
        tms_update(save_path);
        gui_messagebox_show("Update Error", "Installation failed.", "", "OK", 5000);
    } else {
        gui_messagebox_show("Update Error", "Download failed or incomplete.", "", "OK", 5000);
    }

cleanup:
    if (sock != -1) comm_sock_close(sock);
    if (fp != -1) UFile_Close(fp);
    comm_net_unlink();
    if (request_buff) Util_Free(request_buff);
    if (recv_buff) Util_Free(recv_buff);
}