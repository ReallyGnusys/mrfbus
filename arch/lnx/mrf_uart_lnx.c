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

#include "mrf_types.h"
#include "mrf_uart.h"
#include "mrf_sys.h"
#include "mrf_if.h"
#include "mrf_debug.h"

// linux
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#define BAUDRATE  B115200 
//#define BAUDRATE B9600
//#ifdef LP_115200
//#undef LP_115200
//#endif


// FIXME bodge device names here for time being
#define DEVICE_BASE "/dev/ttyUSB"

static int _usb_if_send_func(I_F i_f, uint8 *buff);
static int _mrf_uart_init_lnx(I_F i_f);
static int _mrf_uart_input(I_F i_f, uint8* inbuff, uint8 inlen);

const MRF_IF_TYPE mrf_uart_lnx_if = {
 tx_del : 4,
 funcs : { send : _usb_if_send_func,
           init : _mrf_uart_init_lnx,
           buff : _mrf_uart_input
  }
};

//dirty hack - keep MAX_UARTS fds in array
#define MAX_UARTS 8
static int _fd[MAX_UARTS];

// keep RXSTATE in array;

static UART_CSTATE rxstate[MAX_UARTS];

int copy_to_txbuff(uint8 *buff, uint8 *dest){
  int i = 0;
  UART_CSTATE txstate;
  mrf_uart_init_tx_state(0,&txstate);
  txstate.buff = buff;
  txstate.state = S_START;

  while ( txstate.state != S_IDLE){
    dest[i++] = mrf_uart_tx_byte(&txstate);
  }

  return i;

}

static int _mrf_uart_input(I_F i_f, uint8* inbuff, uint8 inlen){
  uint8 i,rv;
  UART_LSTATE stb,sta;
  mrf_debug("_mrf_uart_input i_f %d inlen %d inbuff[0] %x\n",i_f,inlen,inbuff[0]);
  for ( i = 0 ; i < inlen ; i++){
    stb =  rxstate[i_f].state;
    rv = mrf_uart_rx_byte(inbuff[i], &(rxstate[i_f]));
    sta =  rxstate[i_f].state;
    mrf_debug("i %d char %x stb %d sta %d rv %d\n",i,inbuff[i],stb,sta,rv);
    if(rv){
        mrf_debug("lnx_uart input - think we got a buffer");
        mrf_buff_loaded(rxstate[i_f].bnum);
        // need to alloc next buffer;
        mrf_uart_init_rx_state(i_f,&(rxstate[i_f]));
      }
  }
  return i;
}
static int _usb_if_send_func(I_F i_f, uint8 *buff){
  char spath[64];
  uint8 txbuff[_MRF_BUFFLEN+8];
  int fd,bc,tb;
  uint8 sknum;
  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)buff;
  fd = _fd[i_f];
  mrf_debug("_usb_if_send_func fd = %d\n",fd);
  if(fd < 0){
    mrf_debug("fd not valid %d\n",fd);
    perror("ERROR send\n");
    return -1;
  }
  _mrf_print_hex_buff(buff,10);

  
  tb = copy_to_txbuff(buff,txbuff);
  mrf_debug("post copy buff[0] %u tb = %d",buff[0],tb);
  _mrf_print_hex_buff(txbuff,15);

  
  bc = write(fd, txbuff,tb);
  mrf_debug("attempted to send buff len %d after format %d written %d\n",buff[0],tb,bc);
  /*
  bc = write(fd,buff,buff[0]);
  mrf_debug("attempted to send buff len %d written %d\n",buff[0],bc);
  */
  /*
  bc = write(fd,"muppets\n",sizeof("muppets\n"));
  mrf_debug("attempted to send buff len %lu written %d\n",sizeof("muppets\n"),bc);
  */
  fsync(fd);
  //printf("bc = %d  fd = %d\n",bc,fd);
}

// open usb i_f

static struct termios _oldtio;


int usb_open(const char *dev){
  struct termios newtio,chktio;
  //int fd = open((char *)dev, O_RDWR | O_NOCTTY | O_SYNC); 
  int fd = open((char *)dev, O_RDWR); 
  if (fd < 0) {
    printf("failed to open %s\n",dev);
    perror(dev); 
    return -1; 
  }
  printf("%s opened ok fd = %d\n",dev,fd);
  tcgetattr(fd,&_oldtio); /* save current port settings */  

  printf("ispeed %d ospeed %d\n",cfgetispeed(&_oldtio),cfgetospeed(&_oldtio));

  /*
  int rc;
  rc = ioctl(fd, USBDEVFS_RESET, 0);
  if (rc < 0) {
    perror("Error in ioctl");
    return 1;
  }
  printf("Reset successful\n");
  */
  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
 
  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;
 
  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

  tcflush(fd, TCIFLUSH);
  int rv = tcsetattr(fd,TCSANOW,&newtio);  
  printf("usb_open : fd = %d tcsetattr returned %d\n",fd,rv);
  printf("newtio.c_oflag %x newtio.c_iflag %x newtio.c_cflag %x newtio.c_lflag %x\n",
         newtio.c_oflag, newtio.c_iflag,newtio.c_cflag,newtio.c_lflag);
  tcgetattr(fd,&chktio); /* check current port settings */  
  printf("chktio.c_oflag %x chktio.c_iflag %x chktio.c_cflag %x chktio.c_lflag %x\n",
         chktio.c_oflag, chktio.c_iflag,chktio.c_cflag,chktio.c_lflag);

  return fd;
}


static int _mrf_uart_init_lnx(I_F i_f){
  char devname[64];
  int fd;
  if (i_f >= MAX_UARTS){
    mrf_debug("error mrf_uart_init_lnx i_f %d\n",i_f);
    return -1;
  }
  MRF_IF *mif = mrf_if_ptr(i_f); 
  mrf_debug("mrf_uart_init_lnx having a go at opening %s for i_f %d\n",mif->name,i_f);

  fd = usb_open(mif->name);

  if (fd < 0){
    mrf_debug("mrf_uart_init_lnx sadly failed to open %s..adios\n",devname);
    _fd[i_f] = -1;
    return fd;
  }
  
  mrf_debug("opened %s fd %d for i_f %d\n",mif->name,fd,i_f);
  _fd[i_f] = fd;
  mrf_uart_init_rx_state(i_f,&(rxstate[i_f]));  

  return fd; // lnx arch needs it for epoll
}
