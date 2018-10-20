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
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>


//mrfbus
#include <mrf_debug.h>

#include <mrf_arch.h>
#include <mrf_buff.h>
#include <mrf_sys.h>
#include <mrf_if.h>
#include <mrf_route.h>
#include <mrf_arch.h>

//extern uint8 _mrfid;

#define DEFSTR(s) STR(s)
#define STR(s) #s


#ifndef LISTEN_ON
#define LISTEN_ON  8915
#endif


//FIXME debug tmp
//extern const MRF_CMD  *mrf_sys_cmds;

long long mrf_timestamp(){
struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}


#define handle_error(msg)  \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

uint8 _nibble2hex(uint8 nib){
  nib = nib%16;
  if(( 0 <= nib ) && ( nib < 10))
    return '0' + nib;
  else
    return 'A' + (nib - 10);
}

#define _MAX_PRINTLEN 512
void _mrf_print_hex_buff(uint8 *buff,uint16 len){

  uint8 s[_MAX_PRINTLEN + 2];
  uint8 i;
  mrf_debug("print_hex_buff : len is %u buff:\n",len);
  //if (len >  _MRF_BUFFLEN){
  if (len > _MAX_PRINTLEN ){
    mrf_debug("try len <= %u - you had %d\n",_MRF_BUFFLEN,len);
    len = _MAX_PRINTLEN;
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

// posix signal handler - not be confused with mrf signal which is handled by app
void sig_handler(int signum)
{
  printf("Received signal %d\n", signum);
  if (signum == 9){
    exit(0);
  }
}

/*
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
*/



int make_listener_socket (uint16_t port)
{
  int sock;
  int opt = 1;
  struct sockaddr_in name;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      mrf_debug("%s","sock creation error");
      return -1;
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  name.sin_addr.s_addr = inet_addr("127.0.0.1");

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      mrf_debug("bind error for port %u",port);
      return -1;
    }

  return sock;
}


static MRF_APP_CALLBACK app_callback;
static int app_callback_fd;

// slightly desperate for now - need 2 sockets - one for initial connection
//static MRF_APP_CALLBACK app_ccallback;
//static int app_ccallback_fd;




int mrf_arch_app_callback(MRF_APP_CALLBACK callback){
  // allow mrf_app_init to set fd and callback to add to epoll loop
  mrf_debug("mrf_arch_app_callback mrfid %d\n",MRFID);
  app_callback    = callback;
  return 0;

}

//static int num_fds = NUM_INTERFACES+2;
static  int efd, lisfd , servfd;
static  struct epoll_event ievent[NUM_INTERFACES + 5];


int mrf_arch_servfd(){
  return servfd;
}

void kill_signal (int sig)
{
  mrf_debug("%s","kill signal received\n");
  if (lisfd != -1){
    mrf_debug("closing lisfd %d\n",lisfd);
    close(lisfd);
  }

  if (servfd != -1){
    mrf_debug("closing servfd %d\n",servfd);
    close(servfd);
  }
  mrf_debug("%s","ttfn\n");
  exit(0);

}

static int _app_tick_seconds;
//static MRF_APP_CALLBACK _app_tick_callback;
static int _app_timerfd;
static int  _sys_running, _app_tick_enabled;


int mrf_app_tick_enable(int secs){
  struct itimerspec new_value;

  if (_sys_running){
    _app_timerfd = timerfd_create(CLOCK_MONOTONIC,0);
    new_value.it_value.tv_sec = _app_tick_seconds;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = _app_tick_seconds;
    new_value.it_interval.tv_nsec = 0;
    if (timerfd_settime(_app_timerfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
      return -1;
    ievent[NUM_INTERFACES+4].data.u32 = NUM_INTERFACES+4;   // 3 is used when server connection is made
    ievent[NUM_INTERFACES+4].events = EPOLLIN |EPOLLET;

    epoll_ctl(efd, EPOLL_CTL_ADD,_app_timerfd , &ievent[NUM_INTERFACES+4]);

  }
  _app_tick_seconds = secs;
  _app_tick_enabled = 1;

}
int mrf_app_tick_disable(){
  if(_app_tick_enabled){
    epoll_ctl(efd, EPOLL_CTL_DEL,_app_timerfd , &ievent[NUM_INTERFACES+4]);
  }
  _app_tick_enabled = 0;

}

int mrf_arch_boot(){
  // would like to handle sig USR1 at least for clean shutdown of sockets
  struct sigaction usr_action;
  sigset_t block_mask;

  /* Establish the signal handler. */
  sigfillset (&block_mask);
  usr_action.sa_handler = kill_signal;
  usr_action.sa_mask = block_mask;
  usr_action.sa_flags = 0;
  sigaction (SIGUSR1, &usr_action, NULL);

  mrf_debug("%s","lnx arch boot - installed SIGUSR1 handler\n");
  // allow mrf_app_init to set  callback for established server connections
  app_callback = NULL;
  _app_timerfd = -1;
  // _app_tick_callback = NULL;
  _app_tick_seconds = 0;
  _app_tick_enabled = 0;
  _sys_running = 0;
  lisfd  = -1;
  servfd = -1;

  return 0;

}
int mrf_arch_run(){

  // if app_callback is set we establish a simple server, to wait for connections from python land server
  if ( app_callback != NULL ){
    mrf_debug("%s","creating listening socket\n");
    lisfd = make_listener_socket(LISTEN_ON);

    if (listen (lisfd, 1) < 0)
      {
        mrf_debug("%s","failed to listen socket\n");
        return -1;
      }

    mrf_debug("opened listener socket fd = %d\n\n", lisfd);
    if (lisfd < 1 ) {

      mrf_debug("failed to open socket fd = %d\n\n",lisfd);

      return -1;
    }

  }


  //  _print_mrf_cmd(mrf_cmd_device_info);
  //  printf("mrf_arch_run entry: mrf_sys_cmds = %p mrf_sys_cmds[3] = %p 3.str = %p\n",mrf_sys_cmds,&(mrf_sys_cmds[3]),mrf_sys_cmds[3].str);
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

  uint8 *end  = buff + strlen((char *)buff) - 1;
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

  struct epoll_event revent[NUM_INTERFACES + 5];
  int nfds;
  uint32 inif;
  char sname[64];
  printf("Initialising DEVTYPE %s MRFID %d NUM_INTERFACES %d \n", DEFSTR(DEVTYPE),MRFID,NUM_INTERFACES);

  // this is system tick timer - 1KHz
  new_value.it_value.tv_sec = 0;
  new_value.it_value.tv_nsec = 1000000;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 1000000;

  timerfd = timerfd_create(CLOCK_MONOTONIC,0);
  if (timerfd == -1)
    handle_error("timerfd_create");
  //if (clock_gettime(CLOCK_REALTIME, &now) == -1)
  //  handle_error("clock_gettime");
  if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
    handle_error("timerfd_settime");
  print_elapsed_time();
  printf("timer started - fd = %d\n",timerfd);
  printf("opened pipe i = %d  fd = %d\n",NUM_INTERFACES,timerfd);

  // create app timer if cback set
  if ( (_app_tick_seconds > 0)) {
    new_value.it_value.tv_sec = _app_tick_seconds;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = _app_tick_seconds;
    new_value.it_interval.tv_nsec = 0;

    _app_timerfd = timerfd_create(CLOCK_MONOTONIC,0);
    if (_app_timerfd == -1)
      handle_error("_app_timerfd_create");
    //if (clock_gettime(CLOCK_REALTIME, &now) == -1)
    //  handle_error("clock_gettime");
    if (timerfd_settime(_app_timerfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
      handle_error("_app_timerfd_settime");
    print_elapsed_time();
    printf("apptimer started - fd = %d\n",_app_timerfd);

  }

  i = NUM_INTERFACES + 1;
  sprintf(sname,"%s%d-internal",SOCKET_DIR,MRFID);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  mrf_debug("created pipe %s res %d",sname,tmp);
  intfd = open(sname,O_RDONLY | O_NONBLOCK);
  mrf_debug("opened pipe i = %d  %s fd = %d\n",i,sname,intfd);


  mrf_debug("mrf_arch_main_loop:entry NUM_INTERFACES %d\n",NUM_INTERFACES);

  int count = 0;

  efd = epoll_create(2);

  struct epoll_event fevent,tevent;


  // devices must have been initialised - we're getting fds from _sys_if
  // lnx arch drivers must set an fd for input stream
  // add i_f events + cntrl if
  const MRF_IF *ifp;
  for ( i = 0 ; i <  NUM_INTERFACES ; i++){
    ifp = mrf_if_ptr((I_F)i);
    ievent[i].data.u32 = i;
    ievent[i].events = EPOLLIN | EPOLLET;

    epoll_ctl(efd, EPOLL_CTL_ADD, *(ifp->fd), &ievent[i]);
    mrf_debug("I_F event added i_f %d fd %d infd %d\n",i,ievent[i].data.fd,*(ifp->fd));
  }
  //_print_mrf_cmd(mrf_cmd_device_info);
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
  mrf_debug("Internal cntrl added %d u32 %u infd %d\n",NUM_INTERFACES+1,ievent[NUM_INTERFACES+1].data.u32,intfd);


  // add server listener if initialised by appl
  //num_fds = NUM_INTERFACES+2;
  if (( app_callback != NULL ) && (lisfd > 0)){
    mrf_debug("adding epoll for app fifo %d\n",lisfd);
    ievent[NUM_INTERFACES+2].data.u32 = NUM_INTERFACES+2;
    ievent[NUM_INTERFACES+2].events = EPOLLIN | EPOLLET;
    epoll_ctl(efd, EPOLL_CTL_ADD, lisfd , &ievent[NUM_INTERFACES+2]);
    mrf_debug("Application fifo added %d u32 %u applfd %d\n",NUM_INTERFACES+2,ievent[NUM_INTERFACES+2].data.u32,lisfd);
  }

  // add
  if (_app_timerfd != -1) {

    mrf_debug("adding epoll for _app_timerfd %d\n",_app_timerfd);
    ievent[NUM_INTERFACES+4].data.u32 = NUM_INTERFACES+4;   // 3 is used when server connection is made
    ievent[NUM_INTERFACES+4].events = EPOLLIN | EPOLLET;

    epoll_ctl(efd, EPOLL_CTL_ADD,_app_timerfd , &ievent[NUM_INTERFACES+4]);
  }

  _sys_running = 1;
  while(1){

   nfds = epoll_wait(efd, revent, NUM_INTERFACES+5 , -1);
   //s = read(fd, &exp, sizeof(uint64_t));
   //mrf_debug("nfds = %d\n",nfds);

   for ( i = 0 ; i < nfds ; i++){  // process epoll events
     //printf("epoll event %d - u32 = %d NUM_INTERFACES %d\n",i,revent[i].data.u32,NUM_INTERFACES);
     inif = revent[i].data.u32; // data contains mrfbus i_f num
     //mrf_debug("fd %i is i_f %d\n",i,inif);
     if (inif < NUM_INTERFACES){
       //it's an input stream
       fd = *(mrf_if_ptr((I_F)inif)->fd);
       //sanity check
       mrf_debug("event on fd %d\n",fd);
       s = read(fd, buff, 1024); // FIXME need to handle multiple packets
       buff[s] = 0;

       mrf_debug("read %d bytes\n",(int)s);
       //mrf_debug("%s\n",buff);
       //_mrf_print_hex_buff(buff,s);
       uint8 bind;
       //bind = mrf_alloc_if(inif);

       if (*(mrf_if_ptr((I_F)inif)->type->funcs.buff) != NULL){
         //use interface method to copy to buff
         int rv = (*(mrf_if_ptr((I_F)inif)->type->funcs.buff))((I_F)inif,(uint8 *)buff,s);
         //mrf_debug(" arch lnx i_f %d buff function returned %d\n",inif,rv);
       } else {
         // probably mistake here to not have buff function defined..
         // but assume buff contains mrf packet that needs straight copy
         mrf_debug("%s","ERROR ERROR ERROR : lnx - no input/buff function\n\n");
         if ((s <  sizeof(MRF_PKT_HDR)) || (s > _MRF_BUFFLEN)){
           mrf_debug("i_f %d had not buff function defined but buff len was %d",inif,(int)s);
             _mrf_buff_free(bind);
         } else
           mrf_buff_loaded(bind);

       }
     }

     else if (revent[i].data.u32 == NUM_INTERFACES){
       // sys timer tick  1kHz
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
       char token[64];  // we must split tokens
       int bi, j = 0;
       s = read(intfd, buff, 1024);
       mrf_debug("internal control (1) : s = %d buff = %s\n",(int)s,buff);

       for (bi = 0 ; bi < s ; bi++){
         token[j] = buff[bi];
         j++;
         if (buff[bi] == 0) {
           j = 0;
           mrf_debug("token %s\n",token);
           if(strcmp(token,TICK_ENABLE) == 0){
             mrf_debug("%s","INTERNAL:tick_enable\n");
             epoll_ctl(efd, EPOLL_CTL_ADD,timerfd , &ievent[NUM_INTERFACES]);
             //printf("TIMER event added %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,_input_fd[NUM_INTERFACES]);
           }
           else if (strcmp(token,TICK_DISABLE) == 0){
             mrf_debug("%s","INTERNAL:tick_disable\n");
             epoll_ctl(efd, EPOLL_CTL_DEL,timerfd , &ievent[NUM_INTERFACES]);
             //printf("TIMER event removed %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,_input_fd[NUM_INTERFACES]);
           }
           else if (strcmp(token,"wake") == 0){
             mrf_debug("%s","INTERNAL:wakeup\n");
             int i = mrf_foreground();
             mrf_debug("ran %d foreground tasks\n",i);
           }
           else{

             mrf_debug("internal control unrecognised string %s\n",buff);
           }


         }
       }
     } else if (revent[i].data.u32 == NUM_INTERFACES+2){
       // connection to server

       struct sockaddr_in clientname;
       size_t size;
       mrf_debug("server connection fd %d\n",lisfd);

       int newcon;
       size = sizeof (clientname);
       newcon = accept (lisfd,
                     (struct sockaddr *) &clientname,
                     (socklen_t*)&size);
       if (newcon < 0)
         {
           mrf_debug("%s","big trouble accepting connection");
         }
       else {
         mrf_debug ("Server: connect from host %s, port %u \n",
                    inet_ntoa (clientname.sin_addr),
                    ntohs (clientname.sin_port));
         if ( servfd == -1) {
           servfd = newcon;
           int optval = 1;
           setsockopt(servfd, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(int));
           if ( app_callback != NULL ){
             //num_fds = NUM_INTERFACES+4;
             mrf_debug("adding epoll for app c fifo %d\n",servfd);
             ievent[NUM_INTERFACES+3].data.u32 = NUM_INTERFACES+3;
             ievent[NUM_INTERFACES+3].events = EPOLLIN | EPOLLET;
             epoll_ctl(efd, EPOLL_CTL_ADD, servfd , &ievent[NUM_INTERFACES+3]);
             mrf_debug("Server connection EPOLL  added %d u32 %u applfd %d\n",NUM_INTERFACES+3,ievent[NUM_INTERFACES+3].data.u32,servfd);
           }

         }
         else {

           mrf_debug("already have a connection ... closing %d",newcon);
           close(newcon);


         }




       }
     } else if (revent[i].data.u32 == NUM_INTERFACES+3){
       // app c fifo

       printf("server socket  event\n");
       if (app_callback != NULL ){
         if ((*(app_callback))(servfd) == -1){
           mrf_debug("%s","looks like servers disconnected\n");
           servfd = -1;

         }
       }
     }

     else if (revent[i].data.u32 == NUM_INTERFACES+4){
       printf("app timer  event\n");
       s = read(_app_timerfd, buff, 1024);
       buff[s] = 0;

       mrf_app_signal(APP_SIG_TICK);


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
  td->mon = tm.tm_mon + 1;  // matches cc RTC output months 1 - 12
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
  sprintf(sname,"%s%d-internal",SOCKET_DIR,MRFID);
  fd = open(sname, O_WRONLY);
  if(fd == -1){
    mrf_debug("write internal pipe error, fd was %d  name %s\n",fd,sname);
    perror("_write_internal_pipe ERROR sock open\n");
    return -1;
  }
  bc = write(fd, data,len);
  close(fd);
  return bc;
}



int mrf_tick_enable(){
  mrf_debug("%s","mrf_tick_enable : sending tick_enable signal\n");
  int bc = _write_internal_pipe(TICK_ENABLE, sizeof(TICK_ENABLE) );
  return bc;
}

int mrf_tick_disable(){
  mrf_debug("%s","mrf_tick_disable : sending tick_disable signal\n");
  return  _write_internal_pipe(TICK_DISABLE, sizeof(TICK_DISABLE) );
}

int mrf_wake(){
  mrf_debug("%s","mrf_wake..writing internal pipe\n");
  return _write_internal_pipe("wake", sizeof("wake"));

}
int mrf_sleep(){
  return 0;
}
void mrf_reset(){

}
