#include "squeue.h"
#include "xqueues.h"

static SQUEUE _xb_app_q;
static uint8 _xb_app_q_buff[_XB_Q_DEPTH * _XB_MAXPKTLEN];
static uint8 _xb_app_q_blen[_XB_Q_DEPTH];

static SQUEUE _xb_tx_q;
static uint8 _xb_tx_q_buff[_XB_Q_DEPTH * _XB_MAXPKTLEN];
static uint8 _xb_tx_q_blen[_XB_Q_DEPTH];


#ifdef XB_UART
static SQUEUE _xb_utx_q;
static uint8 _xb_utx_q_buff[_XB_Q_DEPTH * _XB_MAXPKTLEN];
static uint8 _xb_utx_q_blen[_XB_Q_DEPTH];
#endif

void xqueues_init(){  
  squeue_init(&_xb_app_q,_XB_Q_DEPTH,_XB_MAXPKTLEN,
	     _xb_app_q_buff,_xb_app_q_blen); 
  squeue_init(&_xb_tx_q,_XB_Q_DEPTH,_XB_MAXPKTLEN,
	     _xb_tx_q_buff,_xb_tx_q_blen); 
#ifdef XB_UART
  squeue_init(&_xb_utx_q,_XB_Q_DEPTH,_XB_MAXPKTLEN,
	     _xb_utx_q_buff,_xb_utx_q_blen); 
#endif

}


int xq_app_push(uint8 *buff, uint8 len){
  return squeue_push(&_xb_app_q,buff,len);
}
int xq_app_pop(uint8 *buff){
  return squeue_pop(&_xb_app_q,buff);
}
uint8  *xq_app_head(){
  return squeue_head(&_xb_app_q);
}
uint8  xq_app_head_len(){
  return squeue_head_len(&_xb_tx_q);
}

int xq_app_data_avail(){
  return squeue_data_avail(&_xb_app_q);
}

int xq_app_full(){
  return squeue_full(&_xb_app_q);
}


int xq_tx_push(uint8 *buff, uint8 len){
  return squeue_push(&_xb_tx_q,buff,len);
}
int xq_tx_pop(uint8 *buff){
  return squeue_pop(&_xb_tx_q,buff);
}
uint8  *xq_tx_head(){
  return squeue_head(&_xb_tx_q);
}
uint8  xq_tx_head_len(){
  return squeue_head_len(&_xb_tx_q);
}

int xq_tx_data_avail(){
  return squeue_data_avail(&_xb_tx_q);
}

int xq_tx_full(){
  return squeue_full(&_xb_tx_q);
}
#ifdef XB_UART

int xq_utx_push(uint8 *buff, uint8 len){
  return squeue_push(&_xb_utx_q,buff,len);
}
int xq_utx_pop(uint8 *buff){
  return squeue_pop(&_xb_utx_q,buff);
}
uint8  *xq_utx_head(){
  return squeue_head(&_xb_utx_q);
}

uint8  xq_utx_head_len(){
  return squeue_head_len(&_xb_utx_q);
}



int xq_utx_data_avail(){
  return squeue_data_avail(&_xb_utx_q);
}

int xq_utx_full(){
  return squeue_full(&_xb_utx_q);
}
#endif
