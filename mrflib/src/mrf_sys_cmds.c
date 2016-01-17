#include "mrf_sys.h"

extern const MRF_PKT_DEVICE_INFO device_info;
extern const MRF_PKT_SYS_INFO    sys_info;

const MRF_CMD mrf_sys_cmds[MRF_NUM_SYS_CMDS] = {
  [ mrf_cmd_ack          ] = {"ACK"        , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , 0                          , 0                          ,  NULL               , mrf_task_ack      },
  [ mrf_cmd_retry        ] = {"RETRY"      , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , 0                          , 0                          ,  NULL               , mrf_task_retry      },
  [ mrf_cmd_resp         ] = {"RESP"       , MRF_CFLG_INTR | MRF_CFLG_NO_RESP, 0                          , 0                          ,  NULL               , mrf_task_resp     },
  [ mrf_cmd_device_info  ] = {"DEVICE_INFO", MRF_CFLG_INTR                   , 0                          , sizeof(MRF_PKT_DEVICE_INFO),  (void*)&device_info, NULL },
  [ mrf_cmd_device_status] = {"DEVICE_STATUS"    , MRF_CFLG_INTR             , 0                          , sizeof(MRF_PKT_DEVICE_STATUS)    ,  NULL               , mrf_task_device_status },
  [ mrf_cmd_sys_info  ]    = {"SYS_INFO"   , MRF_CFLG_INTR                , 0                          , sizeof(MRF_PKT_SYS_INFO),  (void*)&sys_info, NULL },
  [ mrf_cmd_if_stats    ] = {"IF_STATUS"  , MRF_CFLG_INTR                    , sizeof(MRF_PKT_UINT8)     , sizeof(IF_STATS      )    ,  NULL               , mrf_task_if_status  },
  [ mrf_cmd_get_time    ] = {"GET_TIME"   , MRF_CFLG_INTR                   , 0                          , sizeof(MRF_PKT_TIMEDATE)   ,  NULL               , mrf_task_get_time },
  [ mrf_cmd_set_time    ] = {"SET_TIME"   , MRF_CFLG_INTR                   , sizeof(MRF_PKT_TIMEDATE)   , sizeof(MRF_PKT_TIMEDATE)   ,  NULL               , mrf_task_set_time   },
  [ mrf_cmd_buff_state ] = {"BUFF_STATE"  , MRF_CFLG_INTR                   , sizeof(MRF_PKT_UINT8)      , sizeof(MRF_PKT_BUFF_STATE) ,  NULL               , mrf_task_buff_state }  ,
  [ mrf_cmd_read_sensor ] = {"READ_SENSOR", 0                               , 0                          , 0                          ,  NULL          , mrf_task_read_sensor   },
  [ mrf_cmd_test_1      ] = {"TEST_1"     , 0                               , 0                          , sizeof(MRF_PKT_TIMEDATE)   ,  NULL          , mrf_task_test_1   },
  [ mrf_cmd_test_2      ] = {"TEST_2"     , 0                               , 0                          , sizeof(MRF_PKT_DBG_CHR32)  ,  NULL          , mrf_task_test_2   },
  [ mrf_cmd_usr_resp    ] = {"USR_RESP"   ,                 MRF_CFLG_NO_RESP, 0                          , 0                          ,  NULL               , mrf_task_usr_resp     }
};

const uint16 mrf_num_cmds = (uint16)MRF_NUM_SYS_CMDS;  // FIXME -better to have user commands separate from sys
