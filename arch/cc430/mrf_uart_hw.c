
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


const MRF_IF_TYPE mrf_uart_cc_if = {
 tx_del : 4,
 funcs : { send : mrf_uart_send_cc
           init : mrf_uart_init_cc }
};



static int _tx_rdy_cnt;


static UART_RX_STATE rxstate;
static UART_RX_STATE txstate;




int mrf_uart_send_cc(I_F i_f, uint8 *buff){
  //UART_CSTATE  *txstate =  (UART_CSTATE*)sp;
  // MRF_IF *mif = mrf_if_ptr(i_f);
  if (txstate->state != 0) {
    mrf_debug("uart_if_send_func found if busy");
    return -1;
  }
  _tx_rdy_cnt = 0;
  txstate->buff = buff;
  txstate->len = buff[0];
  txstate->busy = 1;
  txstate->count = 0;
  UCA0IE |= UCTXIE;         // enable TX ready interrupt
  __bis_SR_register(GIE);
  return 0;
}

void rx_newpacket(){
  rxstate.bindex = 0;
}



int mrf_uart_init_cc(I_F i_f){

  mrf_uart_init_rx_state(&rxstate);  
  mrf_uart_init_tx_state(&txstate);

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
#ifdef LP_115200
  UCA0CTL1 |= UCSSEL__SMCLK;
  UCA0BRW =   72;       // trying 96              // 
  UCA0MCTL = UCBRS_7+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
#else
  UCA0CTL1 |= UCSSEL__SMCLK;
  //UCA0BR0 = 69;                           // 
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
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;        // enable RX interrupt 
  return 0;
}






//#define LP_9600
int _uart_rx_int_cnt;
int _uart_tx_int_cnt;


static inline void _tx_byte(uint8 chr){
  UCA0TXBUF = chr;     
}

static inline uint8  _rx_byte(){
  return UCA0RXBUF;
}

interrupt (USCI_A0_VECTOR) USCI_A0_ISR()
{
  switch(UCA0IV)
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    _uart_rx_int_cnt++;
    mrf_uart_rx_byte(_rx_byte(),&rxstate);
    UCA0IE |= UCRXIE;         // re-enable RX ready interrupt
    break;
  case 4:                                   // Vector 4 - TXIFG
    _uart_tx_int_cnt++;
    if (mrf_uart_tx_complete(txstate)){
      UCA0IE &= ~UCTXIE;  // disable this intr
    } else {
      _tx_byte(mrf_uart_tx_byte(txstate));
      UCA0IE |= UCTXIE;  //re-enable this intr
    }
    break;
  default: break;
  }  
}
