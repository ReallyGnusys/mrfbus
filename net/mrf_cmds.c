#ifndef __MRF_CMDS_INCLUDED__
#define __MRF_CMDS_INCLUDED__

#include "mrf_types.h"
#include "mrf_cmd_def.h"
#include "mrf_sys_structs.h"
#include "mrf_sys_tasks.h"
#include "mrf_usr_structs.h"



extern const MRF_PKT_DEVICE_INFO device_info;


const MRF_CMD const mrf_cmds[] = {
  [ mrf_cmd_ack         ] = {"ACK"        , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , sizeof(MRF_PKT_ACK)        , NULL               , mrf_task_ack      },
  [ mrf_cmd_resp        ] = {"RESP"       , MRF_CFLG_INTR | MRF_CFLG_NO_RESP, sizeof(MRF_PKT_RESP)       , NULL               , mrf_task_resp     },
  [ mrf_cmd_device_info ] = {"DEVICE_INFO", 0                               , sizeof(MRF_PKT_DEVICE_INFO), (void*)&device_info, NULL              },
  [ mrf_cmd_get_time    ] = {"GET_TIME"   , 0                               , sizeof(MRF_PKT_TIMEDATE)   , NULL               , mrf_task_get_time },
  [ mrf_cmd_test_1      ] = {"TEST_1"     , 0                               ,0                           , NULL               , mrf_task_test_1   },
  [ mrf_cmd_test_2      ] = {"TEST_2"     , 0                               ,0                           , NULL               , mrf_task_test_2   }	  
};

const uint16 mrf_num_cmds = (uint16)MRF_NUM_CMDS;

#endif
