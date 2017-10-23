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


#include "iqueue.h"


//circular uint8 buffer

void queue_init(IQUEUE *q){
  q->qip = 0;
  q->qop = 0;
  q->items = 0;  // FIXME kind of pointless ..
  q->push_errors = 0;
  q->pop_errors = 0;

}

int queue_items(IQUEUE *q){
  int items;

  if (q->qip >= q->qop)
    items = q->qip - q->qop;
  else
    items = (IQUEUE_DEPTH*2 + q->qip) -  q->qop;
  return items;
}
int queue_flush(IQUEUE *q){
  int items = queue_items(q);
  
  q->items = 0;  //FIXME shouldn't need this field - use function above 
  q->qop = 0;
  q->qip = 0;
  
  //return (q->items == (IQUEUE_DEPTH -1 ));
  return items;
}

int queue_full(IQUEUE *q){
  return ( queue_items(q) >= IQUEUE_DEPTH);  
  //return ((q->qip+1) % (IQUEUE_DEPTH*2)) == q->qop;
}

int queue_data_avail(IQUEUE *q){
  return (q->qip != q->qop);
}

// returns elem for entry qop
static uint8 _q_elem(IQUEUE *q, uint8 index){
  return q->buffer[q->qop % IQUEUE_DEPTH];
}

// sets elem for entry qip
static void _q_elem_set(IQUEUE *q, uint8 val){
  q->buffer[q->qip % IQUEUE_DEPTH] = val;
}

uint8 queue_head(IQUEUE *q){
  return q->buffer[q->qop % IQUEUE_DEPTH];
}


int queue_push(IQUEUE *q, uint8 data){
  if (queue_full(q)){
    q->push_errors++;
    return -1;
  }
  _q_elem_set(q,data);  
  q->qip = (q->qip + 1) % (IQUEUE_DEPTH * 2);
  q->items++;
  return 0;
}


int16 queue_pop(IQUEUE *q){
  if (queue_data_avail(q) == 0){
    q->pop_errors++;
    return -1;
  }
  int16 data = (int16)queue_head(q);
  q->qop = (q->qop + 1) % (IQUEUE_DEPTH * 2);
  q->items--;
  return data;
}


uint8 queue_pop_old(IQUEUE *q){
  uint8 data;
  if (queue_data_avail(q) == 0){
    q->pop_errors++;
    return 0;
  }
  
  data = queue_head(q);
  
  q->qop = (q->qop + 1) % IQUEUE_DEPTH;
  return data;

}
