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
#include <unistd.h>

#define SOCKET_DIR "/tmp/mrf_bus/"

//extern uint8 _mrfid;



static int _mrf_pipe_send_lnx(I_F i_f, uint8 *buff);
static int _mrf_pipe_init_lnx(I_F i_f);
static int _mrf_pipe_buff_lnx(I_F i_f, uint8* inbuff, uint8 inlen);

extern const MRF_IF_TYPE mrf_pipe_lnx_if = {
 tx_del : 2,
 ack_del: 1,
 funcs : { send : _mrf_pipe_send_lnx,
           init : _mrf_pipe_init_lnx,
           buff : _mrf_pipe_buff_lnx}
};


int _print_mrf_cmd(MRF_CMD_CODE cmd);


// lnx i_f uses named pipes , usb uses tty
// lnx i_f opens fd for each write
// usb keeps opened ( in output_fd )

//static int _input_fd[NUM_INTERFACES+2];

static int _bnum[NUM_INTERFACES];

static int _pipe_alloc_buff(I_F i_f){

  _bnum[i_f] = mrf_alloc_if(i_f);
  if ( _bnum[i_f] == _MRF_BUFFS){
    mrf_debug("_pipe_alloc_buff houston,we have a prob - mrf_alloc_if returned %d\n", _bnum[i_f]);
  }
  else{
    mrf_debug("_pipe_alloc_buff mrf_alloc_if returned %d\n", _bnum[i_f]);
  }
  return _bnum[i_f];
}
int _mrf_pipe_init_lnx(I_F i_f){
  // open input pipes
  int fd,tmp;
  char sname[64];
  sprintf(sname,"%s%d-%d-in",SOCKET_DIR,MRFID,i_f);
  // create input fifo for i_f
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  mrf_debug("created pipe %s res %d\n",sname,tmp);
  fd = open(sname,O_RDONLY | O_NONBLOCK);
  mrf_debug("opened pipe i = %d  %s fd = %d\n",i_f,sname,fd);
  _pipe_alloc_buff(i_f);
  return fd; // lnx arch needs it for epoll
}


static int hex_digit_to_int(uint8 dig){
  if (( dig >= '0') && ( dig <= '9'))
    return dig - '0';
  else if(( dig >= 'A') && ( dig <= 'F'))
    return 10 + dig - 'A';
  else {
    mrf_debug("error dig was not hexdigit , got %d\n",dig);
    return 0;
  }
}

static uint8 hex_dig_str_to_int(uint8 *dig){
  return 16*hex_digit_to_int(*dig) + hex_digit_to_int(*(dig+1));
}

static int  copy_to_mbuff(uint8 *buffer,int len,uint8 *mbuff){
  int i;

  if (len < sizeof(MRF_PKT_HDR) * 2){
    mrf_debug("PACKET too short ( %d bytes )\n",len);
    return -1;
  }


  for ( i = 0 ; i <  sizeof(MRF_PKT_HDR) ; i++){
    mbuff[i] = hex_dig_str_to_int(&buffer[i*2]);
  }

  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)mbuff;

  mrf_debug("packet hdr->length is %d  len (ascii hex) %d\n",hdr->length,len);

  if (hdr->length > _MRF_BUFFLEN) {
    mrf_debug("PACKET too long ( hdr->length %d bytes len was %d )\n",hdr->length,len);
    return -1;
  }

  if (((hdr->length)*2) > len) {
    mrf_debug("looks like incomplete packet  in buffer ( hdr->length %d , but read len %d bytes hexascii)\n",hdr->length,len);
    return -1;
  }




  for (  ; i <  hdr->length ; i++){
    mbuff[i] = hex_dig_str_to_int(&buffer[i*2]);
  }


  return i*2;
}
// named pipe uses ascii hex encoding
static int  copy_to_txbuff(uint8 *buffer,int len,uint8 *txbuff){
  int i;
  for ( i = 0 ; i < len ; i++){
    txbuff[i*2] = int_to_hex_digit(buffer[i]/16);
    txbuff[i*2 + 1] = int_to_hex_digit(buffer[i]%16);
  }
  txbuff[i*2] = '\n';

  return i*2 + 1;
}

// from mrf_arch.c (lnx)
void trim_trailing_space(uint8 *buff);

//convert raw i_f data to buffer data
static int _mrf_pipe_buff_lnx(I_F i_f, uint8* inbuff, uint8 inlen){
  int len;
  //_print_mrf_cmd(mrf_cmd_device_info);
  //mrf_debug("%s","_mrf_pipe_buff_lnx about to trim_trailing_space\n");

  trim_trailing_space(inbuff);
  len = strlen((char *)inbuff);

  mrf_debug("_mrf_pipe_buff_lnx entry i_f %d inlen %d  chk len %d\n",i_f,inlen,len);
  /*
  if (len % 2){
    mrf_debug("ALERT - odd length packet %d\n",len);
    len = len - 1;
  }
  */
  //mrf_debug("len is %d\n",len);
  //mrf_debug("%s\n",inbuff);
  // sanity checking gone bonkers
  /*
  if ( len > _MRF_BUFFLEN * 2){
    mrf_debug("ALERT - buffer oversized at %d\n",len);
    return -1;
  }
  */

  /*
  for (i = 0 ; i < len ; i ++ )
    {
      if (is_hex_digit(inbuff[i] == 0) ){
        mrf_debug("dig %d ( %c ) not hex\n",i,inbuff[i]);
        return -1;
      }
    }
  */
  if (len < sizeof(MRF_PKT_HDR) * 2){
    mrf_debug("PACKET too short ( %d bytes )\n",len);
    return -1;
  }

  int ibcnt = 0;

  while (len > 0 ) {  // we can have multiple packets waiting

    uint8 *mbuff = _mrf_buff_ptr(_bnum[i_f]);

    mrf_debug("about to copy to mbuff len (ascii) %d \n",len);

    int nc;
    nc = copy_to_mbuff(inbuff+ibcnt,len,mbuff);

    if (nc == -1) {
      mrf_debug("%s","problem copying to buff...\n");
      return -1;
    }
    len -= nc;
    ibcnt += nc;
    mrf_debug("copied %d bytes to mbuff i_f is %d , bnum is %d\n",nc,i_f,_bnum[i_f]);

    mrf_buff_loaded(_bnum[i_f]);
    mrf_debug("%s","mbuff loaded!\n");
    // need to alloc next buffer
    uint8 _bnum = _pipe_alloc_buff(i_f);
    mrf_debug("_pipe_alloc_buff 'ed %d!\n",_bnum);

    if(len > 0){
      if (is_hex_digit(inbuff[ibcnt])== 0){
        len -= 1;
        ibcnt += 1;
        mrf_debug("got non hex_digit %d - removed from buff len is now %d\n",inbuff[ibcnt-1],len);

      }

    }

  }


  mrf_debug("%s","_mrf_pipe_buff_lnx exit\n");
  return 0;
}


static int _mrf_pipe_send_lnx(I_F i_f, uint8 *buff){
  char spath[64];
  uint8 txbuff[_MRF_BUFFLEN*2];
  int fd,tb;
  //int bc;
  uint8 sknum;
  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)buff;
  mrf_debug("_mrf_pipe_send_lnx : i_f %d  buff[0] %d sending..\n",i_f,buff[0]);
  mrf_print_packet_header(hdr);

  // rough hack to get up and down using correct sockets
  if (hdr->hdest >= SNETSZ)  // rf devices only have 1 i_f
    sknum = 0;
  else if (hdr->hdest >= 1) { // usbrf or host
      if(hdr->hdest < MRFID)
        sknum = 1;
      else
        sknum = 0;
  }
  else  // zl-0
    sknum = 0;
  //printf("lnx_if_send_func i_f %d buff %p  len %d\n",i_f,buff,buff[0]);
  // mrf_debug("hdest %d udest %d hsrc %d usrc %d\n",hdr->hdest,hdr->udest,hdr->hsrc,hdr->usrc);
  // apologies - this is how we frig the 'wiring' on the interface to write to the intended target
  sprintf(spath,"%s%d-%d-in",SOCKET_DIR,hdr->hdest,sknum);
  mrf_debug("_mrf_pipe_send_lnx using socket *%s*\n",spath);
  fd = open(spath, O_WRONLY | O_NONBLOCK);
  if(fd == -1){
    mrf_debug(" %d\n",fd);
    mrf_debug("%s","ERROR file open\n");
    return -1;
  }
  if(buff[0] > _MRF_BUFFLEN){
    mrf_debug("%s","\nGOT DANGER ERROR COMING\n");
  }
  mrf_debug("copying to tx_buffer .. len %d\n",buff[0]);
  tb = copy_to_txbuff(buff,buff[0],txbuff);
  mrf_debug("copied to tx_buffer .. len %d\n",tb);

  //bc = write(fd, txbuff,tb );
  write(fd, txbuff,tb );
  fsync(fd);
  close(fd);
  mrf_debug("%s closed\n",spath);
  mrf_if_tx_done(i_f);
  //mrf_debug("bc = %d  fd = %d\n",bc,fd);
  return 0;
}
