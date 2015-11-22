
#include "mrf_uart.h"
#include "mrf_sys.h"
#include "mrf_if.h"
#include "mrf_buff.h"
#include  <msp430.h>
#include <legacymsp430.h>

#define mrf_buff_loaded(buff)  mrf_buff_loaded_if(UART0,buff)
#define mrf_alloc() mrf_alloc_if(UART0)

int uart_if_send_func(I_F i_f, uint8 *buff);

const MRF_IF_TYPE uart_if_type = {
 tx_del : 4,
 send_func : uart_if_send_func
};


typedef struct{
  uint8 *buff;
  uint8 len;
  uint8 count;
  int8 busy;
  int8 errors;
  int  tx_st_cnt;
  int  tx_cmp_cnt;
  uint tx_bytes;
}TX_STATUS;


static uint8 _rx_bnum;

static uint8 *_rx_buff;
static uint8 *_tx_buff;

int uart_if_send_func(I_F i_f, uint8 *buff){
  MRF_IF *mif = mrf_if_ptr(i_f);
  if (_uart_tx_status.busy != 0) {
    mrf_debug("uart_if_send_func found if busy");
    return -1;
  }
  _tx_buff = buff;
  _tx_rdy_cnt = 0;

 
  _uart_tx_status.len = len;
  _uart_tx_status.busy = 1;
  _uart_tx_status.count = 0;
  UCA0IE |= UCTXIE;         // enable TX ready interrupt
  __bis_SR_register(GIE);
}

int mrf_uart_init(){

  _rx_bnum =  mrf_alloc();

  if (_rx_bnum == _MRF_BUFFS){
    mrf_debug("mrf_uart_init: failed to alloc buff!");
    _rx_buff = NULL;
    return -1;
  }

  _rx_buff = mrf_buff_ptr(bind);


  //dbg
  _tx_rdy_cnt =0 ;  
  rxstate.errors = 0;

  rxstate.state = S_PREAMBLE_0;
  rx_newpacket();
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


volatile TX_STATUS _uart_tx_status;

static int _tx_rdy_cnt;


static uint8 _rx_buff_index;



void uart_tx_string(uint8 *buffer,int maxlen){
  int i;
  for (i = 0 ; (i < maxlen ) && (buffer[i] != '\0');i++)
    {
      while (!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = buffer[i];
    }
  UCA0TXBUF = '\n';
  
}




int uart_tx_data_works(uint8 *buffer,int len){
  int i;
  uint8 code;
  uint8 cbuff[3];
  for (i = 0 ; (i < len ) ;i++)
    {
      sprintf(cbuff,"%02X",buffer[i]);
      while (!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = cbuff[0];
      while (!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = cbuff[1];
      while (!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = ' ';
    }
  UCA0TXBUF = '\n';
  return 0;
}


void _tx_noddy_chr(uint8 chr){
      while (!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = chr;
}

int uart_tx_data_nointr(uint8 *buffer,int len){
  int i,j;
  j =  _build_tx_buffer(buffer,len);
  for (i = 0 ; i < j  ;i++)
    {
      _tx_noddy_chr(_tx_buffer[i]);
    }
  //_tx_noddy_chr('\n');
  return 0;
}

int uart_rx_rdy(){
  return (_uart_tx_status.busy == 0);
}

int utd_cnt;
int utd_len;


void uart_error(){
    _uart_tx_status.errors++;
}

int uart_tx_rdy(){
  return (_uart_tx_status.busy == 0);

}

int uart_tx_data(uint8 *buffer,int len){
  utd_cnt++;
  _uart_tx_status.tx_st_cnt++;
  while (_uart_tx_status.busy != 0){
    __delay_cycles(100);
  }
  /*
  if (_uart_tx_status.busy != 0){
    return -1;
  }
  */
  _tx_rdy_cnt = 0;

 
  _uart_tx_status.len = len;
  _uart_tx_status.busy = 1;
  _uart_tx_status.count = 0;
  UCA0IE |= UCTXIE;         // enable TX ready interrupt
  __bis_SR_register(GIE);
  }
}
typedef enum _rstate {
  S_PREAMBLE_0 = 0,
  S_PREAMBLE_1 = 1,
  S_LEN        = 2,
  S_ADDR       = 3,
  S_DATA       = 4,
  S_CSUM       = 5
} RSTATE;

typedef struct {
  RSTATE state;
  int bindex;
  uint errors;
  uint empties;
} UART_RX_STATE;

static UART_RX_STATE rxstate;


//#define LP_9600
#define LP_115200
int _uart_rx_int_cnt;
int _uart_tx_int_cnt;

void rx_newpacket(){
  rxstate.bindex = 0;
}

static void _tx_byte(uint8 chr){
  UCA0TXBUF = chr;     
  _uart_tx_status.tx_bytes++;
}
static uint8  _rx_byte(){
  return UCA0RXBUF;
}
uint8 _uart_response[_XB_MAXPKTLEN];
uint8 tmp_resp[5] = {0xFF,0xFF,3,4,5};





// binary packet data on serial ports 
// intr handler for uart rx
static void _rx_ready(){
  uint8 c1 = _rx_byte();

  switch (rxstate.state){
  case  S_PREAMBLE_0:
    if (c1 == _MRF_UART_PREAMBLE)
      rxstate.state = S_PREAMBLE_1;
    break;
  case  S_PREAMBLE_1:
    if (c1 == _MRF_UART_PREAMBLE)
      rxstate.state = S_LEN;
    else
      rxstate.state = S_PREAMBLE_0;
    break;
  case  S_LEN:
    if ((c1 <= _MRF_BUFFLEN) && (c1 >= sizeof(MRF_PKT_HDR))){
      rxstate.state = S_ADDR;
      rxstate.bindex = 0; 
      buffer[rxstate.bindex++] = c1;
    } else {
      rxstate.state = S_PREAMBLE_0;
    }
    break;
  case  S_ADDR:
    if (c1 == _mrfid){
      rxstate.state = S_NETID;
      buffer[rxstate.bindex++] = c1;
    } else {
      rxstate.state = S_PREAMBLE_0;
    }
    break;
  case  S_NETID:
    if (c1 == MRFNET){
      rxstate.state = S_HDR;
      buffer[rxstate.bindex++] = c1;
    } else {
      rxstate.state = S_PREAMBLE_0;
    }
    break;    
  case  S_DATA:
    if(rxstate.bindex < buffer[0]){
      buffer[rxstate.bindex++] = c1;
    }

    if (rxstate.bindex >= buffer[0]){
      // packet received
      mrf_buff_loaded(bnum);
      rxstate.state = S_PREAMBLE_0
    }
    break;    
  default :
    rxstate.state = S_PREAMBLE_0;
  }

  UCA0IE |= UCRXIE;         // re-enable RX ready interrupt

}

static void _tx_ready(){
  _tx_rdy_cnt++;
  if (_uart_tx_status.count < _uart_tx_status.len) {
    _tx_byte( _tx_buffer[_uart_tx_status.count]);

    //    UCA0TXBUF = _tx_buffer[_uart_tx_status.count];
    _uart_tx_status.count++;
  }

  if (_uart_tx_status.count >= _uart_tx_status.len){
      _uart_tx_status.len = 0;
      _uart_tx_status.count = 0;
      _uart_tx_status.tx_cmp_cnt++;
      _uart_tx_status.busy = 0;
      UCA0IE &= ~UCTXIE; 
  }
  else{
    UCA0IE |= UCTXIE;         // re-enable TX ready interrupt
  }


}
interrupt (USCI_A0_VECTOR) USCI_A0_ISR()
{
  switch(UCA0IV)
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    _uart_rx_int_cnt++;
   _rx_ready();                             
    break;
  case 4:                                   // Vector 4 - TXIFG
    _uart_tx_int_cnt++;
   _tx_ready();                             
    break;
  default: break;
  }  
}
