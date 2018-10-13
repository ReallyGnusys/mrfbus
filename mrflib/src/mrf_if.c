/* mrf bus interface */

// allocate interface structures


//#include <mrf_if.h>
//#include <mrf_buff.h>
#include <mrf_sys.h>
#include <mrf_debug.h>



extern const MRF_IF _sys_ifs[NUM_INTERFACES]; //[NUM_INTERFACES];

const MRF_IF *mrf_if_ptr(I_F i_f){
  if (i_f < NUM_INTERFACES)
    return &_sys_ifs[i_f];
  else
    return (MRF_IF *)NULL;
}


int mrf_if_can_sleep(I_F i_f){
  return 0;
}


int mrf_if_recieving(I_F i_f){
  //FIXME

  switch (mrf_if_ptr(i_f)->status->state)
    {
    case MRF_ST_IDLE:         return TRUE;
    case MRF_ST_WAIT_ACK:    return TRUE;
    default:                 return FALSE;
    }
}

int mrf_if_transmitting(I_F i_f){
  switch (mrf_if_ptr(i_f)->status->state)
    {
    case MRF_ST_TX:         return TRUE;
    case MRF_ST_TX_FOR_ACK: return TRUE;
    default:                return FALSE;
    }
}



void mrf_if_init(){
  unsigned int i,j;
  uint8 *dptr;
  //mrf_debug("mrf_if_init entry NUM_INTERFACES %d \n",NUM_INTERFACES);
  for (i = 0 ; i < NUM_INTERFACES ; i++){
    //mrf_debug("interface %d\n",i);
    const MRF_IF *mif = mrf_if_ptr((I_F)i);
    // rough zeroing of status data including stats
    dptr = (uint8 *)mif->status;
    //mrf_debug("dptr = %p sizeof IF_STATUS %lu\n",dptr,sizeof(IF_STATUS));
    for ( j = 0 ; j < sizeof(IF_STATUS) ; j++)
      dptr[j] = 0;
    queue_init(&(mif->status->txqueue));
    mif->status->state = MRF_ST_IDLE;
#ifdef MRF_ARCH_lnx
    *(mif->fd) =  (*(mif->type->funcs.init))((I_F)i); //needed for epoll
#else
    (*(mif->type->funcs.init))((I_F)i);
#endif

  }

}

/*
void mrf_if_register(I_F i_f,const MRF_IF_TYPE *type){
  _sys_ifs[i_f].type = type;

}
*/

void _mrf_if_print_all(){

  int i;
  const MRF_IF *ifp;
  for ( i = 0 ; i < NUM_INTERFACES ; i++){
    ifp = mrf_if_ptr((I_F)i);
    mrf_debug("I_F %d state %d txq_da %d\n",i,ifp->status->state,queue_data_avail(&(ifp->status->txqueue)));
  }
}

void mrf_if_tx_done(I_F i_f){
  const MRF_IF *mif = mrf_if_ptr(i_f);
  if (mif->status->state == MRF_ST_ACK) {
    mrf_debug("mrf_if_tx_done I_F %d state %d from MRF_ST_ACK to IDLE \n",i_f,mif->status->state);
    mif->status->state = MRF_ST_IDLE;

  }

  else if (mif->status->state == MRF_ST_TX) {
    mrf_debug("mrf_if_tx_done I_F %d state %d from MRF_ST_TX to TX_COMPLETE \n",i_f,mif->status->state);
    mif->status->state = MRF_ST_TX_COMPLETE;

  }
  else if(mif->status->state == MRF_ST_TX_FOR_ACK) {
    mrf_debug("mrf_if_tx_done I_F %d state %d from MRF_ST_TX_FOR_ACK to WAIT_ACK \n",i_f,mif->status->state);
    mif->status->state = MRF_ST_WAIT_ACK;
  }
  else{

    mrf_debug("ERROR!!! mrf_if_tx_done I_F %d state %d \n",i_f,mif->status->state);

  }
}

int8 mrf_if_tx_queue(I_F i_f, uint8 bnum ){
  const MRF_IF *mif = mrf_if_ptr(i_f);
  MRF_BUFF_STATE *mbst = _mrf_buff_state(bnum);
  IQUEUE *qp = &(mif->status->txqueue);
  //mrf_debug("mrf_if_tx_queue entry\n");
  if ( bnum >= _MRF_BUFFS)
    return -2;
  //mrf_debug("mrf_if_tx_queue 1\n");
  if (queue_push(qp,bnum) == 0)    {
    //_sys_ifs[i_f].status->state = MRF_ST_TXQ;
      mbst->owner = i_f;
      mbst->tx_timer = mif->type->tx_del;  // FIXME shouldn't need any of this
      mbst->state = TXQUEUE;
      mbst->retry_count = 0;
      mrf_tick_enable();
      mrf_debug("mrf_if_tx_queue OK i_f %d buff %d retry_count %d\n",i_f,bnum,mbst->retry_count);
      return 0;
  }
  else {
  // fall through if no space in queue
    mrf_if_ptr(i_f)->status->stats.tx_overruns++;
  return -1;
  }
}
