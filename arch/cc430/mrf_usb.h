#ifndef _UART_INCLUDED_
#define _UART_INCLUDED_
#include  <msp430.h>
#include <legacymsp430.h>
#include "cc430f5137.h"
#include "g430_types.h"

typedef void (*RXFUNCPTR)(uint8 *);
void uart_set_rx_callback(RXFUNCPTR func);
void uart_init();
void uart_tx_string(uint8 *buffer,int maxlen);

int uart_tx_data(uint8 *buffer,int len);
int uart_rx_rdy();
int uart_data_avail();
int uart_pop(uint8 *buff);
uint8 *uart_q_entry(uint8 entry);
uint8 uart_q_entry_len(uint8 entry);
int uart_tx_rdy();


#endif
