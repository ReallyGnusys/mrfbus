#ifndef __XQUEUES_INCLUDED__
#define __XQUEUES_INCLUDED__
#define _XB_Q_DEPTH 4
#define _XB_MAXPKTLEN  64

void xqueues_init();

int xq_app_push(uint8 *buff, uint8 len);
int xq_app_pop(uint8 *buff);
int xq_app_data_avail();
int xq_app_full();
uint8 *xq_app_head();
uint8  xq_app_head_len();


int xq_tx_push(uint8 *buff, uint8 len);
int xq_tx_pop(uint8 *buff);
int xq_tx_data_avail();
int xq_tx_full();
uint8 *xq_tx_head();
uint8  xq_tx_head_len();
#ifdef XB_UART
int xq_utx_push(uint8 *buff, uint8 len);
int xq_utx_pop(uint8 *buff);
int xq_utx_data_avail();
int xq_utx_full();
uint8 *xq_utx_head();
uint8  xq_utx_head_len();
#endif

#endif
