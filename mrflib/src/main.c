#include <mrf.h>

const uint8 _mrfid = MRFID;

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  _mrf_buff_free(bnum);
}

int main(void){
  int i;
  mrf_init();
  while(1){
#ifdef MRF_ARCH_lnx
    usleep(1000000);  //FIXME mrf_foreground should handle this using epoll on lnx
#endif
    i = mrf_foreground();
  }
  
}

