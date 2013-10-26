#include "mrf.h"
#include <time.h>
#include <string.h>

#include <mrf_arch.h>
#include <mrf_buff.h>
#include <mrf_sys.h>
#include <mrf_if.h>
#include <mrf_route.h>
#include <mrf_debug.h>
#define SOCKET_DIR "/tmp/mrf_bus/"


#define DEFSTR(s) STR(s)
#define STR(s) #s
int lnx_if_send_func(I_F i_f, uint8 *buff);

MRF_IF_TYPE lnx_if_type = {
 tx_del : 1,
 send_func : lnx_if_send_func
};


int _bin2hex(uint* inbuff,uint *outbuff,uint8 *len);
// define interfaces 
//int lnx_if_send_func(I_F i_f, uint8 bnum){


int lnx_if_send_func(I_F i_f, uint8 *buff){
  char spath[64];
  uint8 txbuff[_MRF_BUFFLEN];
  int fd,bc,tb;
  uint8 sknum;
  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)buff;

  // rough hack to get up and down using correct sockets
  if (hdr->hdest >= SNETSZ)  // rf devices only have 1 i_f
    sknum = 0;
  else if (hdr->hdest >= 1)  // usbrf or host
    {
      if(hdr->hdest < MRFID) 
        sknum = 1;
      else
        sknum = 0;
    }
  else  // zl-0
    sknum = 0;



  //printf("lnx_if_send_func i_f %d buff %p  len %d\n",i_f,buff,buff[0]);
  // printf("hdest %d udest %d hsrc %d usrc %d\n",hdr->hdest,hdr->udest,hdr->hsrc,hdr->usrc);
  sprintf(spath,"%s%d-%d-in",SOCKET_DIR,hdr->hdest,sknum);
  printf("using socket *%s*\n",spath);
  fd = open(spath, O_WRONLY | O_NONBLOCK);
  if(fd == -1){
    printf(" %d\n",fd);
    perror("ERROR file open\n");
    return -1;
  }
  tb = copy_to_txbuff(buff,buff[0],txbuff);
  
  bc = write(fd, txbuff,tb );
  //printf("bc = %d  fd = %d\n",bc,fd);

}




#define handle_error(msg)  \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)



// lnx i_f uses sockets 

int _input_fd[NUM_INTERFACES+2];


static void print_elapsed_time(void)
{
  static struct timespec start;
  struct timespec curr;
  static int first_call = 1;
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



int mrf_arch_init(){
  int i,timerfd,tmp;
  struct timespec now;
  struct itimerspec new_value;
  char sname[64];
  printf("Initialising DEVTYPE %s MRFID %d NUM_INTERFACES %d \n", DEFSTR(DEVTYPE),MRFID,NUM_INTERFACES);

  for ( i = 0 ; i < NUM_INTERFACES ; i++){
      mrf_if_register(i,&lnx_if_type);
      sprintf(sname,"%s%d-%d-in",SOCKET_DIR,MRFID,i);
      tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
      printf("created pipe %s res %d",sname,tmp);
      _input_fd[i] = open(sname,O_RDONLY | O_NONBLOCK);
      printf("opened pipe i = %d  %s fd = %d\n",i,sname,_input_fd[i]);

  }

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
  _input_fd[NUM_INTERFACES] = timerfd;
  print_elapsed_time();
  printf("timer started - fd = %d\n",timerfd);
  printf("opened pipe i = %d  fd = %d\n",NUM_INTERFACES,_input_fd[NUM_INTERFACES]);

  i = NUM_INTERFACES + 1;
  sprintf(sname,"%s%d-internal",SOCKET_DIR,MRFID);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  printf("created pipe %s res %d",sname,tmp);
  _input_fd[i] = open(sname,O_RDONLY | O_NONBLOCK);
  printf("opened pipe i = %d  %s fd = %d\n",i,sname,_input_fd[i]);

  
}

int is_hex_digit(uint8 dig){
  if (( dig >= '0') && ( dig <= '9'))
    return 1;
  else if(( dig >= 'A') && ( dig <= 'F'))
    return 1;
  else
    return 0;
}
int hex_digit_to_int(uint8 dig){
  if (( dig >= '0') && ( dig <= '9'))
    return dig - '0';
  else if(( dig >= 'A') && ( dig <= 'F'))
    return 10 + dig - 'A';
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



uint8 *trim_trailing_space(uint8 *buff){

  uint8 *end  = buff + strlen(buff) - 1;
  while(end > buff && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return buff;

}
uint8 hex_dig_str_to_int(uint8 *dig){
  return 16*hex_digit_to_int(*dig) + hex_digit_to_int(*(dig+1));

}
void  copy_to_mbuff(uint8 *buffer,int len,uint8 *mbuff){
  int i;
  for ( i = 0 ; i < len/2 ; i++)
    mbuff[i] = hex_dig_str_to_int(&buffer[i*2]);

}
int  copy_to_txbuff(uint8 *buffer,int len,uint8 *txbuff){
  int i;
  for ( i = 0 ; i < len ; i++){

    txbuff[i*2] = int_to_hex_digit(buffer[i]/16);
    txbuff[i*2 + 1] = int_to_hex_digit(buffer[i]%16);
    

  }
  return i*2;
}

typedef struct  __attribute__ ((packed))   {
  uint8 netid;
  uint8 dest;
  uint8 type;
    
} MRF_CNTRL_PKT;



int packet_received(I_F i_f,char *buffer,int len){
  int i;
  uint8 *mbuff; // mrf_buff
  uint8 cbuff[64];
  // sanity check packet
  //printf("pr 1 : len %d\n",len);
  if ( len > _MRF_BUFFLEN * 2)
    return -1;
 
  // get rid of str terminator
  if (len % 2){
    mrf_debug("ALERT - odd length packet %d\n",len);
    len = len -1;

  }

  for (i = 0 ; i < len ; i ++ )
    {

      if (is_hex_digit(buffer[i] == 0) ){
        printf("dig %d ( %c ) not hex\n",i,buffer[i]); 
        return -1;
      }

    }

  if (len < sizeof(MRF_PKT_HDR) * 2){
    printf("PACKET too short ( %d bytes )\n",len);
  }
 
  mbuff = mrf_alloc_if(i_f);
  //printf("\nPACKET RECIEVED IF = %d: mbuff = %p\n",i_f,mbuff);
  if ( mbuff != NULL) {
    copy_to_mbuff(buffer,len,mbuff);
    //printf("\npkt copied\n");
    mrf_buff_loaded_if(i_f, mbuff,0);
  }
  else {
    printf("mrf_arch : packet_received - failed to allocate buffer!\n");
  }
}
char buff[2048];

#define MAX_EVENTS 8




int mrf_arch_main_loop(){
  
  //uint8 *buff;
  struct itimerspec new_value;
  struct timespec now;
  uint64_t exp, tot_exp; 
  int timerfd,fd , i;
  ssize_t s;
  struct epoll_event revent[NUM_INTERFACES + 2];
  int nfds;
  uint32 inif;

  printf("mrf_arch_main_loop:entry\n");

  int count = 0;

  int efd = epoll_create(2);

  struct epoll_event fevent,tevent;

  // input events for each i_f + one for timer tick
  struct epoll_event ievent[NUM_INTERFACES + 2];

  // add i_f events + cntrl if
  for ( i = 0 ; i <  NUM_INTERFACES ; i++){
    ievent[i].data.u32 = i;
    ievent[i].events = EPOLLIN | EPOLLET;

    epoll_ctl(efd, EPOLL_CTL_ADD, _input_fd[i], &ievent[i]);
    printf("I_F event added %d fd %d infd %d\n",i,ievent[i].data.fd,_input_fd[i]);
  }

  // timer event
  ievent[NUM_INTERFACES].data.u32 = NUM_INTERFACES;
  ievent[NUM_INTERFACES].events = EPOLLIN | EPOLLET;
  /*
  epoll_ctl(efd, EPOLL_CTL_ADD,_input_fd[NUM_INTERFACES] , &ievent[NUM_INTERFACES]);
  printf("TIMER event added %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,_input_fd[NUM_INTERFACES]);
  */
  // internal cntrl pipe
  ievent[NUM_INTERFACES+1].data.u32 = NUM_INTERFACES+1;
  ievent[NUM_INTERFACES+1].events = EPOLLIN | EPOLLET;
  epoll_ctl(efd, EPOLL_CTL_ADD,_input_fd[NUM_INTERFACES+1] , &ievent[NUM_INTERFACES+1]);
  printf("Internal cntrl added %d u32 %u infd %d\n",NUM_INTERFACES+1,ievent[NUM_INTERFACES+1].data.u32,_input_fd[NUM_INTERFACES+1]);



  while(1){
   nfds = epoll_wait(efd, revent,NUM_INTERFACES+2 , -1);
   //s = read(fd, &exp, sizeof(uint64_t));
   //printf("nfds = %d\n",nfds);
   
   for ( i = 0 ; i < nfds ; i++){
     //printf("epoll event %d - u32 = %d NUM_INTERFACES %d\n",i,revent[i].data.u32,NUM_INTERFACES);

     // search input fds for each event
     inif = revent[i].data.u32;

     if (revent[i].data.u32 < NUM_INTERFACES){
       //it's an input stream
       
       //sanity check
     
       s = read(_input_fd[inif], buff, 1024);
       buff[s] = 0;

       trim_trailing_space(buff);
       s = strlen(buff);

       if (s >=  sizeof(MRF_PKT_HDR)*2) { // min cntrl packet size 
         //printf("infd event p_r next: read %d bytes = %s ,  inif = %d fd = %d \n",(int)s,buff,inif,_input_fd[inif]);  
       
         packet_received(inif,buff,s);              
       }

     }
   
     else if (revent[i].data.u32 == NUM_INTERFACES){
       // timer tick

       s = 1;
       int l = 0;
       // while (s > 0){
       s = read(_input_fd[NUM_INTERFACES], buff, 1024);
       buff[s] = 0;
       l++;
         // }
         //printf("TIMER event :count %d l %d read %d bytes  u32 = %u  inif = %d fd = %d \n",count,l,(int)s,revent[i].data.u32,inif,_input_fd[inif]);  

       
         //if((count ) == 1000 ){
       _mrf_tick();
       count++;
       if((count ) == 1000 ){

         //if((count % 1000 ) == 0 ){
         printf("%d\n",count);
       }

     }

     else if (revent[i].data.u32 == NUM_INTERFACES+1){
       // internal cntrl

 
       s = read(_input_fd[NUM_INTERFACES+1], buff, 1024);
       buff[s] = 0;
       trim_trailing_space(buff);
       s = strlen(buff);


       //printf("\n internal control : s = %d buff = %s\n",(int)s,buff);
       if ( s > 10){

         if(strcmp(buff,"tick_enable") == 0){
           //printf("INTERNAL:tick_enable\n");
           epoll_ctl(efd, EPOLL_CTL_ADD,_input_fd[NUM_INTERFACES] , &ievent[NUM_INTERFACES]);
           //printf("TIMER event added %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,_input_fd[NUM_INTERFACES]);


           
         }
         else if (strcmp(buff,"tick_disable") == 0){
           //printf("INTERNAL:tick_disable\n");
           epoll_ctl(efd, EPOLL_CTL_DEL,_input_fd[NUM_INTERFACES] , &ievent[NUM_INTERFACES]);
           //printf("TIMER event removed %d u32 %u infd %d\n",NUM_INTERFACES,ievent[NUM_INTERFACES].data.u32,_input_fd[NUM_INTERFACES]);

         }
       }
       
     }
   }


   
  }
       

  
  return 0;

}



int mrf_rtc_get(TIMEDATE *td){

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
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

const char tick_en_str[] = "tick_enable";
const char tick_dis_str[] = "tick_disable";

int mrf_tick_enable(){
  char sname[64];
  int fd,bc,tb;
  sprintf(sname,"%s%d-internal",SOCKET_DIR,MRFID);
  fd = open(sname, O_WRONLY);
  if(fd == -1){
    printf(" %d\n",fd);
    perror("mrf_tick_enable ERROR sock open\n");
    return -1;
  }
  bc = write(fd, tick_en_str,sizeof(tick_en_str) );

}

int mrf_tick_disable(){
  char sname[64];
  int fd,bc,tb;
  sprintf(sname,"%s%d-internal",SOCKET_DIR,MRFID);
  fd = open(sname, O_WRONLY);
  if(fd == -1){
    printf(" %d\n",fd);
    perror("mrf_tick_enable ERROR sock open\n");
    return -1;
  }
  bc = write(fd, tick_dis_str,sizeof(tick_dis_str) );

}

