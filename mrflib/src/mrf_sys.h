#ifndef __MRF_SYS_INCLUDED__
#define __MRF_SYS_INCLUDED__

#include "mrf_types.h"
#include "mrf_sys_structs.h"
#include "if.h"
#include "mrf_if.h"
#include "mrf_buff.h"

#include "mrf_cmd_def.h"
/* xbus "protected" symbols , not for application use */

#define FALSE 0
#define TRUE  1

#define _MRF_BUFFS 8
#define _MRF_BUFFLEN 64


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
  MRF_CMD_RES_ERROR
} MRF_CMD_RES;
//#define MRF_CMD_RES int




typedef MRF_CMD_RES (*MRF_CMD_FUNC)(MRF_CMD_CODE cmd, uint8 bnum , MRF_IF *ifp);


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

/* constant flags */
#define MRF_CFLG_NO_ACK 1   // send no ack when segment recipient
#define MRF_CFLG_NO_RESP 2   // send no resp when final recipient
 

#define MRF_CFLG_INTR 2  // task is run in interrupt handler

typedef struct {
  const char *str;
  const uint8 cflags;
  const uint8 req_size;
  const uint8 rsp_size;
  void *data;
  MRF_CMD_FUNC func;
} MRF_CMD;
#endif


int _mrf_process_packet(I_F owner,uint8 bnum);
void _mrf_print_packet_header(MRF_PKT_HDR *hdr,I_F i_f);
void _mrf_tick();
void _mrf_print_hex_buff(uint8 *buff,uint16 len);
void mrf_print_packet_header(MRF_PKT_HDR *hdr);
uint8 *mrf_response_buffer(uint8 bnum);
int mrf_send_response(uint8 bnum,uint8 rlen);
