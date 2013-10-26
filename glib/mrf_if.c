/* mrf bus interface */

// allocate interface structures


//#include <mrf_if.h>
//#include <mrf_buff.h>
#include <mrf_sys.h>
#include <stdio.h>

MRF_IF _sys_ifs[NUM_INTERFACES];

MRF_IF *mrf_if_ptr(I_F i_f){
  return &_sys_ifs[i_f];
}
void mrf_if_init(){
  int i,j;
  uint8 *dptr;
  for (i = 0 ; i < NUM_INTERFACES ; i++){
    // rough zeroing of data
    dptr = (uint8 *)&_sys_ifs[i];
    for ( j = 0 ; j < sizeof(IF_STATUS) ; j++)
      dptr[j] = 0;    
    queue_init(&(_sys_ifs[i].status.txqueue));
    _sys_ifs[i].status.state = MRF_ST_IDLE;
    _sys_ifs[i].status.tx_status = MRF_TX_IDLE;

  }

}

void mrf_if_register(I_F i_f, MRF_IF_TYPE *type){
  _sys_ifs[i_f].type = type;
  
}

void _mrf_if_print_all(){

  I_F i_f;
  MRF_IF *ifp;
  for ( i_f = 0 ; i_f < NUM_INTERFACES ; i_f++){
    MRF_IF *ifp = mrf_if_ptr(i_f);

    printf("I_F %d state %d txq_da %d\n",i_f,ifp->status.state,queue_data_avail(&(ifp->status.txqueue)));

  }
}

int8 mrf_if_tx_queue(I_F i_f, uint8 bnum ){
  int i;
  MRF_IF *mif = mrf_if_ptr(i_f);
  MRF_BUFF_STATE *mbst = _mrf_buff_state(bnum);
  IQUEUE *qp = &(mif->status.txqueue);
  //printf("mrf_if_tx_queue entry\n");
  if ( bnum >= _MRF_BUFFS)
    return -2;
  //printf("mrf_if_tx_queue 1\n");
  if (queue_push(qp,bnum) == 0)    {
    //_sys_ifs[i_f].status.state = MRF_ST_TXQ;
      mbst->owner = i_f;
      mbst->tx_timer = mif->type->tx_del;
      mbst->state = TXQUEUE;
      mrf_tick_enable();
      //printf("mrf_if_tx_queue OK\n");
      return 0;
      
    
  }
  else {
  // fall through if no space in queue
  _sys_ifs[i_f].status.stats.tx_overruns++;
  return -1;
  }
}
