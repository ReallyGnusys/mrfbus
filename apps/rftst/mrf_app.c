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

int mrf_app_init(){
  init_relays();

  
}


// this is run by mrf_foreground - so a foreground task
int signal_handler(uint8 signal){
  /*
  if (signal == APP_SIG_SECOND)
    return tick_task();
  return 0;
  */
}


MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
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



