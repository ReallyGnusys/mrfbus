#ifndef __MRF_SYS_TASKS_INCL__
#define __MRF_SYS_TASKS_INCL__
#include <mrf_sys.h>

MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_retry(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_device_status(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_if_status(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_set_time(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_sensor_data(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_read_sensor(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_test_1(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_task_test_2(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);

#endif
