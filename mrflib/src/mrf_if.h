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
typedef int (*IF_BUFF_FUNCPTR)(I_F i_f, uint8* inbuff, uint8 inlen, uint8 tobnum);

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

#define MRF_IF_QL 4

typedef struct  __attribute__ ((packed))  {
  uint16 rx_pkts;
  uint16 tx_pkts;
  uint16 tx_overruns;
  uint16 tx_retries;  
  uint8  unexp_ack;  
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

void mrf_if_register(I_F i_f, const MRF_IF_TYPE *type);

MRF_IF *mrf_if_ptr(I_F i_f);

int8 mrf_if_tx_queue(I_F i_f, uint8 bnum );
void _mrf_if_print_all();
int mrf_if_recieving(I_F i_f);
int mrf_if_transmitting(I_F i_f);

#endif
