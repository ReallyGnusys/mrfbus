/******************************************************************************
*
* Copyright (c) 2012-16 Gnusys Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************************/

#include "mrf.h"
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <stdlib.h>
//mrfbus
#include <mrf_arch.h>
#include <mrf_buff.h>
#include <mrf_sys.h>
#include <mrf_if.h>
#include <mrf_route.h>
#include <mrf_debug.h>
#include <mrf_arch.h>

extern uint8 _mrfid;

#define DEFSTR(s) STR(s)
#define STR(s) #s

//debug tmp
extern const MRF_CMD const *mrf_sys_cmds;

#define handle_error(msg)  \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

uint8 _nibble2hex(uint8 nib){
  nib = nib%16;
  if(( 0 <= nib ) && ( nib < 10))
    return '0' + nib;
  else
    return 'A' + (nib - 10);
}

void _mrf_print_hex_buff(uint8 *buff,uint16 len){
 
  uint8 s[_MRF_BUFFLEN*2 + 2];
  uint8 i;
  mrf_debug("print_hex_buff : len is %u buff:",len);
  if (len >  _MRF_BUFFLEN){
    mrf_debug("try len <= %u - you had %d\n",_MRF_BUFFLEN,len);
    return;
  }
  for ( i=0; i<len ; i++){
    s[i*2] = _nibble2hex(buff[i]/16);
    s[i*2+1] = _nibble2hex(buff[i]%16);
  }
  s[i*2] = '\0';
 
  //s[0] = '\0';
  mrf_debug("%s\n",s);

}



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

//static pthread_t _sys_loop_pthread;

static void *_sys_loop(void *arg);

void sig_handler(int signum)
{
  printf("Received signal %d\n", signum);
  if (signum == 9){
    exit(0);
  }
}

int _print_mrf_cmd(MRF_CMD_CODE cmd){
  mrf_debug("_print_mrf_cmd entry cmd %d\n",cmd);
  mrf_debug("mrf_sys_cmds %p  &mrf_sys_cmds[0] %p \n",mrf_sys_cmds,&mrf_sys_cmds[0]);
  if (cmd >= MRF_NUM_SYS_CMDS){
    mrf_debug("cmd code too big %d\n",cmd);
    return -1;
  }
  mrf_debug("cmd %d is at %p\n",cmd, &mrf_sys_cmds[cmd]);
  mrf_debug("cmd desc is at %p\n",mrf_sys_cmds[cmd].str);
}


static MRF_APP_CALLBACK app_callback;

static int app_callback_fd;


int mrf_arch_app_callback(int fd, MRF_APP_CALLBACK callback){
  // allow mrf_app_init to set fd and callback to add to epoll loop
  app_callback_fd = fd;
  app_callback    = callback;

  return 0;

}
int mrf_arch_boot(){
  // allow mrf_app_init to set fd and callback to add to epoll loop
  app_callback = NULL;
  app_callback_fd = -1;
  
  
}
int mrf_arch_run(){
  
  _print_mrf_cmd(mrf_cmd_device_info);
  printf("mrf_arch_run entry: mrf_sys_cmds = %p mrf_sys_cmds[3] = %p 3.str = %p\n",mrf_sys_cmds,&(mrf_sys_cmds[3]),mrf_sys_cmds[3].str);
  (*_sys_loop)(NULL);

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
  // input events for each i_f + one for timer tick plus optional app fifo
  struct epoll_event ievent[NUM_INTERFACES + 3];

  struct epoll_event revent[NUM_INTERFACES + 3];
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


  // devices must have been initialised - we're getting fds from _sys_if
  // lnx arch drivers must set an fd for input stream
  // add i_f events + cntrl if
  MRF_IF *ifp;
  for ( i = 0 ; i <  NUM_INTERFACES ; i++){
    ifp = mrf_if_ptr(i);
    ievent[i].data.u32 = i;
    ievent[i].events = EPOLLIN | EPOLLET;

    epoll_ctl(efd, EPOLL_CTL_ADD, *(ifp->fd), &ievent[i]);
    printf("I_F event added i_f %d fd %d infd %d\n",i,ievent[i].data.fd,*(ifp->fd));
  }
 _print_mrf_cmd(mrf_cmd_device_info);
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


  // add appl pipe if initialised by appl
  int app_event = 0;
  int num_fds = NUM_INTERFACES+2;
  if ( app_callback != NULL ){
    num_fds = NUM_INTERFACES+3;
    app_event = 1;
    mrf_debug("adding epoll for app fifo %d\n",app_callback_fd); 
    ievent[NUM_INTERFACES+2].data.u32 = NUM_INTERFACES+2;
    ievent[NUM_INTERFACES+2].events = EPOLLIN | EPOLLET;
    epoll_ctl(efd, EPOLL_CTL_ADD, app_callback_fd , &ievent[NUM_INTERFACES+2]);
    printf("Application fifo added %d u32 %u applfd %d\n",NUM_INTERFACES+2,ievent[NUM_INTERFACES+2].data.u32,app_callback_fd);
  }
  

  while(1){
   nfds = epoll_wait(efd, revent,num_fds , -1);
   //s = read(fd, &exp, sizeof(uint64_t));
   printf("nfds = %d\n",nfds);
   
   for ( i = 0 ; i < nfds ; i++){  // process epoll events
     //printf("epoll event %d - u32 = %d NUM_INTERFACES %d\n",i,revent[i].data.u32,NUM_INTERFACES);
     inif = revent[i].data.u32; // data contains mrfbus i_f num
     mrf_debug("fd %i is i_f %d\n",i,inif);
     if (inif < NUM_INTERFACES){
       //it's an input stream
       fd = *(mrf_if_ptr(inif)->fd);
       //sanity check
       printf("event on fd %d\n",fd);
       s = read(fd, buff, 1024); // FIXME need to handle multiple packets
       buff[s] = 0;

       printf("read %d bytes\n",(int)s);
       _mrf_print_hex_buff(buff,s);
       uint8 bind;
       //bind = mrf_alloc_if(inif);

       if (*(mrf_if_ptr(inif)->type->funcs.buff) != NULL){
         //use interface method to copy to buff 
         int rv = (*(mrf_if_ptr(inif)->type->funcs.buff))(inif,buff,s);
         mrf_debug(" arch lnx i_f %d buff function returned %d\n",inif,rv);                   
       } else {
         // probably mistake here to not have buff function defined..
         // but assume buff contains mrf packet that needs straight copy
         printf("ERROR ERROR ERROR : lnx - no input/buff function\n\n");
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
       else if (strcmp(buff,"wake") == 0){
         printf("got wakeup\n");
         int i = mrf_foreground();
         printf("ran %d foreground tasks\n",i);
       }
       else{
         
         printf("internal control unrecognised string %s\n",buff);
       }
     } else if (revent[i].data.u32 == NUM_INTERFACES+2){
       // app fifo

       printf("app fifo event\n");
       if (app_callback != NULL ){
         (*(app_callback))(app_callback_fd);
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
  // don't set linux for now
  return 0;
}


int _write_internal_pipe(char *data, int len){
  char sname[64];
  int fd,bc,tb;
  sprintf(sname,"%s%d-internal",SOCKET_DIR,_mrfid);
  fd = open(sname, O_WRONLY);
  if(fd == -1){
    printf(" %d\n",fd);
    perror("_write_internal_pipe ERROR sock open\n");
    return -1;
  }
  bc = write(fd, data,len);
  close(fd); 
  return bc;
}



int mrf_tick_enable(){
  int bc = _write_internal_pipe(TICK_ENABLE, sizeof(TICK_ENABLE) );
  return bc;
}

int mrf_tick_disable(){
  return  _write_internal_pipe(TICK_DISABLE, sizeof(TICK_DISABLE) );
}

int mrf_wake(){
  mrf_debug("mrf_wake..writing internal pipe\n");
  return _write_internal_pipe("wake", sizeof("wake"));

}
int mrf_sleep(){
}
