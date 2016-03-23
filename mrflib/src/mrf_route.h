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

#ifndef __MRF_ROUTE_INCLUDED__
#define __MRF_ROUTE_INCLUDED__

#include "mrf_sys.h"
#define SNETSZ  0x20

#ifndef MRFID

//error "MRFID must be defined e.g. use -DMRFID=id in gcc args"
#define MRFID 1
#endif

#define NUM_IFS MRFID == 1 ?  (256/SNETSZ)-1 : (MRFID / SNETSZ) ==0 ? 2 :1 

typedef struct {
  I_F  i_f;
  uint8 relay;  
} MRF_ROUTE;

uint8 mrf_nexthop(MRF_ROUTE *route,uint8 us,uint8 dest);
uint8 num_ifs(uint8 addr);

#endif
