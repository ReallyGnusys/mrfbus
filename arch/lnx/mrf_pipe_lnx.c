#include <mrf_buff.h>
#include <mrf_sys.h>
#include <mrf_if.h>
#include <mrf_route.h>
#include <mrf_debug.h>
#include <mrf_arch.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define SOCKET_DIR "/tmp/mrf_bus/"

extern uint8 _mrfid;



static int _mrf_pipe_send_lnx(I_F i_f, uint8 *buff);
static int _mrf_pipe_init_lnx(I_F i_f);
static int _mrf_pipe_buff_lnx(I_F i_f, uint8* inbuff, uint8 inlen);

const MRF_IF_TYPE mrf_pipe_lnx_if = {
 tx_del : 1,
 funcs : { send : _mrf_pipe_send_lnx,
           init : _mrf_pipe_init_lnx,
           buff : _mrf_pipe_buff_lnx}
};


int _print_mrf_cmd(MRF_CMD_CODE cmd);


// lnx i_f uses named pipes , usb uses tty
// lnx i_f opens fd for each write
// usb keeps opened ( in output_fd ) 

static int _input_fd[NUM_INTERFACES+2];

static int _bnum[NUM_INTERFACES];

static int _pipe_alloc_buff(I_F i_f){

  _bnum[i_f] = mrf_alloc_if(i_f);
  if ( _bnum[i_f] == _MRF_BUFFS){
    printf("_pipe_alloc_buff houston,we have a prob - mrf_alloc_if returned %d\n", _bnum[i_f]);
  }
  else{
    printf("_pipe_alloc_buff mrf_alloc_if returned %d\n", _bnum[i_f]);
  }
  return _bnum[i_f];
}
int _mrf_pipe_init_lnx(I_F i_f){
  int fd,tmp;
  char sname[64];
  sprintf(sname,"%s%d-%d-in",SOCKET_DIR,_mrfid,i_f);
  // create input fifo for i_f
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  printf("created pipe %s res %d\n",sname,tmp);
  fd = open(sname,O_RDONLY | O_NONBLOCK);
  printf("opened pipe i = %d  %s fd = %d\n",i_f,sname,fd);
  _pipe_alloc_buff(i_f);
  return fd; // lnx arch needs it for epoll
}


static int hex_digit_to_int(uint8 dig){
  if (( dig >= '0') && ( dig <= '9'))
    return dig - '0';
  else if(( dig >= 'A') && ( dig <= 'F'))
    return 10 + dig - 'A';
  else
    return 0;
}

static uint8 hex_dig_str_to_int(uint8 *dig){
  return 16*hex_digit_to_int(*dig) + hex_digit_to_int(*(dig+1));
}

static int  copy_to_mbuff(uint8 *buffer,int len,uint8 *mbuff){
  int i;
  if (len > _MRF_BUFFLEN * 2){ // belt and braces
    mrf_debug("ALERT copy_to_mbuff - buffer oversized at %d\n",len);
    return -1;
  }
  for ( i = 0 ; i < len/2 ; i++){
    mbuff[i] = hex_dig_str_to_int(&buffer[i*2]);
  }
  return 0;
}
// named pipe uses ascii hex encoding 
static int  copy_to_txbuff(uint8 *buffer,int len,uint8 *txbuff){
  int i;
  for ( i = 0 ; i < len ; i++){
    txbuff[i*2] = int_to_hex_digit(buffer[i]/16);
    txbuff[i*2 + 1] = int_to_hex_digit(buffer[i]%16);    
  }
  return i*2;
}


//convert raw i_f data to buffer data
static int _mrf_pipe_buff_lnx(I_F i_f, uint8* inbuff, uint8 inlen){
  int i,len;
  printf("_mrf_pipe_buff_lnx entry i_f %d len %d\n",i_f,inlen);
  //_print_mrf_cmd(mrf_cmd_device_info);
  printf("_mrf_pipe_buff_lnx about to trim_trailing_space\n");

  trim_trailing_space(inbuff);
  len = strlen(inbuff);
  if (len % 2){
    mrf_debug("ALERT - odd length packet %d\n",len);
    len = len - 1;
  }
  printf("len is %d\n",len);
  printf("%s\n",inbuff);
  // sanity checking gone bonkers
  if ( len > _MRF_BUFFLEN * 2){
    mrf_debug("ALERT - buffer oversized at %d\n",len);
    return -1;
  }
  for (i = 0 ; i < len ; i ++ )
    {
      if (is_hex_digit(inbuff[i] == 0) ){
        printf("dig %d ( %c ) not hex\n",i,inbuff[i]); 
        return -1;
      }
    }

  if (len < sizeof(MRF_PKT_HDR) * 2){
    printf("PACKET too short ( %d bytes )\n",len);
    return -1;
  }

  uint8 *mbuff = _mrf_buff_ptr(_bnum[i_f]);
  printf("about to copy to mbuff\n");
  if ( copy_to_mbuff(inbuff,len,mbuff) == 0){
    printf("copied to mbuff i_f is %d , bnum is %d\n",i_f,_bnum[i_f]);

    mrf_buff_loaded(_bnum[i_f]);
    printf("mbuff loaded!\n");
     // need to alloc next buffer
    uint8 _bnum = _pipe_alloc_buff(i_f);
    printf("_pipe_alloc_buff 'ed %d!\n",_bnum);
    return 0;
  } else {
    printf("problem copying to buff...\n");
    return -1;
  }
  
  printf("_mrf_pipe_buff_lnx exit\n");
}


static int _mrf_pipe_send_lnx(I_F i_f, uint8 *buff){
  char spath[64];
  uint8 txbuff[_MRF_BUFFLEN*2];
  int fd,bc,tb;
  uint8 sknum;
  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)buff;
  printf("_mrf_pipe_send_lnx : i_f %d  buff[0] %d sending..\n",i_f,buff[0]);
  mrf_print_packet_header(hdr);
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
  printf("_mrf_pipe_send_lnx using socket *%s*\n",spath);
  fd = open(spath, O_WRONLY | O_NONBLOCK);
  if(fd == -1){
    printf(" %d\n",fd);
    perror("ERROR file open\n");
    return -1;
  }
  if(buff[0] > _MRF_BUFFLEN){
    printf("\nGOT DANGER ERROR COMING\n");
  }
  tb = copy_to_txbuff(buff,buff[0],txbuff);
  bc = write(fd, txbuff,tb );
  //printf("bc = %d  fd = %d\n",bc,fd);
}
