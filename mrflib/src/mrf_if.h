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
#include "mqueue.hpp"

typedef struct{
  uint8 type;
  uint8 msgid;
  uint8 dest;
} ACK_TAG;

typedef MQueue<ACK_TAG> AckQueue;

typedef enum {
  MRF_ST_IDLE,
  MRF_ST_ACK_DEL,
  MRF_ST_TX_DEL,
  MRF_ST_ACK,
  MRF_ST_TX,
  MRF_ST_TX_FOR_ACK, // called if send_func - needs if to st to next state
  MRF_ST_WAIT_ACK,  // wait for segment ack
  MRF_ST_TX_RETRY,  // received sretry
  MRF_ST_TX_COMPLETE // tx was acknowledged, if required
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
  const uint8 ack_del;
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
  IF_STATS stats;
  IQUEUE txqueue;
  uint8 resp_timer;
  uint8 timer;
  uint8 rx_last_id;
  uint8 rx_last_src;
  uint8 tx_id;
  uint8 rx_on;  // default xbus mode , when not transmitting
  uint8 tx_buff; // I_F is tranmitting buffer at head of txq
  uint8 tx_complete;   // set by i_f driver when data transmition is complete, whether ackbuff or txq head
  uint8 waiting_resp;
  IF_STATE state;  // this is actually just TX state - RX is async
} IF_STATUS;

// I_F logic is implemented largely in _mrf_tick ( mrf_sys.c )

// sets initial flags for each transaction (tx_buff, tx_complete and waiting_resp/resp_timer) when initiating TX
// and sets state to  tx_active

// driver is required to
// set tx_complete when buffer has been transmitted

// RX callbacks, (currently buffer_responded from mrf_task_ack etc) clear waiting_resp

// mrf_tick logic :
/*
 if ((istate==TX_ACTIVE) && tx_complete)
     state = TX_IDLE

 if (istate==TX_IDLE) {
    if (waiting_resp) {
      //dec resp_time and check for timeout
      //
    }
    else if(tx_complete){
      if(tx_buff){
         pop txq
      }
      tx_complete = 0;
    }

    if(ackqueue)
      tx ack

    else if (not waiting resp and not tx_complete and txq)
      tx q


}
*/

typedef struct  {
  IF_STATUS *status;
  const MRF_IF_TYPE *type;
  MRF_PKT_HDR *ackbuff;
  AckQueue  *ackqueue;
#ifdef MRF_ARCH_lnx
  int *fd;  // fd used by lnx epoll
  const char *name;
#endif
  uint8 i_f;
} MRF_IF;

void mrf_if_init();

//voi mrf_if_register(I_F i_f, const MRF_IF_TYPE *type);

const MRF_IF *mrf_if_ptr(I_F i_f);

int8 mrf_if_tx_queue(I_F i_f, uint8 bnum );
void mrf_if_tx_done(I_F i_f);

void _mrf_if_print_all();
int mrf_if_recieving(I_F i_f);
int mrf_if_transmitting(I_F i_f);
int mrf_if_can_sleep(I_F i_f);
const char * mrf_if_state_name(I_F i_f);
void mrf_if_print_info(I_F i_f);

#endif
