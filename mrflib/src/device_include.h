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
#ifndef __DEVICE_INCLUDE_INCL__
#define __DEVICE_INCLUDE_INCL__
// should only be included in device.c

#include "mrf_sys_structs.h"
static IF_STATUS   _if_status[NUM_INTERFACES];
static MRF_PKT_HDR _if_ackbuffs[NUM_INTERFACES];
static AckQueue    _if_ack_queues[NUM_INTERFACES];
static BuffQueue   _if_tx_queues[NUM_INTERFACES];
#ifdef MRF_ARCH_lnx
static int   _if_fd[NUM_INTERFACES];
#endif

#endif //#ifndef __DEVICE_INCLUDE_INCL__
