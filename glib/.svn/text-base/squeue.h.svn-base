#ifndef __SQUEUE_INCLUDED__
#define __SQUEUE_INCLUDED__

#include "g430_types.h"
#include "glib.h"
typedef struct {
  uint8 qip;
  uint8 qop;
  uint8 items;
  uint8 max_len;
  uint8 depth;
  uint8 push_errors;
  uint8 pop_errors;
  uint8 push_ind;
  uint8 pop_ind;
  uint8 *buffer;
  uint8 *blen;
  uint16  push_count;
  uint16  pop_count;
} SQUEUE;

void squeue_init(SQUEUE *q,uint8 depth,uint8 max_len,uint8 *buffers,uint8 *blens);
int squeue_full(SQUEUE *q);
int squeue_data_avail(SQUEUE *q);
int squeue_push(SQUEUE *q, uint8 *buff, uint8 len);
int squeue_pop(SQUEUE *q,uint8 *buffer);
uint8 *squeue_entry(SQUEUE *q,uint8 entry);
uint8 squeue_entry_len(SQUEUE *q,uint8 entry);
uint8 *squeue_head(SQUEUE *q);
uint8 squeue_head_len(SQUEUE *q);

#endif
