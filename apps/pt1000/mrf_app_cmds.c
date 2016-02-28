#include "mrf_sys.h"

//#include "mrf_app_cmds.h"

const MRF_CMD mrf_app_cmds[MRF_NUM_APP_CMDS] = {
  [ mrf_app_cmd_test      ] = {"APP_TEST"    , 0                           , 0                          , sizeof(MRF_PKT_TIMEDATE)   ,  NULL          , mrf_app_task_test   },
  [ mrf_app_cmd_spi_read  ] = {"SPI_READ"   , 0                            , sizeof(MRF_PKT_UINT8)      ,sizeof(MRF_PKT_UINT8)       ,  NULL          , mrf_app_spi_read },
  [ mrf_app_cmd_spi_write ] = {"SPI_WRITE"  , 0                            , sizeof(MRF_PKT_UINT8_2)    ,0                           ,  NULL          , mrf_app_spi_write   }

};


const uint8 mrf_num_app_cmds = (uint8)MRF_NUM_APP_CMDS;

const MRF_PKT_APP_INFO app_info        = {"pt1000", MRF_NUM_APP_CMDS};
