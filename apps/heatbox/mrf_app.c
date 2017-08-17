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

#include  <msp430.h>
#include <legacymsp430.h>
#include "cc430f5137.h"

#include <limits.h>

#include "mrf_pinmacros.h"

#define NUM_RELAY_CHANNELS 8
#include "mrf_relays.h"

// all gpio pins to TI EVM 


#define _AN0_PORT P2
#define _AN0_BIT  0


#define _AN1_PORT P2
#define _AN1_BIT  1

#define _AN2_PORT P2
#define _AN2_BIT  2

#define _AN3_PORT P2
#define _AN3_BIT  3


#define _VREFM_PORT P2
#define _VREFM_BIT  4

#define _VREFP_PORT P2
#define _VREFP_BIT  5


#define _AN6_PORT P2
#define _AN6_BIT  6

#define _AN7_PORT P2
#define _AN7_BIT  7



// conversion calculation consts

#define PREM 1000000


#define RVRMIN 2490000
#define RVRMAX 1560000

#define RS  770000  //FIXME - this is frigged
#define RB  1000000


volatile int dbg22;

volatile uint8 dbg_u8;
volatile uint8 _rxcnt;

void debfunc(uint8 data){
  dbg_u8 = data;
  _rxcnt += 1;
}



uint8 _dbg_rxdata[4];

// these must retain ability to be tweaked - so non constant

// all units in milliohms
static uint32_t _ref_r;  // ref resistor in bridge - min expected pt1000 value 787 ohm
static uint32_t _ref_t;  // the inline resistance with the PT1000 - at least 47 ohm + lead resistance with EVM
static uint32_t _ref_b;  // the inline resistance with the PT1000 - at least 47 ohm + lead resistance with EVM


// precalculated values for converting ADC to milliohmd

static uint64_t vrmin, vrefi;


static uint16_t _last_reading[MAX_RTDS];



#define NUM_ADC_INPUTS 7


static uint8 _curr_adc_channel;


static int port2_icnt;

uint16_t adc_ctl0_a,adc_ctl0_b, adc_ctl0_c;

#define REF_SUPPLY 0
#define REF_EXT    2
#define REF_DEXT   7

int init_adc12() {


   // setup port pins for analogue input

  PMAPKEYID = 0x2d52;
  
  ADCPIN(AN0);
  ADCPIN(AN1);
  ADCPIN(AN2);
  ADCPIN(AN3);
  ADCPIN(VREFM);
  ADCPIN(VREFP);
  ADCPIN(AN6);
  ADCPIN(AN7);

  PMAPKEYID = 0x0;

  //  PORT_MAP to analogue
/*
  P2MAP0  = PM_ANALOG;
  P2MAP1  = PM_ANALOG;
  P2MAP2  = PM_ANALOG;
  P2MAP3  = PM_ANALOG;
  P2MAP4  = PM_ANALOG;
  P2MAP5  = PM_ANALOG;
  P2MAP6  = PM_ANALOG;
  P2MAP7  = PM_ANALOG;
*/

  //ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_2; // Turn on ADC12, set sampling time
  ADC12CTL0 = ADC12ON | ADC12SHT0_2| ADC12MSC; // Turn on ADC12, set sampling time
  ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;       // Use sampling timer, single sequence
  //ADC12CTL1 = ADC12CONSEQ_1;       // Use sampling timer, single sequence
  ADC12MCTL0 = ADC12SREF_7 | ADC12INCH_0;                 // ref+=AVcc, channel = A0
  ADC12MCTL1 = ADC12SREF_7 | ADC12INCH_1;                 // ref+=AVcc, channel = A1
  ADC12MCTL2 = ADC12SREF_7 | ADC12INCH_2;                 // ref+=AVcc, channel = A2
  ADC12MCTL3 = ADC12SREF_7 | ADC12INCH_3;                 // ref+=AVcc, channel = A2
  ADC12MCTL4 = ADC12SREF_7 | ADC12INCH_6;                 // ref+=AVcc, channel = A2
  ADC12MCTL5 = ADC12SREF_7 | ADC12INCH_7+ADC12EOS;        // ref+=AVcc, channel = A3, end seq.
  //ADC12IE = 0x08;                           // Enable ADC12IFG.3
  ADC12CTL0 |= ADC12ENC;                    // Enable conversions
  ADC12CTL0 |= ADC12SC;                   // Start convn - software trigger
  adc_ctl0_c = ADC12CTL0;

}

uint64_t vri(uint64_t r){
  
  return  (uint64_t)PREM * (uint64_t)RB/(r + (uint64_t)RB);
}



uint64_t v2ri(uint64_t sv){
  uint64_t r;
  r =  (( (uint64_t)RB*(uint64_t)PREM)/sv) - (uint64_t)RB;

  return r;
}

uint32_t eval_milliohms(uint16_t adc){

  
  uint64_t rv;
  if (adc == 0)
    return ULONG_MAX;
  rv = ((uint64_t)adc * vrefi) / 4095;
  rv = rv + vrmin;
  rv = v2ri(rv);
  rv = rv - (uint64_t)RS;
  return (uint32_t)rv;
}


int build_state(MRF_PKT_PT1000_STATE *state){

  //uint16 rd = ads1148_data();
  
  mrf_rtc_get(&((*state).td));
  uint8 ch;
  for (ch = 0 ; ch < MAX_RTDS ; ch++){
    (*state).milliohms[ch] = eval_milliohms(_last_reading[ch]);
    //(*state).milliohms[ch] = (uint32_t)_last_reading[ch];
  }

  
  (*state).ref_r   = _ref_r;
  (*state).ref_i   = _ref_b;
  (*state).relay_cmd   = 0;
  (*state).relay_state = relay_state();
  //(*state).milliohms[0]= eval_milliohms(0);  // temp  

}



#define APP_SIG_SECOND  0
static int _tick_cnt;
static uint16_t _tick_err_cnt,_tick_ok_cnt;
static uint16_t _tick_ctl0,_tick_ctl1,_tick_ctl2;
static MRF_PKT_PT1000_STATE _state; 

int tick_task(){
  int ch;
  uint8_t ctlreg;
  _tick_cnt++;
  //return 0;

  /*
  for (ch = 0; ch < MAX_RTDS;  ch++){
    
    _last_reading[ch] = (uint16_t) ADC12MEM0 + ch);

  }
  */
  /*
    _last_reading[0] = *((uint16_t *)ADC12MEM0);
    _last_reading[1] = *((uint16_t *)ADC12MEM1);
    _last_reading[2] = *((uint16_t *)ADC12MEM2);
    _last_reading[3] = *((uint16_t *)ADC12MEM3);
    _last_reading[4] = *((uint16_t *)ADC12MEM4);
    _last_reading[5] = *((uint16_t *)ADC12MEM5);
    _last_reading[6] = *((uint16_t *)ADC12MEM6);
  */

    _last_reading[0] = (uint16_t) ADC12MEM0;
    _last_reading[1] = (uint16_t) ADC12MEM1;
    _last_reading[2] = (uint16_t) ADC12MEM2;
    _last_reading[3] = (uint16_t) ADC12MEM3;
    _last_reading[4] = (uint16_t) ADC12MEM4;
    _last_reading[5] = (uint16_t) ADC12MEM5;
    _last_reading[6] = (uint16_t) ADC12MEM6;

    
  // restart conversion
  _tick_ctl0 = ADC12CTL0;
  _tick_ctl1 = ADC12CTL1;
  _tick_ctl2 = ADC12CTL2;

  if (_tick_ctl1 & 0x01){
    _tick_err_cnt++;
  }
  else{

    _tick_ok_cnt ++;
  }

  //ADC12CTL0  =  _tick_ctl0 | ADC12SC | ADC12ENC;  // start  conversion

    ADC12CTL0 |= ADC12ENC | ADC12SC;                   // Start convn - software trigger

  
  //ADC12CTL0  =  _tick_ctl0 & (0xfffe);
  build_state(&_state);

  mrf_send_structure(0,  _MRF_APP_CMD_BASE + mrf_app_cmd_read_state,  (uint8 *)&_state, sizeof(MRF_PKT_PT1000_STATE));
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
  uint64_t vrmax;
  dbg22 = 101;
  _tick_cnt = 0;
  _tick_err_cnt = 0;
  _tick_ok_cnt = 0;
  _ref_r = (uint32_t)787*(uint32_t)1000; // nominal resistance between ref+ and ref-
  _ref_t = (uint32_t)787*(uint32_t)1000; // nominal resistance between ref+ and ref-
  _ref_b = (uint32_t)1000*(uint32_t)1000;   // nominal resistance in series with PT1000


  vrmax = vri(RVRMAX);
  vrmin = vri(RVRMIN);
  vrefi  = vrmax - vrmin;

  init_relays();
  for (ch = 0 ; ch < MAX_RTDS ; ch++)
    _last_reading[ch] = 0;
  init_adc12();
  
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




