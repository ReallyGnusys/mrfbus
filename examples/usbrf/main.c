
#include <mrf.h>

const uint8 _mrfid = MRFID;


MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  _mrf_buff_free(bnum);
}


int main(void){
  char buff[64];
  int i;
  mrf_init();
  mrf_time(buff);

  while(1){
    //usleep(1000000);
    i = mrf_foreground();
  }
  return 0;
}


