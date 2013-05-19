#ifndef __QUEUE_INCLUDED__
#define __QUEUE_INCLUDED__

#include "g430_types.h"

typedef struct volatile{
  volatile uint8 qip;
  volatile uint8 qop;
  volatile uint8 items;
  uint8 max_len;
  volatile uint8 push_errors;
  volatile uint8 pop_errors;
  volatile uint8 push_ind;
  volatile uint8 pop_ind;
  uint8 *buffer;
  uint8 *blen;
} QUEUE;

void queue_init(QUEUE *q,uint8 max_len,uint8 *buffer);
int queue_full(QUEUE *q);
int queue_data_avail(QUEUE *q);
int queue_push(QUEUE *q, uint8 data);
uint8 queue_pop(QUEUE *q);
#endif
