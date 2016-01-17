#ifndef __MRF_SYS_CMDS_INCLUDED__
#define __MRF_SYS_CMDS_INCLUDED__

typedef enum mrf_cmd_code
 {
   mrf_cmd_ack = 0,
   mrf_cmd_retry = 1,
   mrf_cmd_resp = 2,
   mrf_cmd_device_info = 3,
   mrf_cmd_device_status = 4,
   mrf_cmd_sys_info = 5,
   mrf_cmd_if_stats = 6,
   mrf_cmd_get_time = 7,
   mrf_cmd_set_time = 8,
   mrf_cmd_buff_state = 9,
   mrf_cmd_read_sensor = 10,
   mrf_cmd_test_1 = 11, 
   mrf_cmd_test_2 = 12,
   mrf_cmd_usr_resp = 13,
   MRF_NUM_SYS_CMDS = 14
 } MRF_CMD_CODE;




#endif
