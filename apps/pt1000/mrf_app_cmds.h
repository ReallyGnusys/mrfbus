#ifndef __MRF_APP_CMDS_INCLUDED__
#define __MRF_APP_CMDS_INCLUDED__

typedef enum mrf_app_cmd_code
 {
   mrf_app_cmd_test = 0,
   mrf_app_cmd_spi_read = 1,
   mrf_app_cmd_spi_write = 2,
   MRF_NUM_APP_CMDS = 3
 } MRF_APP_CMD_CODE;

#endif
