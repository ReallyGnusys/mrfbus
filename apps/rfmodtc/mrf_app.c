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
#include "mrf_arch.h"
#include "mrf_relays.h"
#include "mrf_route.h"
#include "LCD1x9.h"

#define _BUT1_PORT P1
#define _BUT1_BIT  1

#define _CS_PORT P1
#define _CS_BIT  7



uint8 _app_mode;

static int _sec_count ;
static uint32_t _last_reading[MAX_RTDS];

int mrf_app_init(){
  int i;

  for ( i=0 ; i<MAX_RTDS ; i++){
    _last_reading[i] = 0;
  }
  
  INPUTPIN(BUT1);
  P1REN &= ~BITNAME(BUT1);
  P1IES |=  BITNAME(BUT1);  // hi to lo 
  P1IE  |=  BITNAME(BUT1);
  _app_mode = 0;
  _sec_count = 0;
  init_relays();
  mrf_spi_init();
  PINLOW(CS);
  OUTPUTPIN(CS);
  mrf_spi_tx(0x0); // NOP
  mrf_spi_tx(0x0); // NOP
  LCD1x9_Initialize();
  rtc_second_signal_enable();
  return 0;
}

char _message[20];

int16 modtc_celx4(){
  int16 val;

  PINLOW(CS);
  
  val = (int16)mrf_spi_rx_noblock();
  val <<= 8;
  val |= (int16)mrf_spi_rx_noblock();
  mrf_spi_flush_rx();
  mrf_spi_tx(0x0); // NOP
  mrf_spi_tx(0x0); // NOP
  
  //mrf_spi_tx(0x0); // NOP
  //mrf_spi_tx(0x0); // NOP

  if(val & 0x4)  //no thermocouple
    return -1;
  val >>= 3; // discard 3 lsbs

  while(mrf_spi_busy())
    __delay_cycles(10);
  
  PINHIGH(CS);

  return val;

}
static MRF_PKT_RFMODTC_STATE _state_pkt;

int _dbg_ping_res(){
  return -21;
}

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
   MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
   MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));

   if (resp->type == mrf_cmd_ping) {     
     MRF_PKT_PING_RES *pres = (MRF_PKT_PING_RES *)((void *)hdr + sizeof(MRF_PKT_HDR)+ sizeof(MRF_PKT_RESP));

     if (_app_mode == 1) {
       sprintf(_message,"R S%02X Q%02X",pres->from_rssi,pres->from_lqi);
       _dbg_ping_res();
       LCD1x9_Write(_message);

     }
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



void button_pressed(){

  _app_mode = ( _app_mode + 1 ) % 2;
  

}

static unsigned int P1IVREG;

interrupt(PORT1_VECTOR) PORT1_ISR (void) 
{
  // switch(__even_in_range(P1IV, 16))
 
  P1IVREG = P1IV;
  switch(P1IVREG)
  {
    case  0: break;
    case  2:break;                         // P1.0 IFG
    case  4: 
      P1IE &= ~BIT1;                             // Debounce by disabling buttons
      button_pressed();

      
      //   __bic_SR_register_on_exit(LPM3_bits); // Exit active   
      break;                         // P1.1 IFG
    case  6: break;                         // P1.2 IFG
    case  8: break;                         // P1.3 IFG
    case 10: break;                         // P1.4 IFG
    case 12: break;                         // P1.5 IFG
    case 14: break;                         // P1.6 IFG
    case 16: break;
  }
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


MRF_CMD_RES mrf_app_read_state(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  mrf_debug("mrf_app_read_state entry bnum %d\n",bnum);
  MRF_PKT_RFMODTC_STATE *state = (MRF_PKT_RFMODTC_STATE *)mrf_response_buffer(bnum);
  build_state(state);
  mrf_send_response(bnum,sizeof(MRF_PKT_RFMODTC_STATE));
  mrf_debug("mrf_app_read_state exit\n");
  return MRF_CMD_RES_OK;
}

int _Dbg_ping(){
  return -15;
}

int sec_task(){
  int val,tw,tf;
  //sprintf(_message,"sec %d",_sec_count);
  MRF_ROUTE route;
  P1IE |= BIT1;                             // hmpff -renable button ( crude debouncing )

  val = modtc_celx4();
  tw = val >> 2;
  tf = val & 3;
  _last_reading[0] = val*25;  // units of 100ths of degree

  switch(tf)
    {
    case 0 : break;
    case 1 : tf = 25; break;
    case 2 : tf = 50; break;
    case 3 : tf = 75; break;
    }
  _sec_count++;

  if ( (_app_mode == 0 ) && ((_sec_count % 10) == 0)){
    build_state(&_state_pkt);
    mrf_send_structure(0,  _MRF_APP_CMD_BASE + mrf_app_cmd_read_state,  (uint8 *)&_state_pkt, sizeof(MRF_PKT_RFMODTC_STATE));
  }

  if (_app_mode == 0 ) {
    sprintf(_message,"%2d.%02d C",tw,tf);
    LCD1x9_Write(_message);
  } else {
    mrf_nexthop(&route, MRFID, 0);
    _Dbg_ping();
    mrf_send_command(route.relay,  mrf_cmd_ping,  NULL, 0);

  }
  

  
  return 0;
}

// this is run by mrf_foreground - so a foreground task
int signal_handler(uint8 signal){
 
  if (signal == APP_SIG_SECOND)
    return sec_task();
  return 0;

}

