#ifndef __MRF_CMD_DEF_INCLUDED__
#define __MRF_CMD_DEF_INCLUDED__
#include "mrf_types.h"

typedef enum mrf_cmd_code
 {
   mrf_cmd_ack = 0,
   mrf_cmd_retry = 1,
   mrf_cmd_resp = 2,
   mrf_cmd_device_info = 3,
   mrf_cmd_if_info = 4,
   mrf_cmd_if_status = 5,
   
   mrf_cmd_get_time = 6,
   mrf_cmd_test_1 = 7, 
   mrf_cmd_test_2 = 8
 } MRF_CMD_CODE;

#define MRF_NUM_CMDS 9




#endif
