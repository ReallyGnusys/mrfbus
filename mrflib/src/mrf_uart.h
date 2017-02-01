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
uint8 mrf_uart_tx_byte(UART_CSTATE *txstate);

#endif
