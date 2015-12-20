#include "mrf_sys.h"
#include "mrf_debug.h"
#include "mrf_uart.h"

int mrf_uart_init_rx_state(I_F i_f,UART_CSTATE *rxstate){
  rxstate->state = S_START;
  rxstate->bindex = 0;
  rxstate->bnum = mrf_alloc_if(i_f);
  if (rxstate->bnum == _MRF_BUFFS){
    mrf_debug("mrf_uart_init: failed to alloc buff!");
    rxstate->buff = NULL;
    rxstate->errors = 1;
    return -1;
  }
  rxstate->buff = _mrf_buff_ptr(rxstate->bnum);
  rxstate->errors = 0;
  return 0;
}

int mrf_uart_init_tx_state(I_F i_f,UART_CSTATE *txstate){
  txstate->state = S_IDLE;
  txstate->buff = NULL;
  txstate->bnum = 0;

}

int mrf_uart_init(I_F i_f, IF_STATE *state){



}



// binary packet data on serial ports 
// intr handler for uart rx
int mrf_uart_rx_byte(uint8 rxbyte, UART_CSTATE *rxstate){ 
  switch (rxstate->state){
  case  S_START:
    if (rxbyte == _MRF_UART_PREAMBLE)
      rxstate->state = S_PREAMBLE_1;
    break;
  case  S_PREAMBLE_1:
    if (rxbyte == _MRF_UART_PREAMBLE)
      rxstate->state = S_LEN;
    else
      rxstate->state = S_START;
    break;
  case  S_LEN:
    if ((rxbyte <= _MRF_BUFFLEN) && (rxbyte >= sizeof(MRF_PKT_HDR))){
      rxstate->state = S_ADDR;
      rxstate->bindex = 0;
      rxstate->buff[rxstate->bindex++] = rxbyte;
    } else {
      rxstate->state = S_START;
    }
    break;
  case  S_ADDR:
    if (rxbyte == MRFID){
      rxstate->state = S_NETID;
      rxstate->buff[rxstate->bindex++] = rxbyte;
    } else {
      rxstate->state = S_START;
    }
    break;
  case  S_NETID:
    if (rxbyte == MRFNET){
      rxstate->state = S_HDR;
      rxstate->buff[rxstate->bindex++] = rxbyte;
    } else {
      rxstate->state = S_START;
    }
    break;    
  case  S_DATA:
    if(rxstate->bindex < rxstate->buff[0]){
      rxstate->buff[rxstate->bindex++] = rxbyte;
    }
    if (rxstate->bindex >= rxstate->buff[0]){
      // packet received
      //mrf_buff_loaded(rxstate->bnum);
      rxstate->state = S_START;
      return 1;
    }
    break;    
  default :
    rxstate->state = S_START;
  }
  return 0;
  //  UCA0IE |= UCRXIE;         // re-enable RX ready interrupt

}
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



inline uint8 mrf_uart_tx_complete(UART_CSTATE *txstate){
  if(txstate->state == S_IDLE)
    return 1;
  else
    return 0;
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
    return _MRF_UART_PREAMBLE;
  case  S_LEN:
    if ((txstate->buff[0] <= _MRF_BUFFLEN) && (txstate->buff[0] >= sizeof(MRF_PKT_HDR))){
      txstate->state  = S_DATA;
      (txstate->bindex)++;
      return txstate->buff[0];
    } else {
      txstate->state  = S_CSUM_MS;
      (txstate->errors)++;
      return 0;
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
    return txstate->csum / 256;
  case  S_CSUM_LS:
    txstate->state  = S_IDLE;
    return txstate->csum % 256;
  case  S_IDLE: // should not be called in this state
    (txstate->errors)++;
    return 0;
  default :
    txstate->state = S_IDLE;
  }
}

