#include "mrf_sys.h"

const MRF_CMD mrf_app_cmds[MRF_NUM_SYS_CMDS] = {
  [ mrf_app_cmd_test      ] = {"APP_TEST"     , 0                               , 0                          , sizeof(MRF_PKT_TIMEDATE)   ,  NULL          , mrf_app_task_test   }
};

const uint8 mrf_num_app_cmds = (uint8)MRF_NUM_APP_CMDS;  // FIXME -better to have user commands separate from sys

const MRF_PKT_APP_INFO app_info        = {"default_app", MRF_NUM_APP_CMDS};
