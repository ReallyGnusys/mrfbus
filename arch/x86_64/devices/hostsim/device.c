#include "mrf_if.h"
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#define SOCKET_DIR "/tmp/mrf_bus/"


extern uint8 _mrfid;

extern MRF_IF_TYPE usb_if_type;
extern MRF_IF_TYPE lnx_if_type;

extern int _output_fd[];  // ugly
extern int _input_fd[];

int mrf_device_init(){
  int i,tmp;
  char sname[64];
  for ( i = 0 ; i < NUM_INTERFACES ; i++){
    mrf_if_register(i,&lnx_if_type);
    sprintf(sname,"%s%d-%d-in",SOCKET_DIR,_mrfid,i);
    tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
    printf("created pipe %s res %d",sname,tmp);
    _input_fd[i] = open(sname,O_RDONLY | O_NONBLOCK);
    _output_fd[i] = -1;
    printf("opened pipe i = %d  %s fd = %d\n",i,sname,_input_fd[i]);
  }
}
