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
#include <mrf_debug.h>


#include "mrf_spi.h"
#include  <msp430.h>
#include <legacymsp430.h>
#include "cc430f5137.h"

#include "mrf_pinmacros.h"

#define NUM_RELAY_CHANNELS 8
#include "mrf_relays.h"

// all gpio pins to TI EVM 


#define _MR_PORT P2
#define _MR_BIT  0


#define _START_PORT P2
#define _START_BIT  1

#define _CS_PORT P2
#define _CS_BIT  2

//#define _RESET_PORT P2
//#define _RESET_BIT  3

#define _DRDY_PORT P2

#ifndef _DRDY_BIT 
 #define _DRDY_BIT 3
#endif


// ADS1148 reg offsets

#define MUX0_OFFS    0
#define VBIAS_OFFS   1
#define MUX1_OFFS    2
#define SYS0_OFFS    3
#define IDAC0_OFFS   0xa
#define IDAC1_OFFS   0xb
#define GPIOCFG_OFFS 0xc
#define GPIODIR_OFFS 0xd


volatile int dbg22;

volatile uint8 dbg_u8;
volatile uint8 _rxcnt;
static uint8 sampling, sample_stop_flg;

void debfunc(uint8 data){
  dbg_u8 = data;
  _rxcnt += 1;
}


volatile static __attribute__((noinline)) uint8 toggle_cs() {
  PINLOW(CS);
  PINHIGH(CS);
}

uint8 _dbg_rxdata[4];

// these must retain ability to be tweaked - so non constant

// all units in milliohms
static uint32_t _ref_r;  // the resistor value between ref+ and ref
static uint32_t _ref_i;  // the inline resistance with the PT1000 - at least 47 ohm + lead resistance with EVM

static uint16_t _last_reading[MAX_RTDS];

uint16 ads1148_data(){
  /*
  while(mrf_spi_tx_data_avail()){  // block until queue cleared
    __delay_cycles(50);
  }
  */
  uint8  b1 = 0x12;  // read data once
  uint16 rv = 0 ;
  uint16 lc = 0;
  uint8  i = 0;
  _rxcnt = 0 ;
  //PINHIGH(START);  // leave continuous sampling
//toggle_cs();
  PINLOW(CS);
  //__delay_cycles(10);
  mrf_spi_flush_rx(); 
  /*  
  while((lc < 100) && (INPUTVAL(DRDY))){
    lc++;
    __delay_cycles(10);
    
  }
  */
  //if (lc == 100){
  //  return 0xeeee;
  //}
  //mrf_spi_tx(b1);
  mrf_spi_tx(0xff); // NOP
  mrf_spi_tx(0xff); // NOP
  //b1 = mrf_spi_rx();  // discard first
  //_dbg_rxdata[i++] = b1;
  //debfunc(b1);
  b1 = mrf_spi_rx();  // get second - msb of result
  rv += (b1 << 8);
  _dbg_rxdata[i++] = b1;
  debfunc(b1);
  b1 = mrf_spi_rx();  // get third  - lsb of result
  rv += b1;
  _dbg_rxdata[i++] = b1;

  
  //PINLOW(START);  // leave continuous sampling
  //PINHIGH(CS);

  return rv;
}

uint8 ads1148_read(uint8 reg){
  /*
  while(mrf_spi_tx_data_avail()){  // block until queue cleared
    __delay_cycles(50);
  }
  */
  _rxcnt = 0 ;
  uint8 b1 = 0x20 + ( reg & 0xf );
//toggle_cs();
  //PINLOW(CS);
  //__delay_cycles(10);
  mrf_spi_flush_rx();

  mrf_spi_tx(b1);
  mrf_spi_tx(0); // 1 byte
  mrf_spi_tx(0xff); // NOP
  b1 = mrf_spi_rx();
  debfunc(b1);
  while(mrf_spi_data_avail()){
    b1 = mrf_spi_rx();
    debfunc(b1);
  }
  //PINHIGH(CS);

  return b1;
}


static int _spi_tx_err_cnt;
static int _spi_tx_busy_cnt;

static void err_spi_tx(){
  _spi_tx_err_cnt++;

}


uint8 ads1148_write_noblock(uint8 reg,uint8 data){
  /*
  while(mrf_spi_tx_data_avail()){  // block until queue cleared
    _spi_tx_busy_cnt++;
    __delay_cycles(10);
  }
  */

  uint8 b1 = 0x40 + ( reg & 0xf );

  if (mrf_spi_txq(b1) == -1)
    err_spi_tx();
  //__delay_cycles(10);  
  if (mrf_spi_txq(0) == -1)  
    err_spi_tx();
  //  mrf_spi_tx(0); // 1 byte
  //__delay_cycles(10);  
  if (mrf_spi_txq(data) == -1)  
    err_spi_tx();
  //mrf_spi_tx(data);
  //__delay_cycles(10);  
  mrf_spi_start_tx();
 
  return 0;
}


// clear all spi transactions
void flush_spi(){
  while(mrf_spi_tx_data_avail()){ // FIXME
    __delay_cycles(10);  
  }
  mrf_spi_flush_rx();
}

uint8 ads1148_write(uint8 reg,uint8 data){
  /*
  while(mrf_spi_tx_data_avail()){  // block until queue cleared
    __delay_cycles(50);
  }
  */

 
  mrf_spi_flush_rx();
  ads1148_write_noblock(reg,data);

  flush_spi();
  return 0;
}


#define NUM_ADC_INPUTS 7


static uint8 _curr_adc_channel;


static int set_input_only(uint8 channel){
  uint8 rc;
  uint16 rv = 0 ;

  if ( channel > NUM_ADC_INPUTS){
    return -1;
  }
  mrf_spi_flush_rx();
  ads1148_write_noblock(MUX0_OFFS, (channel << 3) | 0x7 );
  ads1148_write_noblock(IDAC1_OFFS,(channel << 4) | 0xf ); // IDAC 1 to channel, IDAC 2 disconnected
  _curr_adc_channel = channel;
  mrf_spi_flush_rx(); 
  return rv;

}
static int port2_icnt;
static unsigned int _rx_flush_cnt;

static int cyc_err1, cyc_err2, cyc_cnt;

static void cycle_input(){
  uint16 rc;
  uint16 rv = 0 ;
  uint8 curr_chan,next_chan,last_chan; //,last_chan;
  cyc_cnt++;
  curr_chan = _curr_adc_channel;
  next_chan = (_curr_adc_channel + 1 ) % MAX_RTDS;
  last_chan = (_curr_adc_channel - 1 );
  if (last_chan >  ( MAX_RTDS-1))
    last_chan = ( MAX_RTDS-1);
  // picking up results of last cycle
  rc = mrf_spi_rx_noblock();  // get first - msb of result
  if (rc == -1)
    cyc_err1++;
  rv += (rc << 8);
  rc = mrf_spi_rx_noblock();  // get second  - lsb of result
  if (rc == -1)
    cyc_err2++;
  rv += rc;

  _rx_flush_cnt += mrf_spi_flush_rx();
  ads1148_write_noblock(MUX0_OFFS, (next_chan << 3) | 0x7 );
  _last_reading[last_chan] = rv;
  //ads1148_write(IDAC0_OFFS, 4 ); // 500uA
  ads1148_write_noblock(IDAC1_OFFS,(next_chan << 4) | 0xf ); // IDAC 1 to channel, IDAC 2 disconnected
  

  
  _curr_adc_channel = next_chan;

  PINHIGH(START);  // pulse start
  // __delay_cycles(8);
  //PINLOW(START);  
  


  return;

}

void sample_start(){
 uint8 b1 = 0x16;
  mrf_spi_flush_rx();   

 if (mrf_spi_tx(b1) == -1)
   err_spi_tx();

 sampling = 1;

}

void sample_stop(){  // stop continous sampling command
 uint8 b1 = 0x16;
  mrf_spi_flush_rx();   

 if (mrf_spi_tx(b1) == -1)
   err_spi_tx();
 sampling = 0;
}


int ads1148_config(){
  // setup ads1148 to measure up to 7  2-wire RTDs with negative connections
  // commoned to AIN7 
  // positive connections of RTDs are connected to AIN0-AIN6
  // A ratiometric measurement process is used , generating REF voltage 
  // from in series Rref across REFP and REFN
  
  ads1148_write(VBIAS_OFFS, 0 );
  ads1148_write(MUX1_OFFS,  ( 1 << 5) | ( 0 << 3) ); // VREF ON, ADC ref is REF0 pin pair
  ads1148_write(SYS0_OFFS, 0 );  // PGA = 1 , 5 SPS
  ads1148_write(IDAC0_OFFS,  4 );  // 500uA IDAC current
  ads1148_write(GPIOCFG_OFFS,  0);  // analogue pin functions

  _curr_adc_channel = 0;
  ads1148_write(MUX0_OFFS, (_curr_adc_channel << 3) | 0x7 );
  ads1148_write(IDAC1_OFFS,(_curr_adc_channel << 4) | 0xf ); // IDAC 1 to channel, IDAC 2 disconnected
  mrf_spi_flush_rx();   
  flush_spi();

}

int ads1148_init(){
  // start output
  PINLOW(START);
  OUTPUTPIN(START);
  // cs output
  PINHIGH(CS);
  OUTPUTPIN(CS);
  //reset output
  //PINHIGH(RESET);
  //OUTPUTPIN(RESET);

  // MR output
  PINHIGH(MR);
  OUTPUTPIN(MR);

  // DRDY INPUT
  INPUTPIN(DRDY);

  __delay_cycles(50);
//PINLOW(RESET);
  PINLOW(MR);
  __delay_cycles(200);
//PINHIGH(RESET);
  PINHIGH(MR);  
  __delay_cycles(2000);

  mrf_spi_flush_rx();
  PINLOW(CS);  // leave SPI continually enabled in this app
  PINHIGH(START);  // device needs to be awake to access config
  // need to enable interrupts on DRDY
  P2REN |= BITNAME(DRDY);
  P2OUT |= BITNAME(DRDY);
  P2IE  |= BITNAME(DRDY);  
  P2IES |= BITNAME(DRDY);
  P2IFG &= ~BITNAME(DRDY);
  port2_icnt = 0;
  cyc_cnt = 0;

  int i,j;
  for (i = 0; i < 1000 ; i++)  
    for (j = 0; j < 2 ; j++)  
      __delay_cycles(1000); // need to wait 16ms




  for (i = 0; i < 2 ; i++)  
      __delay_cycles(1000); // need to wait 16ms
  ads1148_config();
  __delay_cycles(200);
  flush_spi();
  __delay_cycles(200);
  ads1148_config();  // temp desperation
  __delay_cycles(100);
  _rx_flush_cnt = 0;
  cyc_err1 = 0;
  cyc_err2 = 0;
  //PINHIGH(START);  // continuous sampling
  //PINHIGH(START);  // pulse start
  //  __delay_cycles(8);
  //PINLOW(START);  // continuous sampling
  
  
}

uint32_t eval_milliohms(uint16_t adc){
  uint64_t rv = (uint64_t)adc * (uint64_t)_ref_r;
  if (rv == 0)
    return 0;
  rv /= (uint64_t)32767;
  rv -= _ref_i;
  return (uint32_t)rv;
}


int build_state(MRF_PKT_PT1000_STATE *state){

  //uint16 rd = ads1148_data();
  
  mrf_rtc_get(&((*state).td));
  uint8 ch;
  for (ch = 0 ; ch < MAX_RTDS ; ch++){
    (*state).milliohms[ch] = eval_milliohms(_last_reading[ch]);
  }
  (*state).ref_r   = _ref_r;
  (*state).ref_i   = _ref_i;
  (*state).relay_cmd   = 0;
  (*state).relay_state = relay_state();
  //(*state).milliohms[0]= eval_milliohms(0);  // temp  

}



#define APP_SIG_SECOND  0
static int _tick_cnt;
static int _tick_err_cnt;
static MRF_PKT_PT1000_STATE _state[2];

int state_diff(MRF_PKT_PT1000_STATE *sta, MRF_PKT_PT1000_STATE *stb){
  int ch;
  if (sta->relay_state != stb->relay_state)
    return 1;

  for ( ch = 0 ; ch < MAX_RTDS ; ch++)
    if (sta->milliohms[ch] != stb->milliohms[ch]) 
      return 1;
  return 0;
}

int tick_task(){
  int stind;
  _tick_cnt++;
  //return 0;

  stind = _tick_cnt % 2;
    build_state(&_state[stind]);

  // only send struct if readings or relays changed
  if ((_tick_cnt > 2) && state_diff(&_state[0], &_state[1]))
    mrf_send_structure(0,  _MRF_APP_CMD_BASE + mrf_app_cmd_read_state,  (uint8 *)&_state[stind], sizeof(MRF_PKT_PT1000_STATE));
  return 0;
  
}


// this is run by mrf_foreground - so a foreground task
int signal_handler(uint8 signal){

  if (signal == APP_SIG_SECOND)
    return tick_task();
  
}


// each second do this.. in interrupt handler
void on_second(){
  // push signal code to app queue
  mrf_app_queue_push(MRF_BNUM_SIGNAL_BASE + APP_SIG_SECOND);

}


int mrf_app_init(){
  uint8 ch;
  sample_stop_flg = 0;
  dbg22 = 101;
  _tick_cnt = 0;
  _tick_err_cnt = 0;
  _ref_r = (uint32_t)2490*(uint32_t)1000; // nominal resistance between ref+ and ref-
  _ref_i = (uint32_t)47*(uint32_t)1000;   // nominal resistance in series with PT1000

  __delay_cycles(1000);  

  init_relays();
  for (ch = 0 ; ch < MAX_RTDS ; ch++)
    _last_reading[ch] = 0;

  for (ch = 0 ; ch < 10 ; ch++)
  __delay_cycles(1000);  

  _spi_tx_err_cnt = 0;
  mrf_spi_init();
  _spi_tx_busy_cnt = 0;

  for (ch = 0 ; ch < 20 ; ch++)
    __delay_cycles(1000);  
  ads1148_init();
  sampling = 1;
  //PINHIGH(START);  // start continous sampling

  rtc_rdy_enable(on_second);
}

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  _mrf_buff_free(bnum);
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_task_usr_struct(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  _mrf_buff_free(bnum);
}

MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug("mrf_app_task_test entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get((TIMEDATE *)rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug("mrf_app_task_test exit\n");
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_app_spi_read(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  //toggle_cs();
  mrf_debug("mrf_app_read_spi entry bnum %d\n",bnum);
  MRF_PKT_UINT8 *data = (MRF_PKT_UINT8 *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  uint8 rd = ads1148_read(data->value);
  MRF_PKT_UINT8 *rbuff = (MRF_PKT_UINT8 *)mrf_response_buffer(bnum);
  rbuff->value  = rd;
  mrf_send_response(bnum,sizeof(MRF_PKT_UINT8));
  mrf_debug("mrf_app_task_app_read_spi exit\n");
  return MRF_CMD_RES_OK;
}


MRF_CMD_RES mrf_app_spi_data(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  //toggle_cs();
  mrf_debug("mrf_app_spi_data entry bnum %d\n",bnum);
  uint16 rd = ads1148_data();
  MRF_PKT_UINT16 *rbuff = (MRF_PKT_UINT16 *)mrf_response_buffer(bnum);
  rbuff->value  = rd;
  mrf_send_response(bnum,sizeof(MRF_PKT_UINT16));
  mrf_debug("mrf_app_spi_data exit\n");
  return MRF_CMD_RES_OK;
}


MRF_CMD_RES mrf_app_spi_write(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug("mrf_app_spi_write entry bnum %d\n",bnum);
  MRF_PKT_UINT8_2 *data = (MRF_PKT_UINT8_2 *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  ads1148_write(data->d0,data->d1);
  //mrf_send_response(bnum,0);
  _mrf_buff_free(bnum);
  mrf_debug("mrf_app_task_app_write_spi exit\n");
  return MRF_CMD_RES_OK;
}


MRF_CMD_RES mrf_app_config_adc(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug("mrf_app_config_adc entry bnum %d\n",bnum);
  ads1148_config();
  //mrf_send_response(bnum,0);
  _mrf_buff_free(bnum);
  mrf_debug("mrf_app_config_adc exit\n");
  return MRF_CMD_RES_OK;
}




MRF_CMD_RES mrf_app_read_state(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  //toggle_cs();
  mrf_debug("mrf_app_read_state entry bnum %d\n",bnum);
  MRF_PKT_PT1000_STATE *state = (MRF_PKT_PT1000_STATE *)mrf_response_buffer(bnum);
  build_state(state);
  mrf_send_response(bnum,sizeof(MRF_PKT_PT1000_STATE));
  mrf_debug("mrf_app_read_state exit\n");
  return MRF_CMD_RES_OK;
}


extern uint16 _spi_rx_int_cnt;
extern uint16 _spi_tx_int_cnt;
extern uint16 _spi_rx_bytes;
extern uint16 _spi_tx_bytes;
extern uint16 _spi_rxov_err;

MRF_CMD_RES mrf_app_spi_debug(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug("mrf_app_spi_debug entry bnum %d\n",bnum);
  MRF_PKT_SPI_DEBUG pkt;  // FIXME why not get pointer to buffer and cast to MRF_PKT_SPI_DEBUG?
  // e.g. MRF_PKT_SPI_DEBUG  *pkt = (MRF_PKT_SPI_DEBUG *)mrf_response_buffer(bnum); 

  pkt.spi_rx_int_cnt = _spi_rx_int_cnt;
  pkt.spi_tx_int_cnt = _spi_tx_int_cnt;
  pkt.spi_rx_bytes = _spi_rx_bytes;
  pkt.spi_tx_bytes = _spi_tx_bytes;
  pkt.spi_rxov_err = _spi_rxov_err;
  pkt.spi_rx_queue_level = (uint16)mrf_spi_rx_queue_items();
  pkt.spi_tx_queue_level = (uint16)mrf_spi_tx_queue_items();
  pkt.spi_rx_queue_data_avail = (uint8)mrf_spi_data_avail();
  pkt.spi_tx_queue_data_avail = (uint8)mrf_spi_tx_data_avail();

  IQUEUE *spi_rx_q = mrf_spi_rx_queue();
  pkt.rxq_qip = spi_rx_q->qip;
  pkt.rxq_qop = spi_rx_q->qop;
  pkt.rxq_items = spi_rx_q->items;
  pkt.rxq_push_errors = spi_rx_q->push_errors;
  pkt.rxq_pop_errors = spi_rx_q->pop_errors;

  IQUEUE *spi_tx_q = mrf_spi_tx_queue();
  pkt.txq_qip = spi_tx_q->qip;
  pkt.txq_qop = spi_tx_q->qop;
  pkt.txq_items = spi_tx_q->items;
  pkt.txq_push_errors = spi_tx_q->push_errors;
  pkt.txq_pop_errors = spi_tx_q->pop_errors;

  pkt.ucb0_ifg = UCB0IFG;
  pkt.ucb0_ie = UCB0IE;
  pkt.ucb0_cntrl0 = UCB0CTL0;
  pkt.ucb0_cntrl1 = UCB0CTL1;
  pkt.ucb0_stat = UCB0STAT;


  mrf_data_response( bnum,(uint8 *)&pkt,sizeof(MRF_PKT_SPI_DEBUG));  


  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_app_set_relay(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_RELAY_STATE *rs;
  mrf_debug("mrf_app_task_relay entry\n");
  rs = (MRF_PKT_RELAY_STATE *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  set_relay_state(rs->chan,rs->val);
  rs->val = get_relay_state(rs->chan);
  mrf_data_response( bnum,(uint8 *)rs,sizeof(MRF_PKT_RELAY_STATE));  
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_app_get_relay(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_RELAY_STATE *rs;
  mrf_debug("mrf_app_task_relay entry\n");
  rs = (MRF_PKT_RELAY_STATE *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  rs->val = get_relay_state(rs->chan);
  mrf_data_response( bnum,(uint8 *)rs,sizeof(MRF_PKT_RELAY_STATE));  
  return MRF_CMD_RES_OK;
}


MRF_CMD_RES mrf_app_sample_start(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  sample_start();
  _mrf_buff_free(bnum);
  return MRF_CMD_RES_OK;
}
MRF_CMD_RES mrf_app_sample_stop(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  sample_stop_flg = 1;
  _mrf_buff_free(bnum);
  return MRF_CMD_RES_OK;
}


void drdy_int(){
  uint16 rd;
  //port2_icnt++;
  cycle_input();
  //rd = ads1148_data();
  //_last_reading[curr_chan] = rd;
  

}



interrupt(PORT2_VECTOR) PORT2_ISR()
{
  
  P2IFG &= ~BITNAME(DRDY);                          // DRDY int cleared
  port2_icnt++;
  if (sample_stop_flg) {
    sample_stop();
    sample_stop_flg = 0;
  }
  else if(sampling)
    drdy_int();
}
