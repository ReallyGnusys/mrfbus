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
uint16 _spi_tx_bytes;
uint16 _spi_rx_bytes;


static IQUEUE _spi_rx_queue,_spi_tx_queue;

static int _enable_spi_tx_int(){
  UCB0IE &= ~UCTXIE;  // disable this intr
  UCB0IE |= UCTXIE;  //re-enable this intr
  __bis_SR_register(GIE);
  return 0;
}

static int _enable_spi_rx_int(){

  UCB0IE |= UCRXIE;   // enable RX interrupt 
  __bis_SR_register(GIE);
  return 0;
}


int mrf_spi_tx(uint8 tx_byte){
  int rv = queue_push(&_spi_tx_queue,tx_byte);
  _enable_spi_tx_int();
  return rv;
}

int mrf_spi_tx_data_avail(){
  return queue_data_avail(&_spi_tx_queue);

}

uint8 mrf_spi_rx(){
  // block and enter LPM3 until rx byte received 
  while(!queue_data_avail(&_spi_rx_queue)){
    _enable_spi_rx_int();
    mrf_sleep();
  }
  return (uint8)queue_pop(&_spi_rx_queue);
}

int mrf_spi_flush_rx(){
  return queue_flush(&_spi_rx_queue);
}

int mrf_spi_data_avail(){
  return queue_data_avail(&_spi_rx_queue);
}

static int disable_tx_int(){
  UCB0IE &= ~UCTXIE;  // disable this intr    
  return 0;
}


int mrf_spi_tx_queue_items(){
  return queue_items(&_spi_tx_queue);
}

int mrf_spi_rx_queue_items(){
  return queue_items(&_spi_rx_queue);
}



int  __attribute__ ((constructor)) mrf_spi_init(){

  _spi_rx_int_cnt = 0;
  _spi_tx_int_cnt = 0;
  
  _spi_rx_bytes = 0;
  _spi_tx_bytes = 0;
 
  queue_init(&_spi_rx_queue);
  queue_init(&_spi_tx_queue);
  
  //
  UCB0CTL1 |= UCSWRST;                      // **Put state machine in reset**

  P1SEL |= BIT2 + BIT3 + BIT7 ; // spi function

  UCB0CTL0 = UCMODE_0 + UCMST + UCMSB;
  
  // should be about 115 kb copied from uart
  UCB0CTL1 |= UCSSEL__SMCLK;
  UCB0BRW =   72;                // 
  //UCB0MCTL = UCBRS_7+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  
  UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  _enable_spi_rx_int();

  return 0;
}


static uint8 last_rx;


static uint8  _rx_byte(){
  // called in intr handler below
  // just read byte from RXBUF and push to queue
  last_rx = UCB0RXBUF;
  return queue_push(&_spi_rx_queue,last_rx);
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
   __bic_SR_register_on_exit(LPM3_bits);    // exit LPM3
    UCB0IE |= UCRXIE;         // re-enable RX ready interrupt
    break;
  case 4:                                   // Vector 4 - TXIFG
    _spi_tx_int_cnt++;
    if(queue_data_avail(&_spi_tx_queue)){
        uint8 txchr = (uint8)queue_pop(&_spi_tx_queue);
        UCB0TXBUF = txchr;
        _spi_tx_bytes += 1;
    }
        /*
        if(queue_data_avail(&_spi_tx_queue)) // re-enable this intr
          UCB0IE |= UCTXIE;  //re-enable this int
        else
          UCB0IE &= ~UCTXIE; 
    } else {
      UCB0IE &= ~UCTXIE; 
    }
        */
    break;
  default: break;
  }  
}
