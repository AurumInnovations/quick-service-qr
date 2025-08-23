#ifndef APP_DEF_H_
#define APP_DEF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>



//SDK head file
#include "emvapi/inc/emv_api.h"
#include "libapi_xpos/inc/def.h"
#include "libapi_xpos/inc/libapi_comm.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "libapi_xpos/inc/libapi_file.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_iccard.h"
#include "libapi_xpos/inc/libapi_print.h"
#include "libapi_xpos/inc/libapi_pub.h"
#include "libapi_xpos/inc/libapi_rki.h"
#include "libapi_xpos/inc/libapi_security.h"
#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_util.h"
#include "libapi_xpos/inc/libapi_version.h"

//pub head file
#include "pub/inc/taskdef.h"
#include "pub/tracedef.h"
#include "pub/osl/inc/osl_system.h"
#include "pub/common/misc/inc/mfmalloc.h"
#include "pub/osl/inc/osl_time.h"

//mbedtls head file
#include "mbedtls/include/mbedtls/sha1.h"
#include "mbedtls/include/mbedtls/md.h"

//app head file
#include "mpos_func.h"
#include "mpos_proc.h"
#include "mpos_pub.h"

#include "sdk_driver.h"
#include "sdk_file.h"
#include "sdk_http.h"
#include "sdk_img.h"
#include "sdk_log.h"
#include "sdk_showqr.h"
#include "sdk_timer.h"
#include "sdk_xgui.h"
#include "test/test_pub.h"
#include "settings/admin_set_func.h"

#include "cJSON/cJSON.h"

#define SOCKET_SUPPORT  1
//#define BLUETOOTH_MODE_SET 1 //set bluetooth mode to PIN-need
//#define DATAVAN 1

#ifdef BLUETOOTH_MODE_SET
#define XM_BT_LINK_RND			0x000E0031
#define XM_BT_LINK_PAGE			0x000E0100
#define XM_BT_LINK_SUCC			0x000E0101
#endif















#endif
