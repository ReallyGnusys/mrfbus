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


#include "mrf_uart.h"
#include "mrf_sys.h"
#include "mrf_if.h"
#include "mrf_debug.h"
#include  <msp430.h>
#include <legacymsp430.h>
#include "device.h"
#include "mrf_buff.h"

//#define mrf_buff_loaded(buff)  mrf_buff_loaded_if(UART0,buff)
#define mrf_alloc() mrf_alloc_if(UART0)
#define LP_115200
//#define LP_57600

static int mrf_uart_send_cc(I_F i_f, uint8 *buff);
static int mrf_uart_init_cc(I_F i_f);

const MRF_IF_TYPE mrf_uart_cc_if = {
 tx_del : 4,
 funcs : { send : mrf_uart_send_cc,
           init : mrf_uart_init_cc,
           buff : NULL
   }
};

static int _tx_rdy_cnt;

static UART_CSTATE rxstate;
static UART_CSTATE txstate;

uint8 _ubindex;
#define _UBUFFLEN 16
uint8 _ubuff[_UBUFFLEN];
uint8 _tbindex;
uint8 _tbuff[_UBUFFLEN];

static int disable_tx_int(){
  UCA0IE &= ~UCTXIE;  // disable this intr    
  return 0;
}
static int enable_tx_int(){
  UCA0IE &= ~UCTXIE;  // disable this intr
  UCA0IE |= UCTXIE;  //re-enable this intr
  __bis_SR_register(GIE);
  return 0;
}

static void _tx_dbg(uint8 chr){
  if (_tbindex < _UBUFFLEN) 
    _tbuff[_tbindex++] = chr;
}

static inline void _tx_byte(uint8 chr){
  _tx_dbg(chr);
  UCA0TXBUF = chr;     
}
static int mrf_uart_send_cc(I_F i_f, uint8 *buff){
  //UART_CSTATE  *txstate =  (UART_CSTATE*)sp;
  // MRF_IF *mif = mrf_if_ptr(i_f);
  if (txstate.state != S_IDLE) {
    mrf_debug("uart_if_send_func found if busy");
    enable_tx_int();
    return -1;
  }
  _tx_rdy_cnt = 0;
  txstate.buff = buff;
  txstate.state = S_START;
  txstate.bindex = 0;

  // debug 
  _tbindex = 0;
  int i;
  for ( i = 0 ; i < _UBUFFLEN; i++)
    _tbuff[i] = 0;
  enable_tx_int();
  return 0;
}

void rx_newpacket(){
  rxstate.bindex = 0;
}



static int mrf_uart_init_cc(I_F i_f){

  mrf_uart_init_rx_state(i_f,&rxstate);  
  mrf_uart_init_tx_state(i_f,&txstate);

  //dbg
  _tx_rdy_cnt =0 ;  

  //
  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
#if 0  // swap rxd txd
  PMAPPWD = 0x02D52;                        // Get write-access to port mapping 
  P1MAP6 = PM_UCA0RXD;                      // Map UCA0RXD output to P2.6 
  P1MAP5 = PM_UCA0TXD;                      // Map UCA0TXD output to P2.7 
  PMAPPWD = 0;                              // Lock port mapping registers 
#endif 

  P1SEL |= BIT5 + BIT6; // uart function
  P1DIR |= BIT6;  // tx out
  P1DIR &= ~BIT5;  // rx in

#ifdef LP_9600
  UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA0BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA0BR1 = 0x00;                           //
  UCA0MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
#else
#ifdef LP_57600
  UCA0CTL1 |= UCSSEL__SMCLK;
  UCA0BRW =   145;       // trying 96              // 
  UCA0MCTL = UCBRS_5+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
#else
#ifdef LP_115200
  UCA0CTL1 |= UCSSEL__SMCLK;  // SMCLK should be 8192 * 1024 Hz  8388608Hz
  UCA0BRW =   72;       // trying 96              // 
  UCA0MCTL = UCBRS_7+UCBRF_0;               // Modulation UCBRSx=7, UCBRFx=0
#else
    
  UCA0CTL1 |= UCSSEL__SMCLK;
  //UCA0BR0 = 69;                         //
  UCA0BR0 = 72;                           //
  UCA0BR1 = 0x00;                           //
  //UCA0MCTL = UCBRS_4+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  UCA0MCTL = UCBRS_7+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  UCA0CTL1 |= UCSSEL_2;                     // CLK = MCLK
  UCA0BR0 = 52;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA0BR1 = 0x00;                           //
  UCA0MCTL = UCBRS_0+UCBRF_0;               // Modulation UCBRSx=0, UCBRFx=0
#endif
#endif
#endif
 
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;        // enable RX interrupt 
  _ubindex = 0; // dbg
  _tbindex = 0;
 __bis_SR_register(GIE);
  return 0;
}






//#define LP_9600
int _uart_rx_int_cnt;
int _uart_tx_int_cnt;



static uint8 last_rx;

static volatile uint8 _db11;
void dbg_break(uint8 rxb){
  _db11 = rxb;
}

static uint8  _rx_byte(){
  last_rx = UCA0RXBUF;
  if (_ubindex < _UBUFFLEN) {
    _ubuff[_ubindex++] = last_rx;
    if (_ubindex > 10)
      dbg_break(last_rx);
  }
  return last_rx;
}

void _dbg_pkt_sent(UART_CSTATE *txstate){


}

static uint8 tb8;
static uint8 csum_ms,csum_ls;

void _dbg_csum_ms(uint8 val){
  csum_ms  = val;
}

void _dbg_csum_ls(uint8 val){
  csum_ls  = val;
}

interrupt (USCI_A0_VECTOR) USCI_A0_ISR()
{
  switch(UCA0IV)
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    _uart_rx_int_cnt++;
    uint8 rxb = _rx_byte();
   
    if(mrf_uart_rx_byte(rxb,&rxstate)){
      mrf_buff_loaded(rxstate.bnum);
      // need to allocate next buffer
      mrf_uart_init_rx_state(UART0,&rxstate);    // FIXME - hardcoded i_f
    }

    UCA0IE |= UCRXIE;         // re-enable RX ready interrupt
    break;
  case 4:                                   // Vector 4 - TXIFG
    _uart_tx_int_cnt++;
    tb8 = mrf_uart_tx_byte(&txstate);
    _tx_byte(tb8);


    if (mrf_uart_tx_complete(&txstate) == 0)
      UCA0IE |= UCTXIE;  //re-enable this int
    else {
      _dbg_pkt_sent(&txstate);
      UCA0IE &= ~UCTXIE;
    }

    /*
    if (mrf_uart_tx_complete(&txstate)){
      disable_tx_int();
    } else {
      _tx_byte(mrf_uart_tx_byte(&txstate));
      UCA0IE |= UCTXIE;  //re-enable this int
    }
    */
    break;
  default: break;
  }  
}
