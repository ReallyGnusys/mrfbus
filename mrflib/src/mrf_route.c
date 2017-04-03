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

#include "mrf_route.h"

#define SNETSZ  0x20

// Routes are calculated rather than lookup up.

// Position in net topology can be determined by address

// 1 = host application

// 2 - 0xF  are motes connected to host by USB

// 0x20 - 0x3F  - relay messages to hub via address 2

// 0x40 - 0x5F  - relay messages to hub via address 4

// ----

// 0xE0 - 0xFF - relay messages to hub via address 0xe


// system supports using a radio relay between usbmote and subnet device
// if HOP_TO_SNET is defined device 0x20 will be relay hub 
#define HOP_TO_SNET 
//#include <stdio.h>

//calculate the cell relay node that handles traffic for this address 
uint8 mrf_cell_relay(uint8 addr){
  if (addr < 2)
    return 0;
#ifdef HOP_TO_SNET
  if (addr/SNETSZ == 0)  // usb rf base station or host
    return 1;
  if (addr%SNETSZ)     // rf rf  base station
    return SNETSZ * (addr/SNETSZ);
  else                 // rf leaf cell
    return (SNETSZ/0x10)*addr/SNETSZ;
#else
  if(addr/SNETSZ == 0)  // usb rf base station
    return  1;
  return (SNETSZ/0x10)*addr/SNETSZ;  // rf leaf cell

#endif
}

uint8 order(uint8 addr){
  if(addr ==0)
    return 0;
  if (addr == 1)    // host
    return 1;
  if ( mrf_cell_relay(addr) == 1)  // usb rf base station
    return 2;
#ifdef HOP_TO_SNET
  if (mrf_cell_relay(addr)/SNETSZ == 0)
    return 3;
  else
    return 4;
#else
    return 3;
#endif
}
uint8 branch(uint8 addr){
  if(addr < 2)
    return 0;
  if (addr/SNETSZ == 0)
    return addr%SNETSZ;
  else
    return (SNETSZ/0x10)*(addr/SNETSZ);
}
        
uint8 nextbranchhopout(uint8 src,uint8 dest){
  uint8 hop = dest;
  uint8 rel = mrf_cell_relay(hop);
  while (rel != src){
    hop = rel;
    rel = mrf_cell_relay(hop);
  }
  return hop;
}

// FIXME - this is rubbish
// how many I_Fs has an address got
uint8 num_ifs(uint8 addr){
  return  addr == 0 ? 1 :addr == 1 ?  (256/SNETSZ)-1 : (addr / SNETSZ) == 0 ? 2 :1 ;
  
}


uint8 mrf_nexthop(MRF_ROUTE *route,uint8 us,uint8 dest){
  route->i_f = 0; 
 if (us == 0 ) {
    route->relay = 1;    
    return route->relay;
  }

  if (dest < 2 ) {
    route->relay = mrf_cell_relay(us);    
    return route->relay;
  }
  else if (us == 1){
    //printf("us 1 : bd %d MUL %d \n",branch(dest),SNETSZ/0x10);
    route->i_f = ( branch(dest) * 0x10/SNETSZ ) %  NUM_INTERFACES; 
    route->relay = branch(dest);
    return route->relay;
  }    
  else if(  branch(us) == branch(dest)){
    if( order(dest) > order(us)){
      route->relay = nextbranchhopout(us,dest);
      if( order(us) == 2)
        route->i_f = 1;
      return route->relay;
    }
    else if (order(us) == order(dest)){
      route->relay = dest;
      return route->relay;
    }
    else{
      route->relay = mrf_cell_relay(us);
      return route->relay;
    }
  }
  else {
    route->relay = mrf_cell_relay(us);
    return route->relay;
  }
}
            

















