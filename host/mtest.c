/*
#include <stdlib.h>
#include <stdio.h>
#include <mrf_types.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>        //Definition of uint64_t 
*/
#include <mrf.h>



int main(void){
  char buff[256];
  mrf_init();
  mrf_time(buff);
  printf("MRF_TIME IS %s\n",buff);
  return mrf_main_loop();

}


