#include "iqueue.h"
#include "stdio.h"

void print_qinf(IQUEUE *q){
  printf("\nQ info\n");
  printf("qip         = %u\n",q->qip);
  printf("qop         = %u\n",q->qop);
  printf("items       = %u\n",q->items);
  printf("push_errors = %u\n",q->push_errors);
  printf("pop_errors  = %u\n",q->pop_errors);
  printf("data_avail  = %d\n",queue_data_avail(q));
  printf("queue_items = %d\n",queue_items(q));
  printf("queue_full  = %d\n",queue_full(q));
  

}

int main(void){
  IQUEUE q;
  uint8 item;
  int loop;
  queue_init(&q);
  print_qinf(&q);
  
  for ( loop = 0 ; loop < 4 ; loop++ ){
    printf("\nLOOP %d\n",loop);
    printf("***********\n");
    for (item = 0 ; item < IQUEUE_DEPTH ; item ++){
      printf("pushing item %d\n",item);
 
      if ( item != q.items){
        printf("unexpected item got %d expected %d\n", q.items,item);
        return -1;
      }
      if ( item != queue_items(&q)){
        printf("unexpected item got %d expected %d\n", queue_items(&q),item);
        return -1;
      }
      queue_push(&q,item);
      print_qinf(&q);  
    }
    

    for (item = 0 ; item < IQUEUE_DEPTH ; item ++){
      printf("popping item %d\n",item);
      queue_pop(&q);
      print_qinf(&q);
    }
  }
  return 0;
}
