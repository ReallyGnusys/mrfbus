#include "mrf_types.h"

#include "mrf_buff.h"
#include "mrf_if.h"
#include "mrf_debug.h"
#include "mrf_cmds.h"
#include "mrf_route.h"
uint8 _mrf_response_type(uint8 type){
  return type | 0x80;
}

int mrf_tx_buffer(I_F i_f,uint8 *buff){
  //mrf_debug("mrf_tx_buffer : i_f %d  buff %p\n",i_f,buff);
  MRF_IF *if_ptr = mrf_if_ptr(i_f);
  (*(if_ptr->type->send_func))(i_f,buff);
  
}
int mrf_tx_bnum(I_F i_f,uint8 bnum){
  //mrf_debug("mrf_tx_buff : i_f %d  bnum %d\n",i_f,bnum);
  MRF_IF *if_ptr = mrf_if_ptr(i_f);uint8 *buff = _mrf_buff_ptr(bnum);
  (*(if_ptr->type->send_func))(i_f,buff);
  
}

// segment ack buffer;

int mrf_sack(I_F i_f,uint8 bnum){
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
 if_ptr->status.acktimer =  if_ptr->type->tx_del;;
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
 if_ptr->status.acktimer =  if_ptr->type->tx_del;;
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

 hdr->udest = hdr->usrc; 
 mrf_nexthop(&route,MRFID,hdr->udest);
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
 
 if( mrf_if_tx_queue(route.i_f,bnum) == -1) // then outgoing queue full - need to retry
   mrf_retry(route.i_f,bnum);


 return 0;
}
void _mrf_print_packet_header(MRF_PKT_HDR *hdr){
  
  uint8 type = hdr->type;
   mrf_debug("**************************************\n");

  mrf_debug("PACKET %s\n",mrf_cmds[hdr->type].str);
  
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
  _mrf_print_packet_header(pkt);

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
  ifp->status.stats.rx_pkts++;

  // lookup command
  const MRF_CMD *cmd = (const MRF_CMD *) &(mrf_cmds[pkt->type]);


  // check if we are udest

  if ( pkt->udest == MRFID)
    {
      //mrf_debug("INFO: EXECUTE PACKET UDEST %02X is us %02X \n",pkt->udest,MRFID);
      // printf("cmd name %s  len %d \n",cmd->str,cmd->size);
      if( ( cmd->data != NULL )  && ( cmd->rsp_size > 0 ) && ( (cmd->cflags & MRF_CFLG_NO_RESP) == 0)) {
        //printf("sending data response \n");
  
        mrf_data_response(bnum,cmd->data,cmd->rsp_size);
        return;

      }
      // check if command func defined
      if(cmd->func != NULL){
        (*(cmd->func))(pkt->type,bnum);
      }

      // send ack if allowed
      if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
        mrf_sack(owner,bnum);

      }
      
    }
  else{
    //otherwise send segment ack then forward on network

    if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
      mrf_sack(owner,bnum);
    }
    MRF_ROUTE route;

    mrf_nexthop(&route,MRFID,pkt->udest);
    mrf_debug("INFO:  UDEST %02X : forwarding to %02X on I_F %d \n",pkt->udest,route.relay,route.i_f);
    
    pkt->hdest = route.relay;
    pkt->hsrc = MRFID;
  
    mrf_tx_bnum(route.i_f,bnum);

  }
  
}

int _tick_count;

void mrf_sys_init(){
  _tick_count = 0;
}
void _mrf_tick(){

  // check each i_f

 
  MRF_IF *mif; 
  I_F i;
  uint8 j;
  uint8 *tb;
  MRF_BUFF_STATE *bs;
  uint8 bnum;
  int count = 0;

  // check each i_f for ack queued
  for ( i = 0 ; i < NUM_INTERFACES ; i++){
    mif = mrf_if_ptr(i);
    
    if ((mif->status.acktimer) <= 0 ){
      count++;
      continue;

    }
    
    mif->status.acktimer--;
    if((mif->status.acktimer) == 0){
      (*(mif->type->send_func))(i,(uint8 *)&(mif->ackbuff));
      count++;
    }
    
  }


  // check each i_f for tx queued
  for ( i = 0 ; i < NUM_INTERFACES ; i++){
    mif = mrf_if_ptr(i);
    
    //printf("I_F %d txq = ",i);
    for ( j = 0 ; j < MRF_IF_QL ; j++){
      bnum = mif->status.txqueue[j];
      //printf(" %02X",bnum);
      if (bnum != MRF_BUFF_NONE) {     

        
        bs = _mrf_buff_state(bnum);
        //printf("\nmrf_tick : FOUND txqueue -IF = %d buffer is %d j = %d tx_timer %d \n",i,bnum,j,bs->tx_timer);

        
        if (  (bs->tx_timer) == 0 ){
          tb = _mrf_buff_ptr(bnum);
          //printf("calling send func\n");
          (*(mif->type->send_func))(i,tb);
          bs->tx_timer = 0;
          mif->status.txqueue[j] = MRF_BUFF_NONE;
          count++;
        }
        else {
          (bs->tx_timer) --;  
          //printf("tx_timer -> %d\n",bs->tx_timer);

        }
        continue;

        // check buff count 
        
      }
    }
    //printf("\n");
    if ( j== MRF_IF_QL){
      count++;
      //printf("IF %d empty txq\n",i);
    }
     
  }

 
  


  if ( count == NUM_INTERFACES * 2)  // all interfaces quiet
    {
      //printf("mrf_tick : count = %d - stopping tick\n",count);
      mrf_tick_disable();
    }
  /*
  if( ( (_tick_count++) % 1000 ) == 0){

    printf("_mrf_tick %d count %d\n",_tick_count,count);
  }
  else*/ 
  _tick_count ++;
  //if (_tick_count < 10)
  //printf("_mrf_tick %d count %d NUM_INTERFACES %d\n",_tick_count,count,NUM_INTERFACES);
 

}
