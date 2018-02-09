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

#ifndef _MRF_IF_INCLUDED_
#define _MRF_IF_INCLUDED_
//#include <mrf_sys.h>
//#include <device.h>
#include "mrf_sys_structs.h"
#include "iqueue.h"



typedef enum {
  MRF_ACK_SEG,
  MRF_ACK_REC,
  MRF_ACK_RTRY,
  MRF_ACK_ERROR} MRF_ACK_CODE;

typedef enum {
  MRF_ST_NONE,
  MRF_ST_IDLE,
  MRF_ST_TXQ,
  MRF_ST_TX,
  MRF_ST_WAITSACK,  // wait for segment ack
  MRF_ST_WAITUACK,  // wait for ultimate ack
  MRF_ST_RX,
  MRF_ST_ACKDELAY,
  MRF_ST_ACK,
} IF_STATE;


#include "device.h"

typedef int (*IF_SEND_FUNCPTR)(I_F i_f, uint8* buf);
typedef int (*IF_INIT_FUNCPTR)(I_F i_f);
typedef int (*IF_BUFF_FUNCPTR)(I_F i_f, uint8* inbuff, uint8 inlen);

typedef struct {
  const IF_SEND_FUNCPTR send;
  const IF_INIT_FUNCPTR init;
  const IF_BUFF_FUNCPTR buff;
} IF_FUNCS;
  
typedef struct {
  const uint8 tx_del;
  const IF_FUNCS funcs;
} MRF_IF_TYPE;



enum{
  MRF_BUFF_NONE = 255
};


typedef struct  __attribute__ ((packed))  {
  uint32 rx_pkts;
  uint32 tx_pkts;
  uint32 tx_acks;
  uint16 tx_overruns;
  uint16 tx_retries;
  uint16 tx_retried;
  uint16 tx_errors;  
  uint16  unexp_ack;  
  uint16  rx_ndr;  
  uint8  alloc_err;
  uint8  st_err;
} IF_STATS;

typedef struct  {
  IF_STATE state;
  IF_STATS stats;
  uint8 acktimer;
  IQUEUE txqueue;
  uint8 rx_last_id;
  uint8 rx_last_src;
  uint8 tx_id;
  uint8 rx_on;  // default xbus mode , when not transmitting
} IF_STATUS;


typedef struct  {
  IF_STATUS *status;
  const MRF_IF_TYPE *type;
  MRF_PKT_HDR *ackbuff;
#ifdef MRF_ARCH_lnx
  int *fd;  // fd used by lnx epoll
  const char *name;
#endif
} MRF_IF;

void mrf_if_init();

//voi mrf_if_register(I_F i_f, const MRF_IF_TYPE *type);

const MRF_IF *mrf_if_ptr(I_F i_f);

int8 mrf_if_tx_queue(I_F i_f, uint8 bnum );
void _mrf_if_print_all();
int mrf_if_recieving(I_F i_f);
int mrf_if_transmitting(I_F i_f);
int mrf_if_can_sleep(I_F i_f);

#endif
