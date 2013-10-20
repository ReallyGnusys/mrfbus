#ifndef __MRF_SYS_TASKS_INCL__
#define __MRF_SYS_TASKS_INCL__
#include <mrf_sys.h>

MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum);
MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum);
MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum);
MRF_CMD_RES mrf_task_test_1(MRF_CMD_CODE cmd,uint8 bnum);
MRF_CMD_RES mrf_task_test_2(MRF_CMD_CODE cmd,uint8 bnum);

#endif
