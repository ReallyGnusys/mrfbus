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
//#include <mrf_if.h>


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

typedef struct  __attribute__ ((packed))   {
  uint8 num_if;
  uint8 buffs_total;
  uint8 buffs_free;
  uint8 errors;
  uint16 tx_retries;
  uint32 rx_pkts;
  uint32 tx_pkts;
} MRF_PKT_DEVICE_STATUS;

typedef struct  __attribute__ ((packed))   {
  uint8 value;
}MRF_PKT_UINT8;

typedef struct  __attribute__ ((packed))   {
  uint8 d0;
  uint8 d1;
}MRF_PKT_UINT8_2;



// 53 bytes
typedef struct  __attribute__ ((packed))   {
  char dev_name[10];
  uint8 mrfid;
  uint8 netid;
  uint8 num_buffs;
  uint8 num_ifs;
} MRF_PKT_DEVICE_INFO;

typedef struct  __attribute__ ((packed))   {
  uint8 num_cmds;
  char  mrfbus_version[40];
  uint8 modified;
  char  build[8];
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
  uint8 type;
  uint8 name[16];
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

#endif
