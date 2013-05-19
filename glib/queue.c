#include "queue.h"

#define INC_QPTR(ptr,depth)  ptr = (ptr + 1) % depth

//circular uint8 buffer

void queue_init(QUEUE *q,uint8 max_len,uint8 *buffer){
  int i;
  q->qip = 0;
  q->qop = 0;
  q->items = 0;
  q->max_len = max_len;
  q->push_errors = 0;
  q->pop_errors = 0;
  q->buffer = buffer;


}

int queue_full(QUEUE *q){
  return (q->items == (q->max_len -1 ));
}

int queue_data_avail(QUEUE *q){
  return (q->items != 0);
}

// returns elem for entry qop
static uint8 _q_elem(QUEUE *q, uint8 index){
  return q->buffer[q->qop ];
}

// sets elem for entry qip
static void _q_elem_set(QUEUE *q, uint8 val){
  q->buffer[q->qip] = val;
}

uint8 queue_head(QUEUE *q){
  return q->buffer[q->qop];
}


int queue_push(QUEUE *q, uint8 data){
  int i;
  if (queue_full(q)){
    q->push_errors++;
    return -1;
  }
  _q_elem_set(q,data);  
  q->qip = (q->qip + 1) % q->depth;
  q->items++;
  return 0;
}


uint8 queue_pop(QUEUE *q){
  uint8 data;
  if (queue_data_avail(q) == 0){
    q->pop_errors++;
    return 0;
  }
  
  data = queue_head(q);
  
  q->qop = (q->qop + 1) % q->max_len;
  q->items--;
  return data;

}
