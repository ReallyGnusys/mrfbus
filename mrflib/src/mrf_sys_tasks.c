#include <mrf_sys.h>
//#include <stdio.h>
#include <mrf_sys_cmds.h>
#include <mrf_debug.h>




MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_ack : bnum %d  IF state %d\n",bnum,ifp->status->state);

  uint8 bn;



  IQUEUE *qp = &(ifp->status->txqueue);

  MRF_PKT_HDR *ackhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L);
  MRF_PKT_HDR *txhdr; 
  MRF_BUFF_STATE *bs;

  // clear buff state of ack buffer
  _mrf_buff_state(bnum)->state = FREE;

  if ( ifp->status->state != MRF_ST_WAITSACK){  
    mrf_debug("mrf_task_ack: unexpected ack for i_f ");
    ifp->status->stats.unexp_ack++;
    return MRF_CMD_RES_ERROR;
  }
  mrf_debug("ack 1\n");
  if (queue_data_avail(qp)){
      bn = queue_head(qp);
      bs = _mrf_buff_state(bn);
      txhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bn)+ 0L);
      mrf_debug(" qhead is %d state %d\n",bn,bs->state);
      if((bs->state == TX) &&((txhdr->msgid) == (ackhdr->msgid)))
        {
        mrf_debug("acknowledge recieved buffer %d \n",bnum);
        queue_pop(qp);
        ifp->status->state = MRF_ST_RX;
        ifp->status->stats.tx_pkts++;
        bs->state = FREE;
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

MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_resp\n");


  // send ack
  mrf_sack(bnum);

  MRF_PKT_HDR *hdr1;
  // hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+0);
  hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???! //sizeof(MRF_PKT_HDR));

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  // send a seg ack when we get resp
  mrf_debug("Response to packet %s len %d usrc %02X  udest %02X\n",mrf_sys_cmds[resp->type].str,resp->rlen,hdr1->usrc,hdr1->udest);

  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE %s  version %s mrfbus version %s\n",dev_inf->dev_name,dev_inf->dev_version,dev_inf->mrfbus_version);
  }
  else if  (resp->type == mrf_cmd_if_info){
    MRF_PKT_IF_INFO *if_inf = (MRF_PKT_IF_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("IF INFO  rx_pkts %u tx_pkts %u tx_retries %u \n",if_inf->rx_pkts,if_inf->tx_pkts,if_inf->tx_retries);
  }
  else if  (resp->type == mrf_cmd_if_status){
    IF_STATUS *if_status = (IF_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("IF STATUS : state %d rx_pkts %u tx_pkts %u tx_retries %u \n",if_status->state,if_status->stats.rx_pkts,if_status->stats.tx_pkts,if_status->stats.tx_retries);
  }  
  else if  (resp->type == mrf_cmd_get_time){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
  }  

  else if  (resp->type == mrf_cmd_test_1){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
  }  
  else if  (resp->type == mrf_cmd_test_2){
    char *buff = (char *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TEST_2 : %s \n",buff);
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)buff,sizeof(MRF_PKT_DBG_CHR32));
    mrf_debug(":end of hex");
  }  



  _mrf_buff_state(bnum)->state = FREE;

}

MRF_CMD_RES mrf_task_if_info(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_if_info\n");

  MRF_PKT_IF_INFO info;
  info.num_if = NUM_INTERFACES;
  info.errors = 0;
  info.tx_retries = 0;
  info.tx_pkts = 0;
  info.rx_pkts = 0;

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

  mrf_data_response( bnum,(uint8 *)&info,sizeof(MRF_PKT_IF_INFO));  
  mrf_debug("mrf_task_if_info exit\n");

}
MRF_CMD_RES mrf_task_if_status(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){

  MRF_PKT_IF_STAT_REQ *streq = (MRF_PKT_IF_STAT_REQ *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  mrf_debug("mrf_task_if_status request for i_f %d\n",streq->i_f);

  if ( (streq->i_f) >= NUM_INTERFACES)
    return MRF_CMD_RES_ERROR;
  MRF_IF *i_f = mrf_if_ptr(streq->i_f);

  mrf_data_response( bnum,(uint8 *)&i_f->status,sizeof(IF_STATUS));  
  return MRF_CMD_RES_OK;

}
MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  TIMEDATE td;
  mrf_rtc_get(&td);
  mrf_debug("mrf_task_get_time \n");
  mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td.day,td.mon,td.year,td.hour,td.min,td.sec);

  mrf_debug("hex buff follows:");
  _mrf_print_hex_buff((uint8 *)&td,sizeof(TIMEDATE));
  mrf_debug(":end of hex");
  mrf_data_response( bnum,&td,sizeof(TIMEDATE));  
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_task_set_time(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_task_set_time exit\n");
  mrf_data_response( bnum,"TIME IS xx",sizeof("TIME IS xx"));  
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
