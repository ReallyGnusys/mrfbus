#include "mrf_types.h"

#include "mrf_buff.h"
#include "mrf_if.h"
#include "mrf_debug.h"
#include "mrf_cmds.h"
#include "mrf_route.h"
uint8 _mrf_response_type(uint8 type){
  return type | 0x80;
}
/*
int mrf_tx_buffer(I_F i_f,uint8 *buff){
  //mrf_debug("mrf_tx_buffer : i_f %d  buff %p\n",i_f,buff);
  MRF_IF *if_ptr = mrf_if_ptr(i_f);
  (*(if_ptr->type->send_func))(i_f,buff);
  
}
*/
int mrf_tx_bnum(I_F i_f,uint8 bnum){
  //mrf_debug("mrf_tx_buff : i_f %d  bnum %d\n",i_f,bnum);
  MRF_IF *if_ptr = mrf_if_ptr(i_f);uint8 *buff = _mrf_buff_ptr(bnum);
  (*(if_ptr->type->send_func))(i_f,buff);
  
}

// segment ack buffer;

int mrf_sack(uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 mrf_nexthop(&route,MRFID,hdr->hsrc);
 MRF_IF *if_ptr = mrf_if_ptr(route.i_f);
 if_ptr->ackbuff.length = sizeof(MRF_PKT_HDR);
 if_ptr->ackbuff.hdest = hdr->hsrc;
 if_ptr->ackbuff.udest = hdr->hsrc;
 if_ptr->ackbuff.netid = MRFNET;
 if_ptr->ackbuff.type = mrf_cmd_ack;

 if_ptr->ackbuff.hsrc = MRFID;
 if_ptr->ackbuff.usrc = MRFID;
 if_ptr->ackbuff.msgid = hdr->msgid;
 if_ptr->status.acktimer =  if_ptr->type->tx_del;

 if_ptr->status.state = MRF_ST_ACKDELAY;
 mrf_tick_enable();
}

int mrf_retry(I_F i_f,uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 mrf_nexthop(&route,MRFID,hdr->hsrc);
 MRF_IF *if_ptr = mrf_if_ptr(route.i_f);
 if_ptr->ackbuff.length = sizeof(MRF_PKT_HDR);
 if_ptr->ackbuff.hdest = hdr->hsrc;
 if_ptr->ackbuff.udest = hdr->hsrc;
 if_ptr->ackbuff.netid = MRFNET;
 if_ptr->ackbuff.type = mrf_cmd_retry;
 if_ptr->ackbuff.hsrc = MRFID;
 if_ptr->ackbuff.usrc = MRFID;
 if_ptr->ackbuff.msgid = hdr->msgid;
 if_ptr->status.acktimer =  if_ptr->type->tx_del;
 _mrf_buff_free(bnum);
}

/*
// turn buffer around - ready to append response data
int mrf_respond_buffer(uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 //mrf_debug("mrf_data_response :  bnum %d  len %d\n",bnum,len);

 hdr->udest = hdr->usrc; 
 mrf_nexthop(&route,MRFID,hdr->udest);
 hdr->hdest = route.relay;
 hdr->usrc = MRFID;
 hdr->hsrc = MRFID;
 hdr->type = mrf_cmd_resp; //_mrf_response_type(hdr->type);
 MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));
 resp-> type = hdr->type;
 resp-> usrc = hdr->usrc;
 resp-> udest = hdr->udest;
 resp-> rlen = len;

}
*/
int mrf_data_response(uint8 bnum,uint8 *data,uint8 len){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 //mrf_debug("mrf_data_response :  bnum %d  len %d\n",bnum,len);

 // turning buffer around - deliver to usrc

 mrf_nexthop(&route,MRFID,hdr->usrc);

 MRF_IF *ifp = mrf_if_ptr(route.i_f);
 printf("mdr l0 -r.if %d  istate %d\n",route.i_f,ifp->status.state);

 if( mrf_if_tx_queue(route.i_f,bnum) == -1){ // then outgoing queue full - need to retry
   printf("mdr l0 i_f %d bnum %d\n",route.i_f,bnum);
   mrf_retry(route.i_f,bnum);
   
 }
 else
   {
     printf("mdr l1\n");
     // turnaround buffer and add response
     hdr->udest = hdr->usrc; 
     hdr->hdest = route.relay;
     hdr->usrc = MRFID;
     hdr->hsrc = MRFID;

     MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));
     resp-> type = hdr->type;
     resp-> usrc = hdr->usrc;
     resp-> udest = hdr->udest;
     resp-> rlen = len;
     hdr->type = mrf_cmd_resp; //_mrf_response_type(hdr->type);
     uint8 *dptr = ((uint8 *)resp)+ sizeof(MRF_PKT_RESP);
 
     int i;
     for ( i = 0 ; i < len ; i++ )
       *(dptr + i) = data[i];

     hdr->length = sizeof(MRF_PKT_HDR) + len;

   }

 return 0;
}
void _mrf_print_packet_header(MRF_PKT_HDR *hdr,I_F owner){
  MRF_IF *ifp = mrf_if_ptr(owner);

  uint8 type = hdr->type;
   mrf_debug("**************************************\n");

   mrf_debug("PACKET %s rx on I_F %d  state %d\n",mrf_cmds[hdr->type].str,owner,ifp->status.state);
  
   mrf_debug(" HSRC %02X HDEST %02X  LEN %02X  MSGID %02X   \n",hdr->hsrc,hdr->hdest,hdr->length,hdr->msgid);
   mrf_debug(" USRC %02X UDEST %02X  NETID %02X type %02X  \n",hdr->usrc,hdr->udest,hdr->netid,hdr->type);
   mrf_debug("**************************************\n");


}

int _mrf_process_packet(I_F owner,uint8 bnum)
{
  uint8 len;
  MRF_PKT_HDR *pkt;
  uint8 type;
  //mrf_debug("_mrf_process_packet: processing buff number %d our MRFID = %X \n",bnum,MRFID);

  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  type = pkt->type;
  _mrf_print_packet_header(pkt,owner);

  // check we are hdest 
  if ( pkt->hdest != MRFID)
    {
      mrf_debug("ERROR:  HDEST %02X is not us %02X - mrf_bus.pkt_error\n",pkt->hdest,MRFID);
      return -1;
    }
  if(type >= MRF_NUM_CMDS){
    mrf_debug("unsupported packed type 0x%02X\n",pkt->type);
    return -1;
  }

  MRF_IF *ifp = mrf_if_ptr(owner);
  // don't count ack and retry
  //  if(type < mrf_cmd_resp)
  // ifp->status.stats.rx_pkts++;
  
  // lookup command
  const MRF_CMD *cmd = (const MRF_CMD *) &(mrf_cmds[pkt->type]);

 
  // begin desperate
  
  // a response can substitute for an ack - if usrc and hsrc are same
  if (((pkt->type) == mrf_cmd_resp) && ((pkt->usrc) == (pkt->hsrc))){
    mrf_task_ack(pkt->type,bnum,ifp);

  }
  // end desperate

  // check if we are udest

  if ( pkt->udest == MRFID)
    {
      if(type >= mrf_cmd_resp)
        ifp->status.stats.rx_pkts++;
      

      //mrf_debug("INFO: EXECUTE PACKET UDEST %02X is us %02X \n",pkt->udest,MRFID);
      // printf("cmd name %s  len %d \n",cmd->str,cmd->size);
      if( ( cmd->data != NULL )  && ( cmd->rsp_size > 0 ) && ( (cmd->cflags & MRF_CFLG_NO_RESP) == 0)) {
        //printf("sending data response \n");
        mrf_data_response(bnum,cmd->data,cmd->rsp_size);
        return;

      }
      printf("pp l12\n");
      // check if command func defined
      if(cmd->func != NULL){
        (*(cmd->func))(pkt->type,bnum,ifp);
        return;
      }
      printf("pp l13\n");

      // send ack if allowed - no! no ack for final delivery - just response
      /*
      if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
        printf("pp l14\n");
        mrf_sack(owner,bnum);

      }
      printf("pp l15\n");
      */
    }
  else{
    //otherwise send segment ack then forward on network
    MRF_ROUTE route;
 
    mrf_nexthop(&route,MRFID,pkt->udest);
    MRF_IF *ifp = mrf_if_ptr(route.i_f);

    if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
      mrf_sack(bnum);   
    }
    if( mrf_if_tx_queue(route.i_f,bnum) == -1) // then outgoing queue full - need to retry
      mrf_retry(route.i_f,bnum);
    else{

      mrf_debug("INFO:  UDEST %02X : forwarding to %02X on I_F %d  st %d\n",pkt->udest,route.relay,route.i_f,ifp->status.state);
    
      pkt->hdest = route.relay;
      pkt->hsrc = MRFID;
  
    }
  }
  
}

int _tick_count;

void mrf_sys_init(){
  _tick_count = 0;
}


// can i_f start tx?
int _mrf_if_can_tx(IF_STATE istate){

  switch(istate){
  case  MRF_ST_WAITSACK:
  case  MRF_ST_ACKDELAY:
    return 0;

  default:
    return 1;  
  }
}



void _mrf_tick(){
 MRF_IF *mif; 
  I_F i;
  uint8 j;
  uint8 *tb;
  MRF_BUFF_STATE *bs;
  uint8 bnum;
  IF_STATE istate;
  int count = 0;
  uint8 if_busy = 0;
  IQUEUE *qp;
  _tick_count++;
  if ( (_tick_count % 1000 ) == 0)
    printf("%d\n",_tick_count);
  for ( i = 0 ; i < NUM_INTERFACES ; i++){
    mif = mrf_if_ptr(i);
    qp = &(mif->status.txqueue);
    istate = mif->status.state;

    // check if i_f busy
    if ( istate == MRF_ST_WAITSACK)
      if_busy = 1;
    if(queue_data_avail(qp))
      if_busy = 1;
      
    if ( istate == MRF_ST_ACKDELAY)
      {
        if_busy = 1;
        if ((mif->status.acktimer) > 0 ){
 
          mif->status.acktimer--;
          if((mif->status.acktimer) == 0){
            (*(mif->type->send_func))(i,(uint8 *)&(mif->ackbuff));
            printf("tick - send ack i_f %d  tc %d \n",i,_tick_count);
            mif->status.state = MRF_ST_RX;
          }
        }

      }
    else if (_mrf_if_can_tx(istate)) {
      //printf("mrf_sys can_tx i = %d - tc %d\n",i,_tick_count);
      if (queue_data_avail(qp))
        {

          if_busy = 1;

          bnum = queue_head(qp);
          if_busy = 1;

          bs = _mrf_buff_state(bnum);
          printf("\nmrf_tick : FOUND txqueue -IF = %d buffer is %d state %d tx_timer %d  tc %d\n",i,bnum,bs->state,bs->tx_timer,_tick_count);
     
          // if (_tick_count >10)
          //  exit(1);
          if ( bs->state == TXQUEUE ){
            printf("mrf_tick 11 \n");

        
            if (  (bs->tx_timer) == 0 ){
              tb = _mrf_buff_ptr(bnum);
            //printf("calling send func\n");
              printf("tick - send buff %d i_f %d tc %d\n",bnum,i,_tick_count);
              bs->state = TX;

              (*(mif->type->send_func))(i,tb);

              mif->status.state = MRF_ST_WAITSACK;
              bs->tx_timer = 0;
            
            // queue_pop(qp);
            //count++;
            }
            else {
              (bs->tx_timer) --;  
            //printf("tx_timer -> %d\n",bs->tx_timer);
            }
          }
        }
  
    } // end if can_tx 
  } // for i=0 to NUM_INTERFACES
  if (if_busy == 0 )
    {
      // all i_fs are idle - turn off tick

      printf("mrf_tick - turning off tick - if_busy = %d  tc = %d\n",if_busy,_tick_count);
      _mrf_if_print_all();
      mrf_tick_disable();
      
    }
  else{
    // printf("mrf_tick - keeping tick - if_busy = %d  tc = %d\n",if_busy,_tick_count);
    //_mrf_if_print_all();   
    
  }
}

