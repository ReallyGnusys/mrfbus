#ifndef __MRF_CMDS_INCLUDED__
#define __MRF_CMDS_INCLUDED__

#include "mrf_types.h"
#include "mrf_cmd_def.h"
#include "mrf_sys_structs.h"
#include "mrf_sys_tasks.h"
#include "mrf_usr_structs.h"



extern const MRF_PKT_DEVICE_INFO device_info;


const MRF_CMD const mrf_cmds[] = {
  [ mrf_cmd_ack         ] = {"ACK"        , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , 0                          , 0                          ,  NULL               , mrf_task_ack      },
  [ mrf_cmd_retry       ] = {"RETRY"      , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , 0                          , 0                          ,  NULL               , mrf_task_retry      },
  [ mrf_cmd_resp        ] = {"RESP"       , MRF_CFLG_INTR | MRF_CFLG_NO_RESP, 0                          , 0                          ,  NULL               , mrf_task_resp     },
  [ mrf_cmd_device_info ] = {"DEVICE_INFO", MRF_CFLG_INTR                   , 0                          , sizeof(MRF_PKT_DEVICE_INFO),  (void*)&device_info, NULL },
  [ mrf_cmd_if_info     ] = {"IF_INFO"    , MRF_CFLG_INTR                   , 0                          , sizeof(MRF_PKT_IF_INFO)    ,  NULL               , mrf_task_if_info  },
  [ mrf_cmd_if_status   ] = {"IF_STATUS"  , MRF_CFLG_INTR                   , sizeof(MRF_PKT_IF_STAT_REQ), sizeof(IF_STATUS      )    ,  NULL               , mrf_task_if_status  },
  [ mrf_cmd_get_time    ] = {"GET_TIME"   , MRF_CFLG_INTR                   , 0                          ,   sizeof(MRF_PKT_TIMEDATE) ,  NULL               , mrf_task_get_time },
  [ mrf_cmd_set_time    ] = {"SET_TIME"   , MRF_CFLG_INTR                   ,sizeof(MRF_PKT_TIMEDATE)    , 0                          ,  NULL               , mrf_task_set_time   },
  [ mrf_cmd_sensor_data ] = {"SENSOR_DATA", 0                               ,0                           , 0                          ,  NULL               , mrf_task_sensor_data },
  [ mrf_cmd_read_sensor ] = {"READ_SENSOR", 0                               ,0                           , 0                          ,  NULL          , mrf_task_read_sensor   },
  [ mrf_cmd_test_1      ] = {"TEST_1"     , 0                               ,0                           , sizeof(MRF_PKT_DBG_CHR32)  ,  NULL          , mrf_task_test_1   },
  [ mrf_cmd_test_2      ] = {"TEST_2"     , 0                               ,0                           , 0                          ,  NULL               , mrf_task_test_2   },
	  
};

const uint16 mrf_num_cmds = (uint16)MRF_NUM_CMDS;

#endif
