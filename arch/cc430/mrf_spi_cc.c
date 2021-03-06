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

int __attribute__ ((constructor)) mrf_spi_init() ;  // seems futile to expect init to be called automatically

uint16 _spi_rx_int_cnt;
uint16 _spi_tx_int_cnt;
uint16 _spi_txrx_int_cnt;
uint16 _spi_tx_bytes;
uint16 _spi_rx_bytes;
uint16 _spi_rxov_err;
uint16 _mrf_spi_busy_errors;

static  BuffQueue _spi_rx_queue,_spi_tx_queue;

static int _enable_spi_tx_int(){
  //UCB0IE &= ~UCTXIE;  // disable this intr
  UCB0IE |= UCTXIE;  //re-enable this intr
  // __bis_SR_register(GIE); // possibly interesting if this is done within IRQ handler
  return 0;
}

static int _enable_spi_rx_int(){

  UCB0IE |= UCRXIE;   // enable RX interrupt
  // __bis_SR_register(GIE);  // don't want this set in intr accidentally
  return 0;
}
static uint16_t _spi_tx_imm , _spi_tx_err1, _spi_tx_err2, _spi_tx_err3;
int _start_spi_tx(){

  if (!_spi_tx_queue.data_avail()){
    _spi_tx_err1++;
    return -1;
  }
  UCB0IE |= UCTXIE;  //re-enable this intr

  if ((UCB0STAT & 1) == 0){ // UCB0 is idle
    UCB0IFG |= UCTXIFG;   // try and set FLG to trigger tx interrupt

  }
  //UCB0IE &= ~UCTXIE;  // disable this intr
#if 0

  if (UCB0IFG & UCTXIFG){ // TX buff empty


    int16 qdata = *(_spi_tx_queue.pop());
    if (qdata == -1) {
      _spi_tx_err2++;
      return -1;
    }
    UCB0TXBUF = (uint8)qdata;
    _spi_tx_bytes += 1;
    _spi_tx_imm += 1;
    return 0;
  }
  _spi_tx_err3++;

#endif
  return 0;
}

static int  _dbg_spi_tx_err1;

int mrf_spi_txq(uint8 tx_byte){
  int rv = _spi_tx_queue.push(tx_byte);
  if (rv == -1 ) {
    _dbg_spi_tx_err1++;
    return -1;
  }
  return rv;
}
int mrf_spi_tx(uint8 tx_byte){
  int rv = _spi_tx_queue.push(tx_byte);
  if (rv == -1 ) {
    _dbg_spi_tx_err1++;
    return -1;
  }
  _start_spi_tx();
  //_enable_spi_tx_int();
  return rv;
}

void mrf_spi_start_tx(){
  _start_spi_tx();
  //_enable_spi_tx_int();

}

int mrf_spi_tx_data_avail(){
  return _spi_tx_queue.data_avail();

}

uint8 mrf_spi_rx(){
  // block and enter LPM3 until rx byte received
  while(!_spi_rx_queue.data_avail()){
    _enable_spi_rx_int();
    mrf_sleep();
  }
  return (uint8)*(_spi_rx_queue.pop());
}

uint16 mrf_spi_rx_noblock(){  // return -1 if nothing available
  return *(_spi_rx_queue.pop());
}

int mrf_spi_busy(){

  if (((UCB0STAT & 1) == 0) && mrf_spi_tx_data_avail()){
    _mrf_spi_busy_errors++;   // FIXME some deadlock if this counts up
    _start_spi_tx();
    return 1;
  }

  return (mrf_spi_tx_data_avail() || (UCB0STAT & 1));


}
int mrf_spi_flush_rx(){
  return _spi_rx_queue.flush();
}

int mrf_spi_data_avail(){
  return _spi_rx_queue.data_avail();
}

static int disable_tx_int(){
  UCB0IE &= ~UCTXIE;  // disable this intr
  return 0;
}

BuffQueue *mrf_spi_tx_queue(){
  return &_spi_tx_queue;
}

BuffQueue *mrf_spi_rx_queue(){
  return &_spi_rx_queue;
}

int mrf_spi_tx_queue_items(){
  return _spi_tx_queue.items();
}

int mrf_spi_rx_queue_items(){
  return _spi_rx_queue.items();
}



int  __attribute__ ((constructor)) mrf_spi_init(){

  _spi_rx_int_cnt = 0;
  _spi_tx_int_cnt = 0;
  _spi_txrx_int_cnt = 0;
  _mrf_spi_busy_errors = 0;
  _spi_rx_bytes = 0;
  _spi_tx_bytes = 0;
  _spi_rxov_err = 0;
  _spi_tx_imm = 0;
  _spi_tx_err1 = 0;
  _spi_tx_err2 = 0;
  _spi_tx_err3 = 0;
  _spi_rx_queue = BuffQueue();
  _spi_tx_queue = BuffQueue();

  //
  UCB0CTL1 = UCSWRST;                      // **Put state machine in reset**

  P1DIR |= BIT3 + BIT4 ; // spi MOSI and CLK outputs

  P1SEL |= BIT2 + BIT3 + BIT4; // spi function. shouldn't need STE +  BIT7 ; // spi function


  UCB0CTL0 = UCMODE_0 + UCMST + UCMSB + UCSYNC; //  + UCCKPH; // + UCSYNC


  // should be about 115 kb copied from uart
  UCB0CTL1 |= UCSSEL__SMCLK;
  //UCB0BRW =   72;                //
  //UCB0BRW =   72;// really a magic number... along with 74... hard to find another!



  //UCB0BRW =   96;//   back off clock searching for better noise immunity  - works but doesn't seem better than 72

  UCB0BRW =   50;//   speed up clock! searching for better noise immunity


  //UCB0BRW =   256;//



  //UCB0BRW =  36;// stab in dark seems good
  //UCB0BRW =  36;// stab in dark seems good


  _dbg_spi_tx_err1 = 0;

  //UCB0MCTL = UCBRS_7+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0

  UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  _enable_spi_rx_int();  // think this must be done after reset released

  return 0;
}


static uint8 last_rx;


static int  _rx_byte(){
  // called in intr handler below
  // just read byte from RXBUF and push to queue
  if ( UCB0STAT & UCOE )
    _spi_rxov_err++;
  last_rx = UCB0RXBUF;
  return _spi_rx_queue.push(last_rx);
}


interrupt (USCI_B0_VECTOR) USCI_B0_ISR()
{
  switch(UCB0IV)
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    _spi_rx_int_cnt++;
    _rx_byte();
    _spi_rx_bytes += 1;
    //__bic_SR_register_on_exit(LPM3_bits);    // exit LPM3
    mrf_wake();
    if (mrf_wake_on_exit())
      __bic_SR_register_on_exit(LPM3_bits);
   UCB0IE |= UCRXIE;         // re-enable RX ready interrupt
   break;
  case 4:                                   // Vector 4 - TXIFG
    _spi_tx_int_cnt++;
    if(_spi_tx_queue.data_avail()){
      uint8 txchr = (uint8)*(_spi_tx_queue.pop());
      UCB0TXBUF = txchr;
      _spi_tx_bytes += 1;
        //UCB0IE |= UCTXIE;  //re-enable this int

        /*
        if(queue_data_avail(&_spi_tx_queue)) // re-enable this intr
          UCB0IE |= UCTXIE;  //re-enable this int
        else
          UCB0IE &= ~UCTXIE;
        */
    }
    UCB0IE |= UCTXIE;  //re-enable this int

#if 0
    else {
      UCB0IE &= ~UCTXIE;
    }

    // check RX while we're here
    if (UCB0IFG & UCRXIFG) {
      _spi_txrx_int_cnt++;
      _rx_byte();
      _spi_rx_bytes += 1;
      UCB0IE |= UCRXIE;         // re-enable RX ready interrupt
      //__bic_SR_register_on_exit(LPM3_bits);    // exit LPM3
      mrf_wake();
      if (mrf_wake_on_exit())
        __bic_SR_register_on_exit(LPM3_bits);

    }

    //UCB0IE |= UCTXIE;  //re-enable this int
 #endif
    break;
  default: break;
  }
}
