#include <mrf.h>

const uint8 _mrfid = MRFID;

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  _mrf_buff_free(bnum);
}

int main(void){
  char buff[256];
  int i;
  mrf_init();
  mrf_time(buff);
  //printf("MRF_TIME IS %s\n",buff);
  //return mrf_main_loop();
  while(1){
    usleep(1000000);
    i = mrf_foreground();
  }
  
}


