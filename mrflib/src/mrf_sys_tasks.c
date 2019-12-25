/******************************************************************************
*
* Copyright (c) 2012-16 Gnusys Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************************/

#include <mrf_sys.h>
//#include <stdio.h>
#include <mrf_debug.h>


// mrf_task_ack
// run whenever ack,retry or resp recieved from hdest
// 1) pops tx queue if msgid matches head head msgid
// 2) frees buffer if not resp


// buffer_responded - checks to see if ack/retry/resp relates to
// txqueue head -  if so removes txqueue entry and returns responded buffer num
// else _MRF_BUFFS

uint8 buffer_responded(uint8 bnum, const MRF_IF *ifp){
  uint8 bn;

  BuffQueue *qp = ifp->txqueue;

  MRF_PKT_HDR *ackhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L);
  MRF_PKT_HDR *txhdr;
  MRF_BUFF_STATE *bs;

  uint8 i = ifp->i_f;
  if ( ifp->status->resp_timer == 0){
    mrf_debug(5,"ERROR: buffer_responded called when ifp resp_timer was %d\n",ifp->status->resp_timer);

  }
  if (!qp->data_avail()){
    mrf_debug(5,"buffer_responded, error nothing was queuing for ifp %p\n",ifp);
    return _MRF_BUFFS;
  }
  // clear buff state of ack buffer
  bn = *(qp->head());
  bs = _mrf_buff_state(bn);
  txhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bn)+ 0L);
  mrf_debug(5," qhead is %d state %d - if state %d header:\n",bn,bs->state,ifp->status->state);
  mrf_print_packet_header(txhdr);

  //FIXME really should do some more checks here on src dest addresses
  if((txhdr->msgid == ackhdr->msgid) && (txhdr->hdest == ackhdr->hsrc))
    {

      mrf_debug(5,"buffer_responded : acknowledge recieved for buffer %d \n",bn);

      if(bs->state != TX){

        mrf_debug(5,"WARNING buffer_responded : acknowledge recieved for buffer %d but bs_state %s \n",bn,mrf_buff_state_name(bn));

      }
      ifp->status->resp_received = 1;
      //mrf_if_print_info((I_F)i);
      //ifp->status->state = MRF_ST_TX_COMPLETE;
      return bn;
    }
  mrf_debug(5,"buffer_responded returning not found for bnum %u msgid %d ackid %d\n",bnum,txhdr->msgid,ackhdr->msgid);
  mrf_debug(5,"buff %d state %d (%s) owner %d i_f state %d \n",bnum,bs->state,mrf_buff_state_name(bnum),bs->owner,ifp->status->state);

  return _MRF_BUFFS;


}


extern MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  uint8 txbuff;
  MRF_PKT_HDR *hdr;

  mrf_debug(5,"mrf_task_ack : bnum %d  IF state %d\n",bnum,ifp->status->state);


  txbuff = buffer_responded(bnum, ifp);

  if (txbuff < _MRF_BUFFS ){

    hdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(txbuff)+ 0L);
    mrf_debug(5,"mrf_task_ack.. ack matched to txbuff %d usrc %u\n",txbuff,hdr->usrc);

    //ifp->status->stats.tx_pkts++;
    //_mrf_buff_free(txbuff);


  } else {

    mrf_debug(5,"%s","mrf_task_retry.. unexpected retry \n");
    ifp->status->stats.unexp_ack++;

  }

  _mrf_buff_free(bnum);

  return MRF_CMD_RES_OK;
}

// Retry command received
// need to queue an NDR to original sender when receive this

// need to stop further immediate retries

MRF_CMD_RES mrf_task_retry(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  uint8 txbuff;
  MRF_PKT_HDR *hdr;

  txbuff = buffer_responded(bnum, ifp);
  if (txbuff < _MRF_BUFFS ){
    hdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(txbuff)+ 0L);
    mrf_debug(5,"mrf_task_retry.. matched to txbuff %d usrc %u\n",txbuff,hdr->usrc);

    ifp->status->tx_retried;

    if(hdr->usrc != MRFID)   // if we weren't initiator then send ndr to initiator
      mrf_ndr(NDR_RECD_SRETRY, txbuff);


  } else {

    mrf_debug(5,"mrf_task_retry.. unexpected retry , txbuff %d\n",txbuff);

  }
  _mrf_buff_free(bnum);

  return MRF_CMD_RES_OK;

}

MRF_CMD_RES mrf_task_ndr(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  // recieved an NDR -  normally only expect server/root to care about this

  ifp->status->stats.rx_ndr++;

  _mrf_buff_free(bnum);

  return MRF_CMD_RES_OK;
}



int _print_mrf_cmd(MRF_CMD_CODE cmd);

MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug(5,"mrf_task_resp cmdcode %u bnum %u\n",cmd,bnum);
  _mrf_buff_print();

  mrf_debug(5,"%s","mrf_task_resp L1\n");


  mrf_debug(5,"%s","pushing to app queue as MRF_CMD_USR_RESP\n");
  // now put in application queue as MRF_CMD_USR_RESP

  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!
  //MRF_PKT_RESP *rsp1 = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+ sizeof(MRF_PKT_HDR));
  //mrf_debug(5,"resp->rlen %u resp->type %u resp->msgid %u\n",rsp1->rlen,rsp1->type,rsp1->msgid);
  hdr1->type = mrf_cmd_usr_resp;
  mrf_app_queue_push(bnum);
  return MRF_CMD_RES_OK;

}

extern uint32 _tick_count;

MRF_CMD_RES mrf_task_device_status(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug(5,"%s","mrf_task_device_status\n");

  MRF_PKT_DEVICE_STATUS info;
  //info.num_if = NUM_INTERFACES;
  info.errors = 0;
  info.tx_retries = 0;
  info.tx_pkts = 0;
  info.rx_pkts = 0;
  //info.buffs_total = _MRF_BUFFS;
  info.buffs_free  = mrf_buff_num_free();
  info.tick_count = _tick_count;
  uint8_t i;
  const MRF_IF *i_f;
  for ( i = 0 ; i < NUM_INTERFACES ; i++ ) {
    i_f = mrf_if_ptr((I_F)i);
    info.tx_retries += i_f->status->stats.tx_retries;
    info.tx_pkts    += i_f->status->stats.tx_pkts;
    info.rx_pkts    += i_f->status->stats.rx_pkts;
  }
  mrf_debug(5,"%s","mrf_task_if_info l1\n");

  mrf_data_response( bnum,(uint8 *)&info,sizeof(MRF_PKT_DEVICE_STATUS));
  mrf_debug(5,"%s","mrf_task_if_info exit\n");
  return MRF_CMD_RES_OK;


}
MRF_CMD_RES mrf_task_if_status(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  MRF_PKT_UINT8 *streq = (MRF_PKT_UINT8 *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  mrf_debug(5,"mrf_task_if_status request for i_f %d\n",streq->value);

  if ( (streq->value) >= NUM_INTERFACES)
    return MRF_CMD_RES_ERROR;
  const MRF_IF *i_f = mrf_if_ptr((I_F)streq->value);

  mrf_data_response( bnum,(uint8 *)&i_f->status->stats,sizeof(IF_STATS));
  return MRF_CMD_RES_OK;

}


MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  TIMEDATE td;
  mrf_rtc_get(&td);
  mrf_debug(5,"%s","mrf_task_get_time \n");
  mrf_debug(5,"TIMEDATE : %u-%u-%u  %u:%u:%u \n",td.day,td.mon,td.year,td.hour,td.min,td.sec);
  /*
  mrf_debug(5,"hex buff follows:");
  _mrf_print_hex_buff((uint8 *)&td,sizeof(TIMEDATE));
  mrf_debug(5,":end of hex");
  */
  mrf_data_response( bnum,(uint8 *)&td,sizeof(TIMEDATE));
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_task_set_time(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  TIMEDATE *td;
  mrf_debug(5,"%s","mrf_task_set_time entry\n");
  td = (TIMEDATE *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  mrf_rtc_set(td);
  mrf_data_response( bnum,(uint8 *)td,sizeof(TIMEDATE));
  //mrf_data_response( bnum,"TIME IS xx",sizeof("TIME IS xx"));
  return MRF_CMD_RES_OK;
}


MRF_CMD_RES mrf_task_buff_state(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_UINT8 *streq = (MRF_PKT_UINT8 *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  mrf_debug(5,"mrf_task_buff_state request for buffer %d\n",streq->value);
  if ( (streq->value) >= _MRF_BUFFS)
    return MRF_CMD_RES_ERROR;
  MRF_PKT_BUFF_STATE buffst;
  buffst.id = streq->value;
  MRF_BUFF_STATE *bstp = _mrf_buff_state(streq->value);
  mrf_copy( (void *)bstp,(void *)&buffst.state, sizeof(MRF_BUFF_STATE));
  mrf_data_response( bnum,(uint8 *)&buffst,sizeof(MRF_PKT_BUFF_STATE));
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_task_cmd_info(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_UINT8 *streq = (MRF_PKT_UINT8 *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  mrf_debug(5,"mrf_task_cmd_info request for cmd %d\n",streq->value);
  if ( (streq->value) >= MRF_NUM_SYS_CMDS)
    return MRF_CMD_RES_ERROR;
  MRF_PKT_CMD_INFO *cinfo = (MRF_PKT_CMD_INFO *)mrf_response_buffer(bnum);
  const MRF_CMD * cmdp = mrf_cmd_ptr(streq->value);
  mrf_debug(5,"think cmdp->str is %s\n",cmdp->str);
  cinfo->type = streq->value;
  cinfo->cflags = cmdp->cflags;
  cinfo->req_size = cmdp->req_size;
  cinfo->rsp_size = cmdp->rsp_size;
  mrf_scopy((void *)cmdp->str,(void *)cinfo->name,16); // FIXME should have some const for this
  mrf_send_response(bnum,sizeof(MRF_PKT_CMD_INFO));
  return MRF_CMD_RES_OK;
}

//extern const MRF_CMD mrf_app_cmds[];

MRF_CMD_RES mrf_task_app_cmd_info(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_UINT8 *streq = (MRF_PKT_UINT8 *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  mrf_debug(5,"mrf_task_app_cmd_info request for cmd %d\n",streq->value);
  if ( (streq->value) >= MRF_NUM_APP_CMDS)
    return MRF_CMD_RES_ERROR;
  MRF_PKT_CMD_INFO *cinfo = (MRF_PKT_CMD_INFO *)mrf_response_buffer(bnum);
  const MRF_CMD * cmdp = mrf_app_cmd_ptr ( streq->value) ;
  mrf_debug(5,"think cmdp->str is %s\n",cmdp->str);
  cinfo->type   = streq->value + _MRF_APP_CMD_BASE;
  cinfo->cflags = cmdp->cflags;
  cinfo->req_size = cmdp->req_size;
  cinfo->rsp_size = cmdp->rsp_size;
  mrf_scopy((void *)cmdp->str,(void *)cinfo->name,16); // FIXME should have some const for this
  mrf_send_response(bnum,sizeof(MRF_PKT_CMD_INFO));
  return MRF_CMD_RES_OK;
}


MRF_CMD_RES mrf_task_test_1(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug(5,"%s","mrf_task_test_1 entry\n");
  TIMEDATE *rbuff = (TIMEDATE *)mrf_response_buffer(bnum);
  mrf_rtc_get(rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug(5,"%s","mrf_task_test_1 exit\n");
  return MRF_CMD_RES_OK;
}


void mrf_reset();


MRF_CMD_RES mrf_task_reset(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){


  mrf_reset();  // must be defined by arch

  return MRF_CMD_RES_OK;  // ho ho

}

MRF_CMD_RES mrf_task_ping(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){

  MRF_PKT_PING_RES *pres = (MRF_PKT_PING_RES *)mrf_response_buffer(bnum);
  // lqi and rssi follow packet - just header with no payload for ping
  uint8 *bptr = (uint8 *)((uint8*)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  pres->to_rssi =  *(bptr);
  pres->to_lqi =  *(bptr + 1);
  mrf_send_response(bnum,sizeof(MRF_PKT_PING_RES));

  return MRF_CMD_RES_OK;

}
