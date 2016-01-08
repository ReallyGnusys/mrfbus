
#include <mrf.h>

const uint8 _mrfid = MRFID;

static uint8 _tabuff[10];

int _test_ack(){
  _tabuff[0] = 8;
  _tabuff[1] = 1;
  _tabuff[2] = MRFNET;
  _tabuff[3] = 1;
  _tabuff[4] = mrf_cmd_ack;
  _tabuff[5] = MRFID;
  _tabuff[6] = MRFID;
  _tabuff[7] = 1;
  MRF_IF *i_f =  mrf_if_ptr(UART0);
  (*(i_f->type->funcs.send))(UART0,_tabuff);
  return 0;
}
int main(void){
  char buff[64];
  int i;
  mrf_init();
  mrf_time(buff);
  //  printf("MRF_TIME IS %s\n",buff);
  // _test_ack();

  while(1){
    //usleep(1000000);
    i = mrf_foreground();
  }
  return 0;
}


