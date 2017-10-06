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

#include "mrf_sys.h"
#include "mrf_debug.h"
#include "mrf_uart.h"
#include "mrf_arch.h"

int mrf_uart_init_rx_state(I_F i_f,UART_CSTATE *rxstate){
  rxstate->state = S_START;
  rxstate->bindex = 0;
  rxstate->cnt  = 0;
  rxstate->bnum = mrf_alloc_if(i_f);
  if (rxstate->bnum == _MRF_BUFFS){
    mrf_debug("%s","mrf_uart_init: failed to alloc buff!");
    rxstate->buff = NULL;
    rxstate->errors = 1;
    return -1;
  }
  rxstate->buff = _mrf_buff_ptr(rxstate->bnum);
  rxstate->errors = 0;
  rxstate->rxcsum = 0;
  return 0;
}

int mrf_uart_init_tx_state(I_F i_f,UART_CSTATE *txstate){
  txstate->state = S_IDLE;
  txstate->buff = NULL;
  txstate->bnum = 0;
  txstate->cnt  = 0;

}

/*
int mrf_uart_init(I_F i_f, IF_STATE *state){
}
*/


// binary packet data on serial ports 
// intr handler for uart rx

static uint8 _db22;
void _dbg_last(uint8 code){
  _db22 = code;
}
void _dbg_len(uint8 code){
  _db22 = code;
}
void _dbg_wrongid(uint8 code){
  _db22 = code;
}

void _dbg_csum(UART_CSTATE *rxstate){
}
void _dbg_preamble(uint8 code){
  _db22 = code;
}

uint8 rcsum[2];

uint8 preamble_errs;
int mrf_uart_rx_byte(uint8 rxbyte, UART_CSTATE *rxstate){ 
  switch (rxstate->state){
  case  S_START:
    if (rxbyte == _MRF_UART_PREAMBLE)
      rxstate->state = S_PREAMBLE_1;
    else
      mrf_debug("mrf_uart_rx_byte : still syncing looking for preamble 1 got 0x%02x\n",
                rxbyte);
    break;
  case  S_PREAMBLE_1:
    if (rxbyte == _MRF_UART_PREAMBLE_1){
      rxstate->state = S_LEN;
      rxstate->csum = 0;
      rxstate->rxcsum = 0;

    }
    else {
      rxstate->state = S_START;
      mrf_debug("mrf_uart_rx_byte : preamble sync error on preamble2 state got 0x%02x expected 0x%02x\n",
                rxbyte,_MRF_UART_PREAMBLE_1);
      _dbg_preamble(rxbyte);
    }
    break;
  case  S_LEN:
    //mrf_debug("mrf_uart_rx_byte : S_SLEN  recieved  0x%x  bindex %d rxstate %p\n",rxbyte, rxstate->bindex, rxstate);

    if ((rxbyte <= _MRF_BUFFLEN) && (rxbyte >= sizeof(MRF_PKT_HDR))){
      rxstate->state = S_ADDR;
      rxstate->bindex = 0;
      rxstate->buff[rxstate->bindex++] = rxbyte;
      rxstate->csum = rxbyte;
    } 
    /*
    else if (rxbyte != _MRF_UART_PREAMBLE){ // FIXME slight bodge that relies on preamble being reject above 
      
      _dbg_len(rxbyte);
      rxstate->state = S_START;
      }*/
    else {
      _dbg_len(rxbyte);
     mrf_debug("mrf_uart_rx_byte : len error   recieved  0x%x  bindex %d rxstate %p\n",rxbyte, rxstate->bindex, rxstate);
     rxstate->state = S_START;  // hmpff
    }
    break;
  case  S_ADDR:
      rxstate->state = S_NETID;
      rxstate->buff[rxstate->bindex++] = rxbyte;
      rxstate->csum += rxbyte;
      break;
  case  S_NETID:
    if (rxbyte == MRFNET){
      rxstate->state = S_DATA;
      rxstate->buff[rxstate->bindex++] = rxbyte;
      rxstate->csum += rxbyte;
    } else {
      mrf_debug("mrf_uart_rx_byte : netid error   recieved  0x%x  bindex %d rxstate %p\n",
                rxbyte, rxstate->bindex, rxstate);
      _dbg_wrongid(rxbyte);
      rxstate->state = S_START;
    }
    break;    
  case  S_DATA:
    if(rxstate->bindex < (rxstate->buff)[0]){
      rxstate->buff[rxstate->bindex++] = rxbyte;
      rxstate->csum += rxbyte;
    }

    if (rxstate->bindex == ((rxstate->buff)[0])){
      rxstate->state = S_CSUM_MS;
      _dbg_last(0x0d);

    }
    break;    
  case  S_CSUM_MS:
    rxstate->rxcsum = ((uint16)rxbyte << 8);
    rcsum[0] = rxbyte;
    rxstate->state = S_CSUM_LS;
    break;
  case  S_CSUM_LS:
    rxstate->state = S_START;
    rcsum[1] = rxbyte;
    rxstate->rxcsum |= (uint16)rxbyte ;

    if (rxstate->rxcsum == rxstate->csum){
      mrf_debug("mrf_uart_rx_byte : csum PASS    bindex %d rxstate %p expected 0x%x  recieved  0x%x  rcsum[0] 0x%02x rcsum[1] 0x%02x\n", rxstate->bindex,
                rxstate, rxstate->csum, rxstate->rxcsum, rcsum[0],rcsum[1]);
      return 1;

    }
    else {
      mrf_debug("mrf_uart_rx_byte : csum 2 error    bindex %d rxstate %p expected 0x%x  recieved  0x%x  rcsum[0] 0x%02x rcsum[1] 0x%02x\n", rxstate->bindex,
                rxstate, rxstate->csum, rxstate->rxcsum, rcsum[0],rcsum[1]);
#ifdef MRF_ARCH_lnx   
      _mrf_print_hex_buff(rxstate->buff,(uint16)(rxstate->buff)[0]);
#endif      
      _dbg_csum(rxstate);
    }
    break;
    
    
  default :
    rxstate->state = S_START;
  }
  return 0;
  //  UCA0IE |= UCRXIE;         // re-enable RX ready interrupt

}
/* TBD
int mrf_uart_to_buff(I_F i_f, uint8* inbuff, uint8 inlen, uint8 tobnum){
  UART_CSTATE rxstate;
  uint8 i;
  rxstate.state = S_START;
  rxstate.bindex = 0;
  rxstate.bnum = tobnum;
  rxstate.buff = _mrf_buff_ptr(rxstate.bnum);
  rxstate.errors = 0;  
  for ( i = 0 ; i < inlen ; i++){
    if(mrf_uart_rx_byte(inbuff[i], &rxstate)){
      return 0;
    }
  }
  return -1;
}
*/


inline uint8 mrf_uart_tx_complete(UART_CSTATE *txstate){
  if(txstate->state == S_IDLE)
    return 1;
  else
    return 0;
}

void _dbg_txlen(uint8 code){
  _db22 = code;
}
void _dbg_txcsum(uint8 code){
  _db22 = code;
}

// return next tx byte
uint8 mrf_uart_tx_byte(UART_CSTATE *txstate){
  uint8 tmp;
  switch (txstate->state){
  case  S_START:
    txstate->state = S_PREAMBLE_1;
    return _MRF_UART_PREAMBLE;
  case  S_PREAMBLE_1:
    txstate->state  = S_LEN;
    txstate->csum   = 0;
    txstate->bindex = 0;
    return _MRF_UART_PREAMBLE_1;
  case  S_LEN:
    if ((txstate->buff[0] <= _MRF_BUFFLEN) && (txstate->buff[0] >= sizeof(MRF_PKT_HDR))){
      txstate->state  = S_DATA;
      (txstate->bindex)++;
      txstate->csum = txstate->buff[0];
      return txstate->buff[0];
    } else {
      mrf_debug("\nmrf_uart_tx_byte S_LEN got buff[0] %x\n",txstate->buff[0]);
      txstate->state  = S_CSUM_MS;
      _dbg_txlen(txstate->buff[0]);
      (txstate->errors)++;
      return 0;  // 0 payload means it was deemed messed up here
    }
    return 0;
  case  S_DATA:
    tmp = txstate->buff[txstate->bindex++];
    txstate->csum += tmp;
    if(txstate->bindex >= txstate->buff[0]){
      txstate->state  = S_CSUM_MS;
    }
    return tmp;     
  case  S_CSUM_MS:
    txstate->state  = S_CSUM_LS;
    _dbg_txcsum((txstate->csum) / 256);
    return (txstate->csum) / 256;
  case  S_CSUM_LS:
    txstate->state  = S_IDLE;
    return (txstate->csum) % 256;
  case  S_IDLE: // should not be called in this state
    (txstate->errors)++;
    return 0;
  default :
    txstate->state = S_IDLE;
  }
}

