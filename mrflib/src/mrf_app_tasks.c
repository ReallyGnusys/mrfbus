#include "mrf_sys.h"
#include <mrf_debug.h>

MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_app_task_test entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get(rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug("mrf_app_task_test exit\n");
  return MRF_CMD_RES_OK;
}
