#ifndef _MRF_UART_INCLUDED_
#define _MRF_UART_INCLUDED_

#include "mrf_types.h"
#include "mrf_if.h"

#define _MRF_UART_PREAMBLE 0x55
  // uart line state
typedef enum _rstate {
  S_IDLE       = 0,
  S_START      = 1,
  S_PREAMBLE_1 = 2,
  S_LEN        = 3,
  S_ADDR       = 4,
  S_NETID      = 5,
  S_HDR        = 6,
  S_DATA       = 7,
  S_CSUM_MS    = 8,
  S_CSUM_LS    = 9
} UART_LSTATE;

// uart channel state
typedef struct {
  UART_LSTATE state;
  uint8 bnum;  // mrfbuff num
  uint8 *buff; // buff ptr kept for convenience
  uint8 bindex; 
  uint16 csum;
  uint16 errors;
} UART_CSTATE;

int mrf_uart_init();
int mrf_uart_rx_byte(uint8 rxbyte, UART_CSTATE *rxstate);
int mrf_uart_to_buff(I_F i_f, uint8* inbuff, uint8 inlen, uint8 tobnum);
int mrf_uart_init_rx_state(I_F i_f,UART_CSTATE *rxstate);
int mrf_uart_init_tx_state(I_F i_f,UART_CSTATE *txstate);

#endif
