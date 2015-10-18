#include "iqueue.h"


//circular uint8 buffer

void queue_init(IQUEUE *q){
  int i;
  q->qip = 0;
  q->qop = 0;
  q->items = 0;
  q->push_errors = 0;
  q->pop_errors = 0;

}

int queue_full(IQUEUE *q){
  return (q->items == (IQUEUE_DEPTH -1 ));
}

int queue_data_avail(IQUEUE *q){
  return (q->items != 0);
}

// returns elem for entry qop
static uint8 _q_elem(IQUEUE *q, uint8 index){
  return q->buffer[q->qop ];
}

// sets elem for entry qip
static void _q_elem_set(IQUEUE *q, uint8 val){
  q->buffer[q->qip] = val;
}

uint8 queue_head(IQUEUE *q){
  return q->buffer[q->qop];
}


int queue_push(IQUEUE *q, uint8 data){
  int i;
  if (queue_full(q)){
    q->push_errors++;
    return -1;
  }
  _q_elem_set(q,data);  
  q->qip = (q->qip + 1) % IQUEUE_DEPTH;
  q->items++;
  return 0;
}


uint8 queue_pop(IQUEUE *q){
  uint8 data;
  if (queue_data_avail(q) == 0){
    q->pop_errors++;
    return 0;
  }
  
  data = queue_head(q);
  
  q->qop = (q->qop + 1) % IQUEUE_DEPTH;
  q->items--;
  return data;

}
