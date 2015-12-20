#include "mrf.h"
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <mrf_arch.h>
#include <mrf_buff.h>
#include <mrf_sys.h>
#include <mrf_if.h>
#include <mrf_route.h>
#include <mrf_debug.h>
#define SOCKET_DIR "/tmp/mrf_bus/"

extern uint8 _mrfid;

#define DEFSTR(s) STR(s)
#define STR(s) #s





static void print_elapsed_time(void)
{
  struct timespec start;
  struct timespec curr;
  int first_call = 1;
  int secs, nsecs;

  if (first_call) {
    first_call = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
      handle_error("clock_gettime");
  }

  if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1)
    handle_error("clock_gettime");

  secs = curr.tv_sec - start.tv_sec;
  nsecs = curr.tv_nsec - start.tv_nsec;
  if (nsecs < 0) {
    secs--;
    nsecs += 1000000000;
  }
  printf("%d.%03d: ", secs, (nsecs + 500000) / 1000000);
}

static pthread_t _sys_loop_pthread;

static void *_sys_loop(void *arg);

int mrf_arch_init(){
  
  // run io event loop in pthread

 if(pthread_create(&_sys_loop_pthread, NULL, _sys_loop, NULL)) {

   printf("Error creating thread\n");
   return -1;

} 
 return 0;
}

int is_hex_digit(uint8 dig){
  if (( dig >= '0') && ( dig <= '9'))
    return 1;
  else if(( dig >= 'A') && ( dig <= 'F'))
    return 1;
  else
    return 0;
}

uint8 int_to_hex_digit(int in){
  
  in = in %16;
  
  if ( in < 10) 
    return '0' + in;
  else
    return in - 10 + 'A';

}



void trim_trailing_space(uint8 *buff){

  uint8 *end  = buff + strlen(buff) - 1;
  while(end > buff && isspace(*end)) end--;
  // Write new null terminator
  *(end+1) = 0;
}


char buff[2048];

#define TICK_DISABLE "tick_disable"
#define TICK_ENABLE "tick_enable"

 void *_sys_loop(void *arg){
  
  struct itimerspec new_value;
  struct timespec now;
  uint64_t exp, tot_exp; 
  int timerfd,intfd,fd , i, tmp;
  ssize_t s;
  struct epoll_event revent[NUM_INTERFACES + 2];
  int nfds;
  uint32 inif;
  char sname[64];
  printf("Initialising DEVTYPE %s _mrfid %d NUM_INTERFACES %d \n", DEFSTR(DEVTYPE),_mrfid,NUM_INTERFACES);

  new_value.it_value.tv_sec = 0;
  new_value.it_value.tv_nsec = 1000000;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 1000000;

  timerfd = timerfd_create(CLOCK_MONOTONIC,0);
  if (timerfd == -1)
    handle_error("timerfd_create");
  if (clock_gettime(CLOCK_REALTIME, &now) == -1)
    handle_error("clock_gettime");
  if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
    handle_error("timerfd_settime");
  print_elapsed_time();
  printf("timer started - fd = %d\n",timerfd);
  printf("opened pipe i = %d  fd = %d\n",NUM_INTERFACES,timerfd);

  i = NUM_INTERFACES + 1;
  sprintf(sname,"%s%d-internal",SOCKET_DIR,_mrfid);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  printf("created pipe %s res %d",sname,tmp);
  intfd = open(sname,O_RDONLY | O_NONBLOCK);
  printf("opened pipe i = %d  %s fd = %d\n",i,sname,intfd);


  printf("mrf_arch_main_loop:entry\n");

  int count = 0;

  int efd = epoll_create(2);

  struct epoll_event fevent,tevent;

  // input events for each i_f + one for timer tick
  struct epoll_event ievent[NUM_INTERFACES + 2];

  // devices must have been initialised - we're getting fds from _sys_if
  // lnx arch drivers must set an fd for input stream
  // add i_f events + cntrl if
  MRF_IF *ifp;
  for ( i = 0 ; i <  NUM_INTERFACES ; i++){
    ifp = mrf_if_ptr(i);
    ievent[i].data.u32 = i;
    ievent[i].events = EPOLLIN | EPOLLET;

    epoll_ctl(efd, EPOLL_CTL_ADD, *(ifp->fd), &ievent[i]);
    printf("I_F event added %d fd %d infd %d\n",i,ievent[i].data.fd,*(ifp->fd));
  }

  // timer event
  ievent[NUM_INTERFACES].data.u32 = NUM_INTERFACES;
  ievent[NUM_INTERFACES].events = EPOLLIN | EPOLLET;
  // timer event must be enabled by system - do *not* enable here 
  /*
  epoll_ctl(efd, EPOLL_CTL_ADD,timerfd , &ievent[NUM_INTERFACES]);
  printf("TIMER event added %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,timerfd);
  */
  // internal cntrl pipe
  ievent[NUM_INTERFACES+1].data.u32 = NUM_INTERFACES+1;
  ievent[NUM_INTERFACES+1].events = EPOLLIN | EPOLLET;
  epoll_ctl(efd, EPOLL_CTL_ADD, intfd , &ievent[NUM_INTERFACES+1]);
  printf("Internal cntrl added %d u32 %u infd %d\n",NUM_INTERFACES+1,ievent[NUM_INTERFACES+1].data.u32,intfd);



  while(1){
   nfds = epoll_wait(efd, revent,NUM_INTERFACES+2 , -1);
   //s = read(fd, &exp, sizeof(uint64_t));
   printf("nfds = %d\n",nfds);
   
   for ( i = 0 ; i < nfds ; i++){  // process epoll events
     //printf("epoll event %d - u32 = %d NUM_INTERFACES %d\n",i,revent[i].data.u32,NUM_INTERFACES);
     inif = revent[i].data.u32; // data contains mrfbus i_f num
     if (inif < NUM_INTERFACES){
       //it's an input stream
       fd = *(mrf_if_ptr(inif)->fd);
       //sanity check
       printf("event on fd %d",fd);
       s = read(fd, buff, 1024); // FIXME need to handle multiple packets
       buff[s] = 0;

       uint8 bind;
       bind = mrf_alloc_if(inif);

       if (*(mrf_if_ptr(inif)->type->funcs.buff) != NULL){
         //use interface method to copy to buff 
         int rv = (*(mrf_if_ptr(inif)->type->funcs.buff))(inif,buff,s,bind);
         if ( rv != 0) {
           mrf_debug("error arch lnx i_f %d buff function returned %d\n",inif,rv);
           _mrf_buff_free(bind);
         } else {
           mrf_buff_loaded(bind);           
         }                       
       } else {
         // probably mistake here to not have buff function defined..
         // but assume buff contains mrf packet that needs straight copy
         if ((s <  sizeof(MRF_PKT_HDR)) || (s > _MRF_BUFFLEN)){
           mrf_debug("i_f %d had not buff function defined but buff len was %d",inif,(int)s);  
             _mrf_buff_free(bind);
         } else 
           mrf_buff_loaded(bind);           

       }
     }
   
     else if (revent[i].data.u32 == NUM_INTERFACES){
       // timer tick
       s = 1;
       int l = 0;
       // while (s > 0){
       s = read(timerfd, buff, 1024);
       buff[s] = 0;
       l++;
       //printf("TIMER event :count %d l %d read %d bytes  u32 = %u  inif = %d fd = %d \n",count,l,(int)s,revent[i].data.u32,inif,_input_fd[inif]);  

       _mrf_tick();
       count++;
       if((count ) == 1000 ){

         //if((count % 1000 ) == 0 ){
         printf("%d\n",count);
       }

     }

     else if (revent[i].data.u32 == NUM_INTERFACES+1){
       // internal cntrl

 
       s = read(intfd, buff, 1024);
       buff[s] = 0;
       trim_trailing_space(buff);
       s = strlen(buff);


       //printf("\n internal control : s = %d buff = %s\n",(int)s,buff);
       if ( s > 10){

         if(strcmp(buff,TICK_ENABLE) == 0){
           //printf("INTERNAL:tick_enable\n");
           epoll_ctl(efd, EPOLL_CTL_ADD,timerfd , &ievent[NUM_INTERFACES]);
           //printf("TIMER event added %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,_input_fd[NUM_INTERFACES]);           
         }
         else if (strcmp(buff,TICK_DISABLE) == 0){
           //printf("INTERNAL:tick_disable\n");
           epoll_ctl(efd, EPOLL_CTL_DEL,timerfd , &ievent[NUM_INTERFACES]);
           //printf("TIMER event removed %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,_input_fd[NUM_INTERFACES]);
         }
       }   
     }
   }
  }
       
  return NULL;

}



int mrf_rtc_get(TIMEDATE *td){

  time_t t = time(NULL);
  mrf_debug("mrf_rtc_get t %lx\n",t);
  struct tm tm = *localtime(&t);
  mrf_debug("\nyear is %u mon %u day %u hour %u min %u sec %u ",
         tm.tm_year,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
  td->year = tm.tm_year - 100;  // mrf bus years start at 2000
  td->mon = tm.tm_mon;
  td->day = tm.tm_mday;
  td->hour = tm.tm_hour;  
  td->min = tm.tm_min;
  td->sec = tm.tm_sec;
  return 0;
}


int mrf_rtc_set(TIMEDATE *td){

}



int mrf_tick_enable(){
  char sname[64];
  int fd,bc,tb;
  mrf_debug("mrf_tick_enable arch lnx\n");
  sprintf(sname,"%s%d-internal",SOCKET_DIR,_mrfid);
  fd = open(sname, O_WRONLY);
  if(fd == -1){
    printf(" %d\n",fd);
    perror("mrf_tick_enable ERROR sock open\n");
    return -1;
  }
  bc = write(fd, TICK_ENABLE,sizeof(TICK_ENABLE) );

}

int mrf_tick_disable(){
  char sname[64];
  int fd,bc,tb;
  sprintf(sname,"%s%d-internal",SOCKET_DIR,_mrfid);
  fd = open(sname, O_WRONLY);
  if(fd == -1){
    printf(" %d\n",fd);
    perror("mrf_tick_enable ERROR sock open\n");
    return -1;
  }
  bc = write(fd, TICK_DISABLE,sizeof(TICK_DISABLE) );

}

