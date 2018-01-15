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

#ifndef __MRF_SYS_INCLUDED__
#define __MRF_SYS_INCLUDED__

#define _QUOTE_SYM_(SYM)  #SYM
#define SYM_NAME(name) _QUOTE_SYM_(name)

#define _CONCAT_(A,B) # A ## B

#include "mrf_types.h"
#include "mrf_sys_structs.h"
#include "device.h"
#include "mrf_if.h"
#include "mrf_buff.h"
#include "mrf_sys_cmds.h"
#include "iqueue.h"

#define FALSE 0
#define TRUE  1

#define _MRF_BUFFLEN 64


#define _MRF_APP_CMD_BASE 128   // app commands start at 128
typedef struct {
  int len;
  int csumok;
  uint8 linktoks[2];
} RX_PKT_INFO;


typedef volatile struct  __attribute__ ((packed)){
 MRF_PKT_HDR hdr;
 MRF_PKT_TIMEDATE td; 
 uint8 xdata[64+2];  // RSSI and CRC appended
} MRF_RX_BUFF;


extern volatile MRF_RX_BUFF _xb_rx_buff;
typedef enum {
  MRF_CMD_RES_RETRY,
  MRF_CMD_RES_OK,
  MRF_CMD_RES_IGNORE,
  MRF_CMD_RES_WARN,
  MRF_CMD_RES_ERROR
} MRF_CMD_RES;
//#define MRF_CMD_RES int




typedef MRF_CMD_RES (*MRF_CMD_FUNC)(MRF_CMD_CODE cmd, uint8 bnum , const MRF_IF *ifp);

#ifdef MRF_ARCH_lnx
typedef int (*MRF_APP_CALLBACK)(int fd);
#endif

#ifndef HOSTBUILD
#define MRF_CMD_FUNC_DEC(dec)   dec
#else
#define MRF_CMD_FUNC_DEC(dec)   NULL
#endif

/* variable command flags  - MSBs of command type code*/
#define MRF_VFLG_PAYLOAD 0x20
#define MRF_VFLG_INITF   0x40
#define MRF_VFLAG_SEND   0x80

#define MRF_VFLAG_MASK   (MRF_VFLG_PAYLOAD | MRF_VFLG_INITF | MRF_VFLAG_SEND )
#define MRF_CODE_MASK   ~MRF_VFLAG_MASK

/* application signals are generated using reserved set of buffer numbers
   starting from MRF_BNUM_SIGNAL_BASE at the top of the bnum 8 bit space
*/

#define MRF_NUM_SIGNALS  32
#define MRF_BNUM_SIGNAL_BASE (256 - MRF_NUM_SIGNALS)

#define APP_SIG_SECOND  0
#define APP_SIG_BUT1  1



/* constant flags */
#define MRF_CFLG_NO_ACK 1   // send no ack when segment recipient
#define MRF_CFLG_NO_RESP 2   // send no resp when final recipient
#define MRF_CFLG_INTR 4  // task is run in interrupt handler

typedef struct {
  const uint8 str[16];
  const uint8 cflags;
  const uint8 req_size;
  const uint8 rsp_size;
  const void *data;
  const MRF_CMD_FUNC func;
} MRF_CMD;
#endif


int _mrf_process_packet(I_F owner,uint8 bnum);
//void _mrf_print_packet_header(MRF_PKT_HDR *hdr,I_F i_f);
void _mrf_tick();
void _mrf_print_hex_buff(uint8 *buff,uint16 len);
void mrf_print_packet_header(MRF_PKT_HDR *hdr);
uint8 *mrf_response_buffer(uint8 bnum);
int mrf_send_response(uint8 bnum,uint8 rlen);
int mrf_send_structure(uint8 dest, uint8 code,  uint8 *data, uint8 len);
int mrf_send_command_root(uint8 dest, uint8 code,  uint8 *data, uint8 len);
int mrf_send_command(uint8 dest, uint8 code,  uint8 *data, uint8 len);

uint16 mrf_copy(void *src,void *dst, size_t nbytes);
uint16 mrf_scopy(void *src,void *dst, size_t nbytes);
const MRF_CMD *mrf_cmd_ptr(uint8 type);
const MRF_CMD *mrf_app_cmd_ptr(uint8 type);
// these must be defined by every application
int mrf_app_init();
int signal_handler(uint8 signal);
int mrf_app_queue_available();

// these are defined in arch  but here is prototype
int mrf_arch_boot();
int mrf_arch_run();
int mrf_tick_enable();
int mrf_tick_disable();
int mrf_wake();
int mrf_sleep();
int mrf_wake_on_exit();
// end arch
void mrf_sys_init();
int _mrf_buff_forward(uint8 bnum);
int _mrf_process_buff(uint8 bnum);
int mrf_foreground();
int mrf_app_queue_push(uint8 bnum);
int mrf_data_response(uint8 bnum,const uint8 *data,uint8 len);
int mrf_rtc_set(TIMEDATE *td);
int mrf_rtc_get(TIMEDATE *td);
int mrf_retry(I_F i_f,uint8 bnum);

int mrf_app_signal(uint8 signum);

#ifdef SLEEP_deep
int mrf_sleep_deep();  // must be defined by app for now
#endif

#include "mrf_sys_tasks.h"
#include "mrf_sys_cmds.h"


#include "mrf_app.h"
#include "mrf_app_cmds.h"
#include "iqueue.h"
