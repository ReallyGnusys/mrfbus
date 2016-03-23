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
#ifndef __IQUEUE_INCLUDED__
#define __IQUEUE_INCLUDED__

#include "mrf_types.h"
#include "device.h"  // for IQUEUE_DEPTH

//typedef struct volatile {
typedef struct  {
  volatile uint8 qip;
  volatile uint8 qop;
  volatile uint8 items;
  volatile uint8 push_errors;
  volatile uint8 pop_errors;
  uint8 buffer[IQUEUE_DEPTH];
} IQUEUE;

void queue_init(IQUEUE *q);
int queue_full(IQUEUE *q);
int queue_data_avail(IQUEUE *q);
int queue_push(IQUEUE *q, uint8 data);
int16 queue_pop(IQUEUE *q);
int queue_flush(IQUEUE *q);
#endif
