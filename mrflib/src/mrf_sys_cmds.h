#ifndef __MRF_SYS_CMDS_INCLUDED__
#define __MRF_SYS_CMDS_INCLUDED__

typedef enum mrf_cmd_code
 {
   mrf_cmd_ack = 0,
   mrf_cmd_retry = 1,
   mrf_cmd_resp = 2,
   mrf_cmd_device_info = 3,
   mrf_cmd_if_info = 4,
   mrf_cmd_if_stats = 5,
   mrf_cmd_get_time = 6,
   mrf_cmd_set_time = 7,
   mrf_cmd_sensor_data = 8,
   mrf_cmd_read_sensor = 9,
   mrf_cmd_test_1 = 10, 
   mrf_cmd_test_2 = 11
 } MRF_CMD_CODE;


#define MRF_NUM_SYS_CMDS 12


#endif
