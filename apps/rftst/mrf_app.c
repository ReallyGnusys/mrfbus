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

#include "mrf_relays.h"

typedef enum {
  SYNC_TIME,
  SHOW_TIME} APP_STATE;

typedef enum {
  ADC_TEMP,
  ADC_VCC} ADC_CHAN;


static APP_STATE _app_state;
static ADC_CHAN _adc_chan;

static MRF_ROUTE _base_route;
static uint16 _adc_res;  // vcc div 2 and temp

static uint32_t tgrad100, tycross100;
static uint16_t _sec_count;

static uint32_t _last_reading[MAX_RTDS];
static MRF_PKT_RFMODTC_STATE _state_pkt;

static uint32_t _ftaps[10];



int mrf_sleep_deep(){
  mrf_rf_idle(RF0);
  return 0;
}
int _appdbg_22(){
  return 22;
}


int mrf_app_init(){

  P3DIR = 0xFF;
  P3OUT = 0x0;

  P2DIR = 0xFC;
  P2OUT = 0x0;
  
  init_relays();
  _app_state = SYNC_TIME;
  mrf_nexthop(&_base_route, MRFID, 0);  // get relay/basestation route
  _appdbg_22();
  _sec_count = 0;

  tgrad100 =  (100*((uint32_t)*((uint16_t *)0x1A1C) - *((uint16_t *)0x1A1A)))/55;  // cal points for 85 and 30C
  tycross100 =   (100*(uint32_t)(*((uint16_t *)0x1A1A))) - (30*tgrad100);

  
  _adc_chan = ADC_TEMP;
  // set up ADC

  REFCTL0 = REFMSTR+REFVSEL_0+REFON;  //+REFTCOFF; 

  //ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
  ADC12CTL0 =   ADC12SHT0_8 + ADC12ON;         // Sampling time, ADC12 on
  ADC12CTL1 = ADC12SHP;                     // Use sampling timer
  ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC input ch A10 => temp sense 
  ADC12CTL0 |= ADC12ENC;

  rtc_second_signal_enable();

  return 0;
}

#define ADC_CHANNELS 2








int sec_task(){

  if (_app_state == SYNC_TIME){ // relying on usr_resp to change state when response received
    mrf_send_command(_base_route.relay,  mrf_cmd_get_time,  NULL, 0);
  }
  else {  // SHOW_TIME
    _sec_count++;

    
      ADC12IE = 0x01;                           // Enable interrupt
      ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion
  }

  
  return 0;
}


int build_state(MRF_PKT_RFMODTC_STATE *state){

  //uint16 rd = ads1148_data();
  
  mrf_rtc_get(&((*state).td));
  uint8 ch;
  for (ch = 0 ; ch < MAX_RTDS ; ch++){
    (*state).tempX100[ch] = _last_reading[ch];
  }
  (*state).relay_cmd   = 0;
  (*state).relay_state = relay_state();
  return 0;
}



int adc_res_ready(){
  uint32_t temp,avg;
  //uint16 adcres  = _adc_res;
  int i;
  temp = (100*(uint32_t)_adc_res)-tycross100;
  temp = 100* temp / tgrad100;

  temp = temp - 270;  // single point calibration done wrt. modtc thermocouple device.. ho ho
  temp = temp / 10;
  temp = temp * 10;  // zero lsb

  
  if ((_sec_count % 10) == 0){  // send avg every 10 secs
    avg = 0;
    for ( i = 0 ; i < 10 ; i++)
      avg = avg + _ftaps[i];

    avg = avg / 10;
    _last_reading[0] = avg; // only one channel here
    build_state(&_state_pkt);
    mrf_send_structure(0,  _MRF_APP_CMD_BASE + mrf_app_cmd_read_state,  (uint8 *)&_state_pkt, sizeof(MRF_PKT_RFMODTC_STATE));

  }
  _ftaps[_sec_count % 10 ] = temp;

  return 0;
}
  

// this is run by mrf_foreground - so a foreground task
int signal_handler(uint8 signal){
  if (signal == APP_SIG_SECOND)
    return sec_task();
   else if(signal == APP_SIG_ADC_RES)
     return adc_res_ready();
 
  return 0;

}


int _app_dbg23(){
  return 23;
}
MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  _app_dbg23();
  if ((_app_state == SYNC_TIME ) && (resp->type == mrf_cmd_get_time)){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    rtc_set(td);
    _app_state = SHOW_TIME;
  }
  _mrf_buff_free(bnum);
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_task_usr_struct(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  _mrf_buff_free(bnum);
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug("%s","mrf_app_task_test entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get((TIMEDATE *)rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug("%s","mrf_app_task_test exit\n");
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

MRF_CMD_RES mrf_app_led_on(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_RELAY_STATE rs;
  mrf_debug("mrf_app_task_relay entry\n");
  set_relay_state(0,1);
  rs.chan = 0;
  rs.val = get_relay_state(rs.chan);
  mrf_data_response( bnum,(uint8 *)&rs,sizeof(MRF_PKT_RELAY_STATE));  
  return MRF_CMD_RES_OK;
}
MRF_CMD_RES mrf_app_led_off(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_RELAY_STATE rs;
  mrf_debug("mrf_app_task_relay entry\n");
  set_relay_state(0,0);
  rs.chan = 0;
  rs.val = get_relay_state(rs.chan);
  mrf_data_response( bnum,(uint8 *)&rs,sizeof(MRF_PKT_RELAY_STATE));  
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



MRF_CMD_RES mrf_app_read_state(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  mrf_debug("mrf_app_read_state entry bnum %d\n",bnum);
  MRF_PKT_RFMODTC_STATE *state = (MRF_PKT_RFMODTC_STATE *)mrf_response_buffer(bnum);
  build_state(state);
  mrf_send_response(bnum,sizeof(MRF_PKT_RFMODTC_STATE));
  mrf_debug("mrf_app_read_state exit\n");
  return MRF_CMD_RES_OK;
}

int _adc_res_rdy(){
  ADC12IE = 0x0;                           // disable interrupt
  _adc_res = ADC12MEM0;

  mrf_app_signal(APP_SIG_ADC_RES);

  
}

interrupt(ADC12_VECTOR) ADC12ISR(void)
{

  switch(ADC12IV)
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
    
    ADC12IE = 0x0;                           // disable interrupt
    
    _adc_res_rdy();
    if (mrf_wake_on_exit())
      __bic_SR_register_on_exit(LPM3_bits);
    
    break;
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}
