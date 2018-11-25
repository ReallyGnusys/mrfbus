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

#ifndef _MRF_IF_INCLUDED_
#define _MRF_IF_INCLUDED_
//#include <mrf_sys.h>
//#include <device.h>
#include "mrf_sys_structs.h"
#include "iqueue.h"
#include "mqueue.hpp"






#include "device.h"


// I_F logic is implemented largely in _mrf_tick ( mrf_sys.c )

// sets initial flags for each transaction (tx_buff, tx_complete and waiting_resp/resp_timer) when initiating TX
// and sets state to  tx_active

// driver is required to
// set tx_complete when buffer has been transmitted

// RX callbacks, (currently buffer_responded from mrf_task_ack etc) clear waiting_resp

// mrf_tick logic :
/*

if (istate==ACK_DEL){
  dec timer
  if timer == 0:
    send_next_ack
    cont
}

if (istate==TX_DEL){
  dec timer
  if timer == 0:
    send_next_queued
    cont

}


 || (istate==TX_DEL){
  // check tx timeout
}


 if ((istate==TX_ACTIVE) && tx_complete)
     state = TX_IDLE

 if (istate==TX_IDLE) {
    if (waiting_resp) {
      //dec resp_time and check for timeout
      //
    }
    else if(tx_complete){
      if(tx_buff){
         pop txq
      }
      tx_complete = 0;
    }

    if(ackqueue)
      tx ack

    else if (not waiting resp and not tx_complete and txq)
      tx q


}
*/


void mrf_if_init();

//voi mrf_if_register(I_F i_f, const MRF_IF_TYPE *type);

const MRF_IF *mrf_if_ptr(I_F i_f);

int8 mrf_if_tx_queue(I_F i_f, uint8 bnum );
void mrf_if_tx_done(I_F i_f);

void _mrf_if_print_all();
int mrf_if_recieving(I_F i_f);
int mrf_if_transmitting(I_F i_f);
int mrf_if_can_sleep(I_F i_f);
const char * mrf_if_state_name(I_F i_f);
void mrf_if_print_info(I_F i_f);

#endif
