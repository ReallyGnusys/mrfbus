#include "g430_types.h"
#include "xb_cmd_def.h"
#include "xb_sys_structs.h"
#include "xb_sys_tasks.h"
#include "xb_usr_structs.h"





const XB_CMD const xb_cmds[] = {
  
  [ xb_cmd_ack ]         = {"ACK", XB_CFLG_INTR | XB_CFLG_NACK, sizeof(XB_PKT_ACK),  NULL, xb_task_ack},
  [ xb_cmd_device_info ] = {"DEVICE_INFO", 0,sizeof(XB_PKT_DEVICE_INFO), (void*)&device_info, NULL},
  [ xb_cmd_time ] = {"TIME", 0,sizeof(XB_PKT_TIMEDATE),(void*)&xb_timedate, xb_task_time }
  /*
,

  [ xb_cmd_modtc ] = {"MODTC", 0,sizeof(XB_PKT_MODTC) , NULL, NULL},
  [ xb_cmd_meteo1 ] = {"METEO1",0, sizeof(XB_PKT_METEO1), NULL, NULL},
  [ xb_cmd_meteo1_coeffs ] = {"METEO1_COEFFS", 0,sizeof(XB_PKT_METEO1_COEFFS), NULL, NULL}
  */
					  
};

#define XB_NUM_CMDS (sizeof(xb_cmds)/sizeof(XB_CMD))
const uint16 xb_num_cmds = (uint16)XB_NUM_CMDS;
