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
#endif
