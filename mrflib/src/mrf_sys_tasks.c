#include <mrf_sys.h>
//#include <stdio.h>
#include <mrf_sys_cmds.h>
#include <mrf_debug.h>

extern const MRF_CMD const *mrf_sys_cmds;



MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_ack : bnum %d  IF state %d\n",bnum,ifp->status->state);

  uint8 bn;

  IQUEUE *qp = &(ifp->status->txqueue);

  MRF_PKT_HDR *ackhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L);
  MRF_PKT_HDR *txhdr; 
  MRF_BUFF_STATE *bs;




  if ( ifp->status->state != MRF_ST_WAITSACK){  
    mrf_debug("mrf_task_ack: unexpected ack for i_f\n");
    ifp->status->stats.unexp_ack++;
  //_mrf_buff_state(bnum)->state = FREE;
    if (ackhdr->type == mrf_cmd_ack) {// don't free for resp
      mrf_debug("mrf_task_ack - buffer %d contains ack .. freeing\n",bnum);
      _mrf_buff_free(bnum);
    }
    return MRF_CMD_RES_ERROR;
  }
  mrf_debug("ack 1\n");
  if (ackhdr->type == mrf_cmd_ack) {// don't free for resp .. hmpff
    mrf_debug("mrf_task_ack - buffer %d contains ack .. freeing\n",bnum);
  //_mrf_buff_state(bnum)->state = FREE;
    _mrf_buff_free(bnum);
  }
  // clear buff state of ack buffer
  if (queue_data_avail(qp)){
      bn = queue_head(qp);
      bs = _mrf_buff_state(bn);
      txhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bn)+ 0L);
      mrf_debug(" qhead is %d state %d\n",bn,bs->state);
      if((bs->state == TX) &&((txhdr->msgid) == (ackhdr->msgid)))
        {
        mrf_debug("acknowledge recieved for buffer %d \n",bn);
        queue_pop(qp);
        ifp->status->state = MRF_ST_RX;
        ifp->status->stats.tx_pkts++;
        _mrf_buff_free(bn);
        //bs->state = FREE;
        return;
        }
      else{
        mrf_debug("no ack 1\n");
      }
  }
  mrf_debug("i_f status %d da %d\n",ifp->status->state,queue_data_avail(&(ifp->status->txqueue)));
  mrf_debug("no ack 2\n");
}

MRF_CMD_RES mrf_task_retry(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_retry..doing nothing yet\n");

}
int _print_mrf_cmd(MRF_CMD_CODE cmd);

MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_resp\n");
  _mrf_buff_print();
  
  mrf_debug("mrf_task_resp L1\n");


  mrf_debug("sending sack\n");
   // send ack
  mrf_sack(bnum);


  mrf_debug("sack sent\n");

  // now put in application queue as MRF_CMD_USR_RESP
  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!
  hdr1->type = mrf_cmd_usr_resp;
  mrf_app_queue_push(bnum);
}

MRF_CMD_RES mrf_task_device_status(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_device_status\n");

  MRF_PKT_DEVICE_STATUS info;
  info.num_if = NUM_INTERFACES;
  info.errors = 0;
  info.tx_retries = 0;
  info.tx_pkts = 0;
  info.rx_pkts = 0;
  info.buffs_total = _MRF_BUFFS;
  info.buffs_free  = mrf_buff_num_free();
  I_F i;
  uint32 rxp,txp;
  MRF_IF *i_f;
  for ( i = 0 ; i < NUM_INTERFACES ; i++ ) {
    i_f = mrf_if_ptr(i);
    info.tx_retries += i_f->status->stats.tx_retries;
    info.tx_pkts += i_f->status->stats.tx_pkts;
    info.rx_pkts += i_f->status->stats.rx_pkts; 
  }
  mrf_debug("mrf_task_if_info l1\n");

  mrf_data_response( bnum,(uint8 *)&info,sizeof(MRF_PKT_DEVICE_STATUS));  
  mrf_debug("mrf_task_if_info exit\n");

}
MRF_CMD_RES mrf_task_if_status(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){

  MRF_PKT_UINT8 *streq = (MRF_PKT_UINT8 *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  mrf_debug("mrf_task_if_status request for i_f %d\n",streq->value);

  if ( (streq->value) >= NUM_INTERFACES)
    return MRF_CMD_RES_ERROR;
  MRF_IF *i_f = mrf_if_ptr(streq->value);

  mrf_data_response( bnum,(uint8 *)&i_f->status->stats,sizeof(IF_STATS));  
  return MRF_CMD_RES_OK;

}




MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  TIMEDATE td;
  mrf_rtc_get(&td);
  mrf_debug("mrf_task_get_time \n");
  mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td.day,td.mon,td.year,td.hour,td.min,td.sec);
  /*
  mrf_debug("hex buff follows:");
  _mrf_print_hex_buff((uint8 *)&td,sizeof(TIMEDATE));
  mrf_debug(":end of hex");
  */
  mrf_data_response( bnum,&td,sizeof(TIMEDATE));  
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_task_set_time(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  TIMEDATE *td,endtd;
  mrf_debug("mrf_task_set_time entry\n");
  td = (TIMEDATE *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  mrf_rtc_set(td);
  
  mrf_rtc_get(&endtd);

  mrf_data_response( bnum,&endtd,sizeof(TIMEDATE));  

  //mrf_data_response( bnum,"TIME IS xx",sizeof("TIME IS xx"));  
  return MRF_CMD_RES_OK;  

}


MRF_CMD_RES mrf_task_buff_state(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){

  MRF_PKT_UINT8 *streq = (MRF_PKT_UINT8 *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  mrf_debug("mrf_task_buff_state request for buffer %d\n",streq->value);

  if ( (streq->value) >= _MRF_BUFFS)
    return MRF_CMD_RES_ERROR;
  MRF_PKT_BUFF_STATE buffst;
  buffst.id = streq->value;
  MRF_BUFF_STATE *bstp = _mrf_buff_state(streq->value);
  mrf_copy( (void *)bstp,(void *)&buffst.state, sizeof(MRF_BUFF_STATE));
  mrf_data_response( bnum,(uint8 *)&buffst,sizeof(MRF_PKT_BUFF_STATE));  
  return MRF_CMD_RES_OK;

}



MRF_CMD_RES mrf_task_sensor_data(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_sensor_data exit\n");
  mrf_data_response( bnum,"TIME IS xx",sizeof("TIME IS xx"));  
  return MRF_CMD_RES_OK;  

}

MRF_CMD_RES mrf_task_read_sensor(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_get_sensor_data exit\n");
  mrf_data_response( bnum,"TIME IS xx",sizeof("TIME IS xx"));  
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_task_test_1(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_test_1 entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get(rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug("mrf_task_test_1 exit\n");
  return MRF_CMD_RES_OK;  
}

MRF_CMD_RES mrf_task_test_2(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_test_2\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
 
  mrf_data_response( bnum,"TIME IS xx",sizeof("TIME IS xx"));  
  return MRF_CMD_RES_OK;  
}
