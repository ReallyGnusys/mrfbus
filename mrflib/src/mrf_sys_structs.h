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

#ifndef __MRF_SYS_STRUCTS_INCL__
#define __MRF_SYS_STRUCTS_INCL__

#include <mrf_types.h>
#include <device.h> // for I_F type
#include "mqueue.hpp"

//#include <mrf_if.h>
// put I/F and therefore device related structs here for simplicity




// tag format - item in ackqueue
typedef struct{
  uint8 type;
  uint8 msgid;
  uint8 dest;
} ACK_TAG;


// define types for ack and buff queues
typedef MQueue<ACK_TAG,IQUEUE_DEPTH> AckQueue;
typedef MQueue<uint8_t,IQUEUE_DEPTH> BuffQueue;



// This is really just TX state - FIXME needs renaming
typedef enum {
  IDLE,
  DELAY_ACK,
  DELAY_BUFF,
  TX_ACK,
  TX_BUFF} IF_STATE;

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
  uint8 resp_timer;
  uint8 timer;
  uint8 rx_last_id;
  uint8 rx_last_src;
  uint8 tx_id;
  uint8 rx_on;  // default xbus mode , when not transmitting
  //uint8 buff_active; // buff transaction in progress
  uint8 tx_complete;   // set by i_f driver when data transmition is complete, whether ackbuff or txq head
  uint8 waiting_resp;
  uint8 resp_received;
  uint8 tx_retried;  // set by RX callbacks - task_retry
  IF_STATE state;
} IF_STATUS;


// end I_F structs

// mainly ( should be ) structures used as params or responses to mrf commands
typedef struct  __attribute__ ((packed)) {
  uint8 length;  // packet length
  uint8 hdest;  // hop destination
  uint8 netid;
  uint8 udest;// ultimate destination of this packet
  uint8 type;
  uint8 hsrc; // hop source
  uint8 usrc; // ultimate source of this packet
  uint8 msgid;
}MRF_PKT_HDR ;

typedef struct  __attribute__ ((packed)) timedate {
  uint8 sec;
  uint8 min;
  uint8 hour;
  uint8 day;
  uint8 mon;
  uint8 year;  // 0 = 2000CE
}TIMEDATE;


typedef struct  __attribute__ ((packed))   {
  uint8 type;
  uint8 msgid;
  uint8 rlen;
} MRF_PKT_RESP;


#define NDR_RECD_SRETRY 0
#define NDR_MAX_RETRIES 1
#define NDR_I_F_ERROR   2


typedef struct  __attribute__ ((packed))   {
  uint8 code;
  uint8 msgid;
  uint8 hsrc;
  uint8 hdest;
} MRF_PKT_NDR;



typedef struct  __attribute__ ((packed))   {
  /*
  uint8 num_if;
  uint8 buffs_total;
  */
  uint8 buffs_free;
  uint8 errors;
  uint16 tx_retries;
  uint32 rx_pkts;
  uint32 tx_pkts;
  uint32 tick_count;
} MRF_PKT_DEVICE_STATUS;

typedef struct  __attribute__ ((packed))   {
  uint8 value;
}MRF_PKT_UINT8;

typedef struct  __attribute__ ((packed))   {
  uint8 d0;
  uint8 d1;
}MRF_PKT_UINT8_2;

typedef struct  __attribute__ ((packed))   {
  uint16 value;
}MRF_PKT_UINT16;


// 53 bytes
typedef struct  __attribute__ ((packed))   {
  char dev_name[10];
  uint8 mrfid;
  uint8 netid;
  uint8 num_buffs;
  uint8 num_ifs;
} MRF_PKT_DEVICE_INFO;

typedef struct  __attribute__ ((packed))   {
  char  mrfbus_version[42];
  char  build[8];
  uint8 num_cmds;
  uint8 modified;

} MRF_PKT_SYS_INFO;


typedef TIMEDATE MRF_PKT_TIMEDATE;
typedef TIMEDATE MRF_TIMEDATE;

typedef  enum { FREE,
		LOADING, // allocated and being written by IF or app
		LOADED,  // loaded and awaiting classification
		TXQUEUE, // loaded and requires forwarding via an interface ,
                TX,  // currently being transmitted by I_F
                APPIN  // loaded and requires processing by app
} mrf_buff_state_t;

typedef struct __attribute__ ((packed)){
  mrf_buff_state_t state;
  I_F owner;
  uint16 tx_timer;
  uint8 retry_count;
} MRF_BUFF_STATE;



typedef struct  __attribute__ ((packed))   {
  uint8 id;
  MRF_BUFF_STATE state;
} MRF_PKT_BUFF_STATE;

typedef struct  __attribute__ ((packed))   {
  uint8 name[16];
  uint8 type;
  uint8 cflags;
  uint8 req_size;
  uint8 rsp_size;
} MRF_PKT_CMD_INFO;

typedef struct  __attribute__ ((packed))   {
  uint8 name[16];
  uint8 num_cmds;
} MRF_PKT_APP_INFO;


typedef struct __attribute__ ((packed)){
  uint32 rx_pkts;
  uint32 tx_acks;
  uint32 tx_pkts;
  uint16 rx_crc_errs;
  uint16 rx_net_errs;
  uint16 rx_short;
  uint16 rx_long;
  uint16 tx_fails;
  uint16 tx_retries;
  uint16 tx_no_eop;
  uint16 rx_overruns;
  uint16 rx_unexp_ack;
  uint16 tick_err;
  uint16 fatal;
  uint16 txn[4];
} MRF_PKT_STATS;

typedef struct  __attribute__ ((packed))   {
  uint16 data[8];
} MRF_PKT_DBG_UINT8;


typedef struct  __attribute__ ((packed))   {
  char data[32];
} MRF_PKT_DBG_CHR32;


typedef struct  __attribute__ ((packed))   {
  uint8 to_rssi;
  uint8 to_lqi;
  uint8 from_rssi;
  uint8 from_lqi;
} MRF_PKT_PING_RES;


// INSTANTIATIONS IN _sys_ifs table




typedef struct  {
  uint8              i_f;
  const MRF_IF_TYPE *type;
  IF_STATUS         *status;
  MRF_PKT_HDR       *ackbuff;
  AckQueue          *ackqueue;
  BuffQueue         *txqueue;

#ifdef MRF_ARCH_lnx
  int               *fd;  // fd used by lnx epoll
  const char        *name;
#endif
} MRF_IF;


// macro to assist instantiations in device.c files

#ifdef MRF_ARCH_lnx
#define MRF_IF_DEF(en,type) {en, &type, &_if_status[en], &_if_ackbuffs[en], &_if_ack_queues[en], &_if_tx_queues[en], &_if_fd[en], #en}
#define MRF_IF_DEF_PATH(en,type,path) {en, &type, &_if_status[en], &_if_ackbuffs[en], &_if_ack_queues[en], &_if_tx_queues[en], &_if_fd[en], path}
#else
#define MRF_IF_DEF(en,type) {en, &type, &_if_status[en], &_if_ackbuffs[en], &_if_ack_queues[en], &_if_tx_queues[en]}

#endif

#endif
