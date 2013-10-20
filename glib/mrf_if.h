#ifndef _MRF_IF_INCLUDED_
#define _MRF_IF_INCLUDED_
//#include <mrf_sys.h>
#include "mrf_sys_structs.h"
#include "if.h"

typedef volatile struct  __attribute__ ((packed)){
 MRF_PKT_HDR hdr;
 MRF_PKT_TIMEDATE td; 
 uint8 xdata[64+2];  // RSSI and CRC appended
} MRF_RX_BUFF  ;


typedef enum {
  MRF_ACK_SEG,
  MRF_ACK_REC,
  MRF_ACK_RTRY,
  MRF_ACK_ERROR} MRF_ACK_CODE;

typedef enum {
  MRF_ST_NONE,
  MRF_ST_IDLE,
  MRF_ST_TX,
  MRF_ST_WAITSACK,  // wait for segment ack
  MRF_ST_WAITUACK,  // wait for ultimate ack
  MRF_ST_RX,
  MRF_ST_ACKDELAY,
  MRF_ST_ACK,
} IF_STATE;


typedef int (*IF_SEND_FUNCPTR)(I_F i_f, uint8* buff);
 

typedef struct {
  uint8 tx_del;
  IF_SEND_FUNCPTR send_func;
} MRF_IF_TYPE;


typedef enum {
  MRF_TX_IDLE,
  MRF_TX_BUSY,
  MRF_TX_POSTED,
  MRF_TX_DELIVERED,
  MRF_TX_DEFERRED,
  MRF_TX_BADACK,
  MRF_TX_NOACK,
  MRF_TX_ABORTED,
  MRF_TX_STAYAWAKE,
  MRF_TX_ERRX} IF_TX_STATUS;

typedef struct  {
  IF_STATE state;
  IF_STATE nstate;  // next state
  uint8 timer;
  MRF_RX_BUFF *rxbuff;
  MRF_RX_BUFF *ackrxbuff;
  MRF_ACK_CODE ackcode;
  uint8 rx_last_id;
  uint8 rx_last_src;
  uint8 tx_id;
  uint8 tx_retrycnt;
  IF_TX_STATUS tx_status; 
  uint8 rx_on;  // default xbus mode , when not transmitting
} IF_STATUS;


typedef struct  {
  IF_STATUS status;
  MRF_IF_TYPE *type;
} MRF_IF;

void mrf_if_init();

void mrf_if_register(I_F i_f, MRF_IF_TYPE *type);
MRF_IF *mrf_if_ptr(I_F i_f);
#endif