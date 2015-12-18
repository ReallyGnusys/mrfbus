#include <mrf_buff.h>
#include <mrf_sys.h>
#include <mrf_if.h>
#include <mrf_route.h>
#include <mrf_debug.h>
#define SOCKET_DIR "/tmp/mrf_bus/"

extern uint8 _mrfid;


static int _mrf_pipe_send_lnx(I_F i_f, uint8 *buff);
static int _mrf_pipe_init_lnx(I_F i_f);

const MRF_IF_TYPE mrf_pipe_lnx_if = {
 tx_del : 1,
 funcs : { send : _mrf_pipe_send_lnx,
           init : _mrf_pipe_init_lnx }
};




// lnx i_f uses named pipes , usb uses tty
// lnx i_f opens fd for each write
// usb keeps opened ( in output_fd ) 

int _input_fd[NUM_INTERFACES+2];

int _mrf_pipe_init_lnx(I_F i_f){
  int fd,tmp;
  char sname[64];
  sprintf(sname,"%s%d-%d-in",SOCKET_DIR,_mrfid,i_f);
  // create input fifo for i_f
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  printf("created pipe %s res %d",sname,tmp);
  fd = open(sname,O_RDONLY | O_NONBLOCK);
  printf("opened pipe i = %d  %s fd = %d\n",i_f,sname,fd);
  return fd; // lnx arch needs it for epoll
}


int _mrf_pipe_send_lnx(I_F i_f, uint8 *buff){
  char spath[64];
  uint8 txbuff[_MRF_BUFFLEN];
  int fd,bc,tb;
  uint8 sknum;
  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)buff;
  // rough hack to get up and down using correct sockets
  if (hdr->hdest >= SNETSZ)  // rf devices only have 1 i_f
    sknum = 0;
  else if (hdr->hdest >= 1) { // usbrf or host
      if(hdr->hdest < _mrfid) 
        sknum = 1;
      else
        sknum = 0;
  }
  else  // zl-0
    sknum = 0;
  //printf("lnx_if_send_func i_f %d buff %p  len %d\n",i_f,buff,buff[0]);
  // printf("hdest %d udest %d hsrc %d usrc %d\n",hdr->hdest,hdr->udest,hdr->hsrc,hdr->usrc);
  // apologies - this is how we frig the 'wiring' on the interface to write to the intended target
  sprintf(spath,"%s%d-%d-in",SOCKET_DIR,hdr->hdest,sknum);
  printf("mrf_arch.c lnx_if_send_func using socket *%s*\n",spath);
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


