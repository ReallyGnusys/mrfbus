/******************************************************************************
*
* Copyright (c) 2012-16 Gnusys Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************************/

#include "mrf_sys.h"

extern const MRF_PKT_DEVICE_INFO device_info;
extern const MRF_PKT_SYS_INFO    sys_info;
extern const MRF_PKT_APP_INFO app_info;

const MRF_CMD mrf_sys_cmds[MRF_NUM_SYS_CMDS] = {
  [ mrf_cmd_ack          ] = {"ACK"        , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , 0                          , 0                          ,  NULL               , mrf_task_ack      },
  [ mrf_cmd_retry        ] = {"RETRY"      , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , 0                          , 0                          ,  NULL               , mrf_task_retry      },
  [ mrf_cmd_resp         ] = {"RESP"       , MRF_CFLG_INTR | MRF_CFLG_NO_ACK , sizeof(MRF_PKT_RESP)       , 0                          ,  NULL               , mrf_task_resp     },
  [ mrf_cmd_device_info  ] = {"DEVICE_INFO", MRF_CFLG_INTR                   , 0                          , sizeof(MRF_PKT_DEVICE_INFO),  (void*)&device_info, NULL },
   [ mrf_cmd_device_status] = {"DEVICE_STATUS"    , MRF_CFLG_INTR            , 0                          , sizeof(MRF_PKT_DEVICE_STATUS)    ,  NULL               , mrf_task_device_status },
  [ mrf_cmd_sys_info  ]    = {"SYS_INFO"   , MRF_CFLG_INTR                   , 0                          , sizeof(MRF_PKT_SYS_INFO),  (void*)&sys_info, NULL },
  [ mrf_cmd_if_stats    ] = {"IF_STATUS"  , MRF_CFLG_INTR                    , sizeof(MRF_PKT_UINT8)     , sizeof(IF_STATS      )    ,  NULL               , mrf_task_if_status  },
  [ mrf_cmd_get_time    ] = {"GET_TIME"   , MRF_CFLG_INTR                    , 0                          , sizeof(MRF_PKT_TIMEDATE)   ,  NULL               , mrf_task_get_time },
  [ mrf_cmd_set_time    ] = {"SET_TIME"   , MRF_CFLG_INTR                    , sizeof(MRF_PKT_TIMEDATE)   , sizeof(MRF_PKT_TIMEDATE)   ,  NULL               , mrf_task_set_time   },
  [ mrf_cmd_buff_state ] = {"BUFF_STATE"  , MRF_CFLG_INTR                    , sizeof(MRF_PKT_UINT8)      , sizeof(MRF_PKT_BUFF_STATE) ,  NULL               , mrf_task_buff_state }  ,
  [ mrf_cmd_cmd_info ] = {"CMD_INFO",  MRF_CFLG_INTR                         , sizeof(MRF_PKT_UINT8)      , sizeof(MRF_PKT_CMD_INFO),  NULL          , mrf_task_cmd_info   },
  [ mrf_cmd_app_info ] = {"APP_INFO",  MRF_CFLG_INTR                         , 0                          , sizeof(MRF_PKT_APP_INFO),   (void*)&app_info, NULL},
  [ mrf_cmd_app_cmd_info ] = {"APP_CMD_INFO",  MRF_CFLG_INTR                 , sizeof(MRF_PKT_UINT8)      , sizeof(MRF_PKT_CMD_INFO),  NULL          , mrf_task_app_cmd_info   },
  [ mrf_cmd_test_1      ] = {"TEST_1"     , 0                                , 0                          , sizeof(MRF_PKT_TIMEDATE)   ,  NULL          , mrf_task_test_1   },
  [ mrf_cmd_usr_struct  ] = {"USR_STRUCT" , 0                                , sizeof(MRF_PKT_RESP)       , 0                          ,  NULL      , mrf_task_usr_struct  },
  [ mrf_cmd_usr_resp    ] = {"USR_RESP"   , 0                                , sizeof(MRF_PKT_RESP)       , 0                          ,  NULL      , mrf_task_usr_resp     },
  [ mrf_cmd_reset       ] = {"RESET"       ,  MRF_CFLG_NO_ACK                , 0                          , 0                          ,  NULL      , mrf_task_reset      },
  [ mrf_cmd_ping        ] = {"PING"        ,  MRF_CFLG_INTR                  , 0                          , sizeof(MRF_PKT_PING_RES)   ,  NULL     , mrf_task_ping},
  [ mrf_cmd_ndr         ] = {"NDR"      , MRF_CFLG_INTR                      , sizeof(MRF_PKT_NDR)        , 0                          ,  NULL     , mrf_task_ndr      },


};

const uint16 mrf_num_cmds = (uint16)MRF_NUM_SYS_CMDS;  // FIXME -better to have user commands separate from sys
