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


#include "mrf_if.h"
#include "device.h"
static IF_STATUS _if_status[NUM_INTERFACES];
  static MRF_PKT_HDR _if_ackbuffs[NUM_INTERFACES];


//extern const MRF_IF_TYPE mrf_uart_cc_if;
extern const MRF_IF_TYPE mrf_rf_cc_if;

extern const MRF_IF _sys_ifs[NUM_INTERFACES] = {
  {&_if_status[RF0], &mrf_rf_cc_if  , &_if_ackbuffs[RF0]}  
};

