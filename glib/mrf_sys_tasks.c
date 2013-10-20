#include <mrf_sys.h>
#include <stdio.h>
MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum){

}

MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum){
  printf("mrf_task_resp\n");
}
MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum){

}
MRF_CMD_RES mrf_task_test_1(MRF_CMD_CODE cmd,uint8 bnum){

  printf("mrf_task_test_1\n");
}
MRF_CMD_RES mrf_task_test_2(MRF_CMD_CODE cmd,uint8 bnum){
  printf("mrf_task_test_2\n");

}
