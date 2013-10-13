#include "xbus.h"
#include "cc430x513x.h"
#include "RF1A.h"
#include "squeue.h"
#include "stddef.h"
#include "adc.h"
#include "xb_sys.h"

#ifndef XB_ID
#error "XB_ID must be defined - use -DXB_ID=<ID> on GCC command line "
#endif

#define _XB_MAXPKTLEN  XB_MAXPKTLEN

const int SIZE_TD = sizeof(XB_PKT_TIMEDATE);

#define  CRC_OK             (BIT7)          // CRC_OK bit 
#define  PATABLE_VAL        (0x51)          // 0 dBm output 
extern RF_SETTINGS rfSettings;
static unsigned char RFRXCSUM,RFRXRSSI,HUBRSSI,HUBLQI;



void xbus_clear_stats();
unsigned char GetRF1ASTATB(void);

#define XB_ACK_RXON 0x80

typedef enum {
  XB_ACK_SEG,
  XB_ACK_REC,
  XB_ACK_RTRY,
  XB_ACK_ERROR} XB_ACK_CODE;

typedef struct  {
  XB_STATE state;
  XB_STATE nstate;  // next state
  XB_RX_BUFF *ackrxbuff;
  XB_ACK_CODE ackcode;
  uint8 rx_last_id;
  uint8 rx_last_src;
  uint8 tx_id;
  uint8 tx_retrycnt;
  XB_TX_STATUS tx_status; 
  uint8 rx_on;  // default xbus mode , when not transmitting
} XB_STATUS;

static volatile XB_STATUS _xb_status;


volatile XB_RX_BUFF _xb_rx_buff;


typedef struct  __attribute__ ((packed)) {
 uint8 len;
 XB_PKT_HDR hdr;
 XB_PKT_TIMEDATE td; 
 uint8 xdata[64];  
} XB_TX_BUFF;



static volatile XB_TX_BUFF _xb_tx_buff;

typedef struct  __attribute__ ((packed)){
  uint8  len;
  XB_PKT_HDR hdr;
  XB_PKT_TIMEDATE td; 
} XB_ACK_BUFF;

typedef struct  __attribute__ ((packed)){
  uint8  pkt_len;
  uint8 dest;
  XB_PKT_HDR hdr;
  XB_PKT_TIMEDATE td; 
  uint8 ack_data[sizeof(XB_PKT_STATS)];
} XB_FULLACK_BUFF;


static volatile XB_FULLACK_BUFF _xb_ack_buff;

//static volatile uint8 ack_data[sizeof(XB_PKT_STATS)];  // enough to take stats

static unsigned char rx_buffer_Length = 0;

static volatile XB_STATE _xb_state ;
static const uint8 XBUS_ID=XB_ID;
static const uint8 XBUS_NET=XB_NET;
static uint8 _xb_tx_count;

#define XB_MAX_DEVICES 32

static uint8 _wakeup_list[XB_MAX_DEVICES];


XB_PKT_STATS  _xb_stats;
volatile static XB_TX_STATUS _tx_status;
#define _XB_RXQ_DEPTH 4

/* new squeue implementation */

SQUEUE _xb_rx_q;
static uint8 _xb_rx_q_buff[_XB_RXQ_DEPTH * _XB_MAXPKTLEN];
static uint8 _xb_rx_q_blen[_XB_RXQ_DEPTH];


// temp debug
static volatile int _rx_push_cnt,_rx_pop_cnt;
static int pop_err,push_err;
static uint8 _xb_tick_enable;

#ifdef XB_MON_ID
uint8 _xb_mon_last_msgid;
#endif

// info structure

const XB_PKT_DEVICE_INFO device_info  = {XBDEVNAME ,"1", SVN };

volatile XB_PKT_TIMEDATE xb_timedate;
#include "xbus_cmds.h"
void _xb_receive_enable(void);

void xbus_start_tick(){
  rtc_ps0_enable(_xb_tick);

}


 void  _xb_ps0_enable(VFUNCPTR func){
  _xb_tick_enable = 1;
#ifndef XB_HUB
  rtc_ps0_enable(func);
#endif
}

 void  _xb_ps0_disable(){
  _xb_tick_enable = 0;
#ifndef XB_HUB
  if((xq_tx_data_avail() == 0) && xbus_can_sleep())
    rtc_ps0_disable();
#endif
}

static uint8  _xb_can_sleep;

uint8 xbus_can_sleep(void){

#ifdef XB_HUB
  return 0;
#else
  return _xb_can_sleep ;
#endif
}



int  _rx_queue_push(){
#ifdef XB_UART
  return xq_utx_push((uint8 *)(&_xb_rx_buff),_xb_rx_buff.len+1+2); // include RSSI and LQI
#else
  return xq_app_push((uint8 *)(&_xb_rx_buff),_xb_rx_buff.len+1+2);
#endif
}
int  _rx_queue_full(){
#ifdef XB_UART
  return xq_utx_full();
#else
  return xq_app_full();
#endif
} 

void _response_clrstats(){
  xbus_clear_stats();
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) - 1; // len field not included in length

}

void _response_getstats(){
  int i;
  uint8 *statsptr = (uint8 *)&_xb_stats;
  uint8 *destptr = (uint8 *)&_xb_ack_buff.td; // td not used for stats reporting
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF)  +sizeof(XB_PKT_STATS) - 1; // len field not included in length
  for ( i = 0 ; i < sizeof(XB_PKT_STATS) ; i++){
    destptr[i] = statsptr[i];
    //_xb_ack_buff.ack_data[i] = statsptr[i];
  }
  // _xb_tx_ack();    
}
void _response_gettime(){
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) + sizeof(XB_PKT_TIMEDATE) - 1; // len field not included in length
  rtc_get((TIMEDATE *) &_xb_ack_buff.td);
}
void _response_settime(){
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) - 1; // len field not included in length
  rtc_set((TIMEDATE *)&_xb_rx_buff.td);
}


void _response_ping(){
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) - 1; // len field not included in length
  // _xb_tx_ack();    
}

void _response_info(){

  int i;
  uint8 *statsptr = (uint8 *)&device_info;
  uint8 *destptr = (uint8 *)&_xb_ack_buff.td; // td not used for stats reporting
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF)  +sizeof(XB_PKT_DEVICE_INFO) - 1; // len field not included in length
  for ( i = 0 ; i < sizeof(XB_PKT_STATS) ; i++){
    destptr[i] = statsptr[i];
    //_xb_ack_buff.ack_data[i] = statsptr[i];
   }
 
}

void _response_sleep(){
  _xb_can_sleep = 1;
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) - 1; // len field not included in length
  // _xb_tx_ack();    
}
void _response_adc1(){
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) - 1; // len field not included in length
  // _xb_tx_ack();  
  adc_temp(1);
}
void _response_adc2(){
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) - 1; // len field not included in length
  // _xb_tx_ack();  
  adc_supply(1);
}



VFUNCPTR  _rx_immediate_response() { // see if rx message can be replied immediately

}

static volatile unsigned int _xb_tick_count,  _xb_htick_count;

void _xb_tick();
  


#define XB_ACK_MAXWAIT 3
#define XB_MAX_RETRIES 3

int _xb_ack_timer;
int _xb_retry_counter;
int _xb_retry_total;
int _xb_tx_fails;

// temp debug
uint8 RF1CTL1,RSTAT;
void _dbg99(){

}
void _xb_st_waitsack(){
  // xb_receive_ack();

  _xb_receive_enable();
  _xb_state = XB_ST_WAITSACK;
  
  _xb_ps0_disable();

  _xb_tick_count = 0;
  RSTAT = GetRF1ASTATB();

  _xb_ps0_enable(_xb_tick);
  _dbg99();

}

int _xb_receiving(){
  switch(_xb_state)
    {
    case XB_ST_RX: return 1;
    case XB_ST_WAITSACK: return 1;
    case XB_ST_WAITUACK: return 1;
    default: return 0;
    }
  return 0;
}

int _xb_transmitting(){
  switch(_xb_state)
    {
    case XB_ST_TX: return 1;
    case XB_ST_ACK: return 1;
    default: return 0;
    }
  return 0;
}
XB_STATE _xb_curr_state(){
  return _xb_state;
}
//transmit buffer
/*
static uint8 __attribute__ ((aligned (2))) _xb_tx_buff.pkt_len;
static uint8 _xb_tx_buff.dest;
static XB_PKT_HDR _xb_tx_buff.hdr;
static XB_PKT_TIMEDATE _xb_tx_buff.td; 
static uint8 _xb_tx_buff.buff[64];  
*/

void xb_active_on_lpm3(void){
  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled 
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L; 
  PMMCTL0_H = 0x00; 
}


void _xb_init_radio(void)
{
  // xb_active_on_lpm3();
  WriteRfSettings(&rfSettings);
  
  WriteSinglePATable(PATABLE_VAL);

}

int xbus_is_idle(){
  return _xb_tx_rdy() && !xq_tx_data_avail();
}


void xb_receive_on(void)
{  

  _xb_state = XB_ST_RX;
  _xb_receive_enable();
           
}

void _xb_receive_disable(void)
{
  RF1AIE &= ~BIT9;                          // Disable RX interrupts
  RF1AIFG &= ~BIT9;                         // Clear pending IFG

  // It is possible that ReceiveOff is called while radio is receiving a packet.
  // Therefore, it is necessary to flush the RX FIFO after issuing IDLE strobe 
  // such that the RXFIFO is empty prior to receiving a packet.
  Strobe( RF_SIDLE );
  Strobe( RF_SFRX  );    
 }

void xb_receive_off(void)
{
  _xb_receive_disable();
  _xb_state = XB_ST_IDLE;
                 
}
// *****************************************************************************
// @fn          ResetRadioCore
// @brief       Reset the radio core using RF_SRES command
// @param       none
// @return      none
// *****************************************************************************
unsigned char RfRxStat(void){
  return Strobe(RF_RXSTAT | RF_SNOP);    
}
unsigned char RfTxStat(void){
  return Strobe(RF_TXSTAT | RF_SNOP);    
}



unsigned char GetRF1ASTATB(void){
 return Strobe(RF_SNOP);    
}

int _TIMEXXX;
uint8 _STATXXX;


uint8 xbus_copy_stats(uint8 *buffer){
  uint8 i;
  uint8 *ptr = (uint8 *)&_xb_stats;
  for ( i = 0 ; i < sizeof(XB_PKT_STATS) ; i++){
    buffer[i] = ptr[i];
  }
  return i;

}
void xbus_clear_stats(){
  int i;
  uint8 *ptr = (uint8 *)&_xb_stats;
  for ( i = 0 ; i < sizeof(XB_PKT_STATS) ; i++){
    ptr[i] = 0;
  }

  /*
  _xb_stats.rx_pkts = 0;
  _xb_stats.tx_pkts = 0;
  _xb_stats.rx_crc_errs = 0;
  _xb_stats.tx_fails = 0;
  _xb_stats.tx_retries = 0;
  _xb_stats.rx_overruns = 0;
  _xb_stats.rx_unexp_ack = 0;
  */
}

uint8 xbus_copy_info(uint8 *buffer){
  uint8 i;
  uint8 *ptr = (uint8 *)&device_info;
  for ( i = 0 ; i < sizeof(XB_PKT_DEVICE_INFO) ; i++){
    buffer[i] = ptr[i];
  }
  return i;

}
static volatile int _setup_ack_cnt,_tx_ack_cnt;

void _xbus_idle(){
  _xb_state = XB_ST_IDLE;
  Strobe( RF_SIDLE );

}




 uint8 _xb_device_is_awake(uint8 device){
  int i;
  for ( i = 0 ; i < XB_MAX_DEVICES ; i++){
    if(_wakeup_list[i] == device)   
      return 1;
  }
  return 0;
}

void xb_sleep_device(uint8 device){
  int i;
  for ( i = 0 ; i < XB_MAX_DEVICES ; i++){
    if(_wakeup_list[i] == device){      
      _wakeup_list[i] = 0;
      break;
    }
  }  
}

void xb_wake_device(uint8 device){
  int i;

  if (_xb_device_is_awake(device))
    return;
  for ( i = 0 ; i < XB_MAX_DEVICES ; i++){
    if(_wakeup_list[i] == 0){      
      _wakeup_list[i] = device;
      break;
    }
  }  
}


void xbus_init(){

  int i;
  for ( i = 0 ; i < XB_MAX_DEVICES ; i++){
    _wakeup_list[i] = 0;

  }
  _xb_state = XB_ST_IDLE;
  _xb_retry_total = 0; 
  _xb_tx_fails = 0; 
  // init tx buff constants
  _xb_tx_buff.hdr.netid = XB_NET;
  _xb_tx_buff.hdr.hsrc = XB_ID;     
  _xb_tx_buff.hdr.usrc = XB_ID;     
  _xb_tx_buff.hdr.msgid = 0;
  // init ack buff
  _xb_ack_buff.pkt_len=sizeof(XB_ACK_BUFF) - 1; // len field not included in length
  _xb_ack_buff.hdr.netid = XB_NET;
  _xb_ack_buff.hdr.hsrc = XB_ID;     
  _xb_ack_buff.hdr.usrc = XB_ID;     
  _xb_ack_buff.hdr.type = 0;
 
  // init stats
  xbus_clear_stats();

  // init q
  xqueues_init();
  // setup RTC PSO
  rtc_init();


  _xb_tick_count = 0;
  _xb_htick_count = 0;
  rtc_ps0_init(DIV32,_xb_tick);  // 1KHz tick
  _xb_tick_enable = 0;


#ifdef XB_UART
  uart_init();
#endif

#ifdef XB_MON_ID
  _xb_mon_last_msgid = 0;
#endif 
  // debug vars
  _setup_ack_cnt = 0;
  _tx_ack_cnt = 0;
  ResetRadioCore();
  _xb_init_radio();
 
#ifdef XB_HUB
  // hub needs continous tick
  rtc_ps0_enable(_xb_tick);
#endif

}


int  _xb_setup_ack(){  // have short turn round delay before tx
  _xb_ps0_disable();
  _xb_state = XB_ST_ACKDELAY;

  _setup_ack_cnt++;
  Strobe( RF_SIDLE );
  Strobe( RF_SFTX  );    
  Strobe( RF_SFRX  );  
  
  _xb_tick_count = 0;
  _xb_ps0_enable(_xb_tick);


}

int  _xb_tx_ack(){
  _tx_ack_cnt++;
  _xb_ack_buff.hdr.msgid = _xb_rx_buff.hdr.msgid;  // ack returns received msgid 
  _xb_ack_buff.hdr.hdest = _xb_rx_buff.hdr.hsrc;  // send ack to source 
  _xb_ack_buff.hdr.udest = _xb_rx_buff.hdr.hsrc;  // send ack to source 


  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt  
  
  //Strobe( RF_STX );                         // Strobe STX   
  Strobe( RF_SIDLE );
  Strobe( RF_SFTX  );  
  Strobe( RF_SNOP );                         // Strobe STX   
  
  WriteBurstReg(RF_TXFIFOWR, (uint8 *)&_xb_ack_buff,_xb_ack_buff.pkt_len + 1);     
  Strobe( RF_STX );                         // Strobe STX   
  return 0; 
}

// this is ugly 
uint8 _xbus_msg_xdata_len(uint8 type){
  return 0;
}

int _xb_tx_rdy(){
  switch(_xb_state){
  case XB_ST_IDLE: return 1; break;
  case XB_ST_RX  : return 1; break;
  default        : return 0;
    }  
}

// _xb_tick runs at 1kHz  when enabled
// processes xbus transmit
// hubs/routers run continously and manages unloading both
// transmit queues
/*
  __attribute__ ((volatile)) void _no_ack(){
}
*/
// manage bus message transmission
// unload queues and send messages
void _xb_hub_tick(){
  uint8 *src,addr,type,len;
  if (xq_tx_data_avail() && _xb_tx_rdy()){
    src = (uint8 *)xq_tx_head();
    addr = src[0];
    type = src[1];
    _xbus_transmit(addr,type,src+2);
    xq_tx_pop(NULL);
  }

#ifdef XB_UART
  if (xq_utx_data_avail() && uart_tx_rdy()){        
    src = (uint8 *)xq_utx_head();
    len = xq_utx_head_len();
    uart_tx_data(src,len);
    xq_utx_pop(NULL);

  }
#endif
}

#define TX_MAX_RETRIES 3
#define _WAIT_LIM 50
#define _ACK_DELAY 10
#define _TX_LIM 15
#define _HTICK  10

void _xb_tick(){
 
  if (_xb_tick_enable) {
    _xb_tick_count++;
  
    if (_xb_state == XB_ST_TX){
      if(_xb_tick_count ==  _TX_LIM){
	if (_xb_tx_count < TX_MAX_RETRIES){
	  _xb_tx_count++;
	  _xb_stats.tx_retries++;
	  _xb_stats.tx_no_eop++;
	  _xb_tick_count = 0;
	  _xbus_send_txbuff();
	} 
	else{
	  _xb_ps0_disable();
	  // _no_ack();
	  _xb_stats.tx_fails++;
	  _tx_status = XB_TX_ABORTED;
	  _xb_tick_count = 0;

	  if ( xbus_can_sleep())
	    _xbus_idle();
	  else
	    xb_receive_on();

	} 
      }
    }
    else if (_xb_state == XB_ST_ACKDELAY){
      if(_xb_tick_count ==  _ACK_DELAY){
	_xb_ps0_disable();
	_xb_state = XB_ST_ACK;
	_xb_tick_count = 0;
	_xb_tx_ack();
      }
    }
    else if(_xb_state == XB_ST_WAITSACK){
      if(_xb_tick_count ==  _WAIT_LIM){
	if (_xb_tx_count < TX_MAX_RETRIES){
	  _xb_tx_count++;
	  _xb_stats.tx_retries++;
	  _xb_tick_count = 0;
	  _xbus_send_txbuff();
	} 
	else{
	  _xb_ps0_disable();
	  // _no_ack();
	  _xb_stats.tx_fails++;
	  _tx_status = XB_TX_NOACK;
	  _xb_tick_count = 0;

	  if ( xbus_can_sleep())
	    _xbus_idle();
	  else
	    xb_receive_on();




	}
      }      
    }
    else {
      _xb_ps0_disable();   
      _xb_tick_count = 0;
      _xb_stats.tick_err++;

    }

  }
  _xb_htick_count++;
  if (_xb_htick_count == _HTICK){
    _xb_htick_count = 0;
    _xb_hub_tick();

  }

#ifndef XB_HUB
  // turn off tick for periphs when not needed 
  if ( (_xb_tick_enable == 0))
    _xb_ps0_disable();

#endif
}


void burstwritten(){
  RSTAT = GetRF1ASTATB();
  __nop();
}



volatile int dummy;


uint8 _xb_is_ack(){
  if (_xb_rx_buff.hdr.msgid != _xb_tx_buff.hdr.msgid)
    return 0;
  if (_xb_rx_buff.hdr.hsrc != _xb_tx_buff.hdr.hdest)
    return 0;
  return 1;
}

typedef enum {
  XB_TX_SUCCESS,
  XB_TX_RETRY,
  XB_TX_FAIL} XB_TX_RESULT;


/*segment ack check*/
int _xb_check_sack(XB_RX_BUFF *rx_buff){

  if (rx_buff->hdr.msgid != _xb_tx_buff.hdr.msgid)
    return 0;
  if (rx_buff->hdr.hsrc != _xb_tx_buff.hdr.hdest)
    return 0;
  
  return 1;
}

int _xb_check_uack(XB_RX_BUFF *rx_buff){
 
  if (rx_buff->hdr.usrc != _xb_tx_buff.hdr.udest)
    return 0;
   if (rx_buff->hdr.udest != _xb_tx_buff.hdr.usrc)
     return 0;

  return 1;
}



static int xb_debug(int val);

XB_TX_STATUS tx_stat;


volatile int XBDBG;
volatile int XBDBGdumm;

static int xb_debug(int val){
  XBDBGdumm++;
  XBDBG = val;
}
uint8 _xb_q_buff[64];
int xbus_queue_msg(uint8 type,uint8 *xdata){
  int i;
  _xb_q_buff[0] = XB_HUBADDR;
  _xb_q_buff[1] = type;
  for ( i = 0 ; i < _xbus_msg_xdata_len( type) ; i++)
    _xb_q_buff[2+i] = xdata[i];
  
  xq_tx_push(_xb_q_buff,_xbus_msg_xdata_len( type)+2);
  xbus_start_tick();
  return 0;
}


XB_TX_STATUS _xbus_transmit(uint8 dest, uint8 type,uint8 *xdata){
  uint8 i,offs,xdlen,pktlen,*dptr;
  xb_receive_off();
  _xb_tx_count = 0; 
 
  xdlen = _xbus_msg_xdata_len( type);
  pktlen = 1 + sizeof(XB_PKT_HDR) + xdlen; // add 1 for addr byte
  offs = 2; // 0,1 reserved for length,addr
  _xb_tx_buff.hdr.hdest = dest;
  _xb_tx_buff.hdr.udest = dest;
  _xb_tx_buff.hdr.type = type;
  _xb_tx_buff.hdr.msgid++;
 
  dptr = (uint8 *)&_xb_tx_buff.td;
 
  _xb_tx_buff.len = pktlen;

  // copy xdata if required

  for (i = 0 ; i < xdlen ; i++)
    *(dptr + i) = *(xdata + i);
  // _xb_state = XB_ST_TX;
  _tx_status = XB_TX_BUSY;
  _xbus_send_txbuff();
  //RF1CTL1 = RF1AIFCTL1;


  //RF1CTL1 = RF1AIFCTL1;
  
  //RSTAT = GetRF1ASTATB();
  burstwritten();
  /*
  while ( _tx_status == XB_TX_BUSY){
    __delay_cycles(100);
  }
  return _tx_status;
  */
  return 0;
  //  return  _xb_tx_buff.hdr.msgid;
} 

// re-transmit txbuff

void wait_tx_rdy(){

  while(RfTxStat() & 0x80)
    __delay_cycles(100);
}

int  _xbus_send_txbuff(){
  // wait for radio to be ready
  //  wait_tx_rdy();


  _xb_state = XB_ST_TX;
  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt  
 
  // P1OUT |= 0x01; // turn LED on 

  RF1CTL1 = RF1AIFCTL1;
  _xb_tick_count = 0;
  //  _xb_ack_timer = 0;
  _xb_hw_wr_tx_fifo(_xb_tx_buff.len, (uint8 *)&_xb_tx_buff.hdr);     

  Strobe( RF_STX );                         // Strobe STX  
  _xb_ps0_enable(_xb_tick);

  return 0;
 

}


XB_TX_STATUS xbus_despatch(uint8 dest, uint8 type,uint8 *xdata){
  do {
    __delay_cycles(1000);
  }while(_xb_tx_rdy() == 0);

  // try delivery
  _xbus_transmit(dest,type,xdata);
  // this is rubbish
  return XB_TX_DELIVERED;
  // return _xbus_main();

  
}


static unsigned int RF1AIVREG;

static XB_STATE dbst;
void dbg1(XB_STATE st){
  dbst = st;
}

void dbg_ack_txed(XB_STATE st){
  dbst = st;
}

void _chk_isr(int cnt,XB_STATE st,uint8 stat){
  

}
void _got_ack(){

}

// only hubs and routers  should do much here
void _xb_proc_ack(){

}


XB_STATE dbgst;
void    _dbg100(XB_STATE st){
}
void    _dbg101(XB_STATE st){
}
void mon_dbg(){
  __delay_cycles(1);
}

RX_PKT_INFO rx_pkt_info;


// called by xb_hw.c interrupt handlers

int xb_process_packet(uint8 *buff)
{
  int len,result;
  uint8 pkt_code,pkt_vflags;
  XB_CMD_FUNC cmd_func;
  uint16 cflags;

  

#ifdef XB_MODE_SNIFFER  
  
  // sniffer gets all packets        
  if (_rx_queue_full())
    _xb_stats.rx_overruns++;
  else{              
    _rx_queue_push();
    _xb_stats.rx_pkts++;    
  }
  xb_receive_on();          
#else
  // Check packet belongs to our network
  // Check the CRC results
  //if(xbst == XB_ST_WAITACK)
  xb_debug(0x55);
  if(rx_pkt_info.csumok){
    if ( _xb_rx_buff.hdr.netid == XB_NET){
      //P1OUT |= BIT0;                    // Toggle LED1             
      //dbg1(xbst);
#ifdef XB_MON_ID
      mon_dbg();
      if((_xb_rx_buff.hdr.source == XB_MON_ID) && (_xb_rx_buff.hdr.type == XB_MON_TYPE) && (_xb_mon_last_msgid != _xb_rx_buff.hdr.msgid)) {
	_xb_mon_last_msgid = _xb_rx_buff.hdr.msgid;
	_xb_stats.rx_pkts++;
	if (_rx_queue_full()){
	  _xb_stats.rx_overruns++;
	}
	else { // pp
	  _rx_queue_push();
	  _xb_stats.rx_pkts++; 
	}

      }
      xb_receive_on();   

#else		
      if ( _xb_rx_buff.hdr.udest == XB_ID ) {
	// process command if we are udest
	pkt_code = _xb_rx_buff.hdr.type & XB_CODE_MASK;
	pkt_vflags = _xb_rx_buff.hdr.type & XB_VFLAG_MASK;
	      
	if ( pkt_code  >= XB_NUM_CMDS)
	  return -1;

	cmd_func = xb_cmds[pkt_code].func;
	     
	cflags =  xb_cmds[pkt_code].cflags;
	      
	// run command func now if XB_CFLG_INTR and func defined
	_xb_status.nstate = XB_ST_NONE;

	      
	if ((cflags & XB_CFLG_INTR) && ( cmd_func != NULL))
	  (*cmd_func)(_xb_rx_buff.hdr.type,&_xb_rx_buff);


      } else {
	// relay message 

      }
	      

      if ( (cflags & XB_CFLG_NACK ) == 0 )  // must ack message
	{
		  
	  _xb_status.ackrxbuff = &_xb_rx_buff;
	  if ( _xb_rx_buff.hdr.udest == XB_ID )		    
	    _xb_status.ackcode = XB_ACK_REC;
	  else if ( _xb_rx_buff.hdr.hdest == XB_ID )
	    _xb_status.ackcode = XB_ACK_SEG;
	  else
	    _xb_status.ackcode = XB_ACK_ERROR;
 

	  if (_xb_device_is_awake(_xb_rx_buff.hdr.hsrc))
	    _xb_status.ackcode |= XB_ACK_RXON;
	  if ( xb_cmds[pkt_code].data != NULL){
	    // return data in ack
	    
	}
	  _xb_setup_ack();
	}	     		    
    } else {  // got packet for different network TODO should be used as a channel congestion indicator
      _xb_stats.rx_net_errs++;
    }
  } else {  // csum error	   
    _xb_stats.rx_crc_errs++; 
  }
  if(xbus_can_sleep())
    _xbus_idle();
  else
    xb_receive_on(); 	 
#endif 

#endif
}


              
int xb_tx_intr(){
  xb_hw_disable_tx_eop();
	// _dbg100(xbst);
        //  _xb_state = XB_ST_IDLE;
        // P3OUT &= ~BIT6;                     // Turn off LED after Transmit 
  if (_xb_state ==  XB_ST_TX){
    _xb_state = XB_ST_WAITSACK;
    RF1AIES |= BIT9;                          // Falling edge of RFIFG9
    RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
    RF1AIE  |= BIT9;                          // Enable the interrupt 
  
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  // _xb_state = XB_ST_RX;
  // Strobe( RF_SFRX  );    
    Strobe(RF_SIDLE );
    Strobe(RF_SFRX);
    Strobe(RF_SRX );        
    _xb_ps0_disable();
    _xb_tick_count = 0;
    _xb_ps0_enable(_xb_tick);
  }
  else if(_xb_state ==  XB_ST_ACK){ 
    _xb_stats.tx_acks++;
          // must be ack state
          // go back to receive mode
    if (xbus_can_sleep())
    _xbus_idle();
  else
    xb_receive_on(); 	 
  }else { // some cockup
    _xb_stats.fatal++;	  	  
  }
        //        Strobe( RF_SRX );                      

        // xb_receive_on(); // desperate measures

}
// sys built in command handlers - run on reception of associated cmd packet - see xbus_cmds.h

XB_CMD_RES xb_task_ack(XB_CMD_CODE cmd,XB_RX_BUFF *buff){
  if ((_xb_status.state != XB_ST_WAITSACK) && (_xb_status.state != XB_ST_WAITUACK))
    {
      _xb_stats.rx_unexp_ack++;
      return XB_CMD_RES_ERROR;
    }
  

  if (_xb_check_sack(buff)){    
    _xb_status.tx_status = XB_TX_POSTED;    
    _xb_status.nstate = XB_ST_WAITUACK;
    _xb_status.ackrxbuff = buff;
    _xb_status.ackcode = XB_ACK_SEG;   
    if( _xb_check_uack(buff))      
      _xb_status.tx_status = XB_TX_DELIVERED;     
    
    if ( _xb_device_is_awake(buff->hdr.hsrc))
      _xb_status.ackcode |= XB_ACK_RXON;


    return XB_CMD_RES_ACCEPTED;
  }

  return XB_CMD_RES_IGNORE;
}

XB_CMD_RES xb_task_sleep(XB_CMD_CODE cmd,XB_RX_BUFF *buff){
  return 0;
}

XB_CMD_RES xb_task_time(XB_CMD_CODE cmd,XB_RX_BUFF *buff){

  return 0;
}

