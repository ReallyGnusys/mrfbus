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
    _sys_ifs[i].status.state = MRF_ST_IDLE;
    _sys_ifs[i].status.tx_status = MRF_TX_IDLE;
    for ( j = 0 ; j < MRF_IF_QL ; j++)
      _sys_ifs[i].status.txqueue[j] = MRF_BUFF_NONE;

  }

}

void mrf_if_register(I_F i_f, MRF_IF_TYPE *type){
  _sys_ifs[i_f].type = type;
  
}

int8 mrf_if_tx_queue(I_F i_f, uint8 bnum ){
  int i;
  MRF_IF *mif = mrf_if_ptr(i_f);
  MRF_BUFF_STATE *mbst = _mrf_buff_state(bnum);
  //printf("mrf_if_tx_queue entry\n");
  if ( bnum >= _MRF_BUFFS)
    return -1;
  //printf("mrf_if_tx_queue 1\n");
  for (i = 0 ; i < MRF_IF_QL ; i++){
    if ( _sys_ifs[i_f].status.txqueue[i] == MRF_BUFF_NONE){
      _sys_ifs[i_f].status.txqueue[i] = bnum;
      
      mbst->owner = i_f;
      mbst->tx_timer = mif->type->tx_del;
      mrf_tick_enable();
      //printf("mrf_if_tx_queue OK\n");
      return 0;
      
    } 
  }
  // fall through if no space in queue
  _sys_ifs[i_f].status.stats.tx_overruns++;
  return -1;

}
