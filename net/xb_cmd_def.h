#ifndef __XB_CMD_DEF_INCLUDED__
#define __XB_CMD_DEF_INCLUDED__
#include "g430_types.h"


typedef enum xb_cmd_code
 {
   xb_cmd_ack = 0,
   xb_cmd_device_info = 1,
   xb_cmd_time = 2
   

   /*
   xb_cmd_dbg_uint8 = 3,
   xb_cmd_dbg_chr32 = 4,

   xb_cmd_modtc = 5,
   xb_cmd_meteo1 = 6,
   xb_cmd_meteo1_coeffs = 7,
   */
/*,
      xb_cmd_ackwake = 1 ,
      xb_cmd_retry = 2,
      xb_cmd_retry_wake = 3,
      xb_cmd_unsupported = 4,
      xb_cmd_stats = 5,
      xb_cmd_time = 6
		     */
 } XB_CMD_CODE;






#endif
