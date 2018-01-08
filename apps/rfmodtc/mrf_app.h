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

#ifndef __MRF_APP_INCLUDED__
#define __MRF_APP_INCLUDED__

// define APP specific packets
#define MAX_RTDS 1

typedef struct  __attribute__ ((packed))   {
  TIMEDATE td;
  uint8    relay_cmd;  // 8 bit emergency masks for request and ideally a validated check below
  uint8    relay_state;
  uint32   tempX100[MAX_RTDS];  // channels
} MRF_PKT_RFMODTC_STATE;
  
typedef struct  __attribute__ ((packed))   {
  uint8 chan;  
  uint8 val;  
} MRF_PKT_RELAY_STATE;
    

/* mrf_app_task_test
   returns current MRF_PKT_TIMEDATE 
*/
int mrf_spi_init_cc()  __attribute__ ((constructor));
MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_app_led_on(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_app_led_off(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_app_set_relay(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);  // set one relay
MRF_CMD_RES mrf_app_get_relay(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);  // set one relay state
MRF_CMD_RES mrf_app_read_state(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp); // read ptd device state

#endif
