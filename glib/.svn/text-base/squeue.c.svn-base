#include "squeue.h"
#include "stddef.h"

#define INC_QPTR(ptr,depth)  ptr = (ptr + 1) % depth

void squeue_init(SQUEUE *q,uint8 depth,uint8 max_len,uint8 *buffer,uint8 *blens){
  int i;
  q->qip = 0;
  q->qop = 0;
  q->items = 0;
  q->depth = depth;
  q->max_len = max_len;
  q->push_errors = 0;
  q->pop_errors = 0;
  q->push_count = 0;
  q->pop_count = 0;

  q->buffer = buffer;
  q->blen = blens;

  q->push_ind = 0;
  q->pop_ind = 0;
  
  for ( i = 0 ; i < q->depth ; i++)
    q->blen[i] = 0;
}

int squeue_full(SQUEUE *q){
  return (q->items == q->depth);
}

int squeue_data_avail(SQUEUE *q){
  return (q->items != 0);
}

// returns elem for entry qop
static uint8 _sq_elem(SQUEUE *q, uint8 index){
  return q->buffer[q->qop * q->max_len + index];
}

// sets elem for entry qip
static void _sq_elem_set(SQUEUE *q, uint8 index,uint8 val){
  q->buffer[q->qip * q->max_len + index] = val;
}

uint8 *squeue_entry(SQUEUE *q,uint8 entry){
  return &q->buffer[entry * q->max_len];
}
uint8 squeue_entry_len(SQUEUE *q,uint8 entry){
  return q->blen[entry];
}

int squeue_push(SQUEUE *q, uint8 *buff, uint8 len){
  int i;
  if (squeue_full(q)){
    q->push_errors++;
    return -1;
  }
  for ( i = 0 ; i < len + 1; i++)
    _sq_elem_set(q,i,buff[i]);
 
  _sq_elem_set(q,i,0); // null term
  q->blen[q->qip] = len;
  q->qip = (q->qip + 1) % q->depth;
  q->items++;
  q->push_count++;

}

uint8 *squeue_head(SQUEUE *q){
  return squeue_entry(q,q->qop);

}
uint8 squeue_head_len(SQUEUE *q){
  return squeue_entry_len(q,q->qop);

}


int squeue_pop(SQUEUE *q, uint8 *buff){
  int i,len;
  if (squeue_data_avail(q) == 0){
    q->pop_errors++;
    return -1;
  }
  len = (int)q->blen[q->qop];

  if ( buff != NULL)
    for ( i = 0 ; i < len ; i++)
      buff[i] =  _sq_elem(q, i);
  q->qop = (q->qop + 1) % q->depth;
  q->items--;
  q->pop_count++;

  return len;

}
