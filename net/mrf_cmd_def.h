#ifndef __MRF_CMD_DEF_INCLUDED__
#define __MRF_CMD_DEF_INCLUDED__
#include "mrf_types.h"

typedef enum mrf_cmd_code
 {
   mrf_cmd_ack = 0,
   mrf_cmd_resp = 1,
   mrf_cmd_device_info = 2,
   mrf_cmd_get_time = 3,
   mrf_cmd_test_1 = 4, 
   mrf_cmd_test_2 = 5
 } MRF_CMD_CODE;

#define MRF_NUM_CMDS 6




#endif
