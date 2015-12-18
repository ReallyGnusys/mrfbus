#include "mrf_types.h"

#include "mrf_buff.h"
#include "mrf_if.h"
#include "mrf_debug.h"
#include "mrf_sys_structs.h"
#include "mrf_route.h"
#include "mrf_sys_cmds.h"

#define _MRF_TX_TIMEOUT 10
#define _MRF_MAX_RETRY 4
extern uint8 _mrfid;

static IQUEUE _app_queue;
extern const MRF_CMD mrf_sys_cmds[MRF_NUM_SYS_CMDS];
uint8 _mrf_response_type(uint8 type){
  return type | 0x80;
}

int mrf_tx_bnum(I_F i_f,uint8 bnum){
  //mrf_debug("mrf_tx_buff : i_f %d  bnum %d\n",i_f,bnum);
  MRF_IF *if_ptr = mrf_if_ptr(i_f);
  uint8 *buff = _mrf_buff_ptr(bnum);
  (*(if_ptr->type->funcs.send))(i_f,buff);
  
}

// send segment ack for buffer
int mrf_sack(uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 mrf_nexthop(&route,_mrfid,hdr->hsrc);
 mrf_debug("mrf_sack : orig header is ");
 mrf_print_packet_header(hdr);
 mrf_debug("route if is %d",route.i_f);
 MRF_IF *if_ptr = mrf_if_ptr(route.i_f);
 if_ptr->ackbuff->length = sizeof(MRF_PKT_HDR);
 if_ptr->ackbuff->hdest = hdr->hsrc;
 if_ptr->ackbuff->udest = hdr->hsrc;
 if_ptr->ackbuff->netid = MRFNET;
 if_ptr->ackbuff->type = mrf_cmd_ack;

 if_ptr->ackbuff->hsrc = _mrfid;
 if_ptr->ackbuff->usrc = _mrfid;
 if_ptr->ackbuff->msgid = hdr->msgid;
 if_ptr->status->acktimer =  if_ptr->type->tx_del;

 if_ptr->status->state = MRF_ST_ACKDELAY;
 mrf_tick_enable();
}

// send segment retry for buffer
int mrf_sretry(uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 mrf_nexthop(&route,_mrfid,hdr->hsrc);
 MRF_IF *if_ptr = mrf_if_ptr(route.i_f);
 if_ptr->ackbuff->length = sizeof(MRF_PKT_HDR);
 if_ptr->ackbuff->hdest = hdr->hsrc;
 if_ptr->ackbuff->udest = hdr->hsrc;
 if_ptr->ackbuff->netid = MRFNET;
 if_ptr->ackbuff->type = mrf_cmd_retry;

 if_ptr->ackbuff->hsrc = _mrfid;
 if_ptr->ackbuff->usrc = _mrfid;
 if_ptr->ackbuff->msgid = hdr->msgid;
 if_ptr->status->acktimer =  if_ptr->type->tx_del;

 if_ptr->status->state = MRF_ST_ACKDELAY;
 mrf_tick_enable();
}

int mrf_retry(I_F i_f,uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 mrf_nexthop(&route,_mrfid,hdr->hsrc);
 MRF_IF *if_ptr = mrf_if_ptr(route.i_f);
 if_ptr->ackbuff->length = sizeof(MRF_PKT_HDR);
 if_ptr->ackbuff->hdest = hdr->hsrc;
 if_ptr->ackbuff->udest = hdr->hsrc;
 if_ptr->ackbuff->netid = MRFNET;
 if_ptr->ackbuff->type = mrf_cmd_retry;
 if_ptr->ackbuff->hsrc = _mrfid;
 if_ptr->ackbuff->usrc = _mrfid;
 if_ptr->ackbuff->msgid = hdr->msgid;
 if_ptr->status->acktimer =  if_ptr->type->tx_del;
 _mrf_buff_free(bnum);
}

uint8 *mrf_response_buffer(uint8 bnum){
  return _mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_RESP);

}

mrf_data_response(uint8 bnum,uint8 *data,uint8 len){
  int i;
  uint8 *dptr = mrf_response_buffer(bnum);
  for ( i = 0 ; i < len ; i++ )
    *(dptr + i) = data[i];

  return mrf_send_response(bnum, len);
}



int mrf_send_response(uint8 bnum,uint8 rlen){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));

 MRF_ROUTE route;
 mrf_debug("mrf_data_response :  bnum %d  rlen %d\n",bnum,rlen);

 // turning buffer around - deliver to usrc

 mrf_nexthop(&route,_mrfid,hdr->usrc);

 MRF_IF *ifp = mrf_if_ptr(route.i_f);
 mrf_debug("mdr l0 -r.if %d  istate %d\n",route.i_f,ifp->status->state);

 if( mrf_if_tx_queue(route.i_f,bnum) == -1){ // then outgoing queue full - need to retry
   mrf_debug("mdr l0 i_f %d bnum %d\n",route.i_f,bnum);
   mrf_retry(route.i_f,bnum);
   
 }
 else
   {
     mrf_debug("mdr l1\n");
     // turnaround buffer and add response
     hdr->udest = hdr->usrc; 
     hdr->hdest = route.relay;
     hdr->usrc = _mrfid;
     hdr->hsrc = _mrfid;

     resp-> rlen = rlen;
     resp->type = hdr->type;
     hdr->type = mrf_cmd_resp; //_mrf_response_type(hdr->type);
     hdr->length = sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_RESP) + rlen;
     mrf_debug("mrf_data_resp, responding - header follows");
     mrf_print_packet_header(hdr);
   }

 return 0;
}

void mrf_print_packet_header(MRF_PKT_HDR *hdr){

  uint8 type = hdr->type;
   mrf_debug("**************************************\n");

   mrf_debug("PACKET %s\n",mrf_sys_cmds[hdr->type].str);
  
   mrf_debug(" HSRC %02X HDEST %02X  LEN %02X  MSGID %02X   \n",hdr->hsrc,hdr->hdest,hdr->length,hdr->msgid);
   mrf_debug(" USRC %02X UDEST %02X  NETID %02X type %02X  \n",hdr->usrc,hdr->udest,hdr->netid,hdr->type);
   mrf_debug("**************************************\n");
}


void _mrf_print_packet_header(MRF_PKT_HDR *hdr,I_F owner){
  MRF_IF *ifp = mrf_if_ptr(owner);

  uint8 type = hdr->type;
   mrf_debug("**************************************\n");

   mrf_debug("PACKET %s rx on I_F %d  state %d\n",mrf_sys_cmds[hdr->type].str,owner,ifp->status->state);
  
   mrf_debug(" HSRC %02X HDEST %02X  LEN %02X  MSGID %02X   \n",hdr->hsrc,hdr->hdest,hdr->length,hdr->msgid);
   mrf_debug(" USRC %02X UDEST %02X  NETID %02X type %02X  \n",hdr->usrc,hdr->udest,hdr->netid,hdr->type);
   mrf_debug("**************************************\n");
}

uint8 _nibble2hex(uint8 nib){
  nib = nib%16;
  if(( 0 <= nib ) && ( nib < 10))
    return '0' + nib;
  else
    return 'A' + (nib - 10);
}

void _mrf_print_hex_buff(uint8 *buff,uint16 len){
 
  uint8 s[50];
  uint8 i;
  mrf_debug("print_hex_buff : len is %u buff:",len);
  if (len > 16 ){
    mrf_debug("try len < 16 - you had %d\n",len);
    return;
  }
  for ( i=0; i<len ; i++){
    s[i*2] = _nibble2hex(buff[i]/16);
    s[i*2+1] = _nibble2hex(buff[i]%16);
  }
  s[i*2] = '\0';
 
  //s[0] = '\0';
  mrf_debug("%s",s);

}

int _mrf_ex_packet(uint8 bnum, MRF_PKT_HDR *pkt, const MRF_CMD *cmd,MRF_IF *ifp){
      mrf_debug("_mrf_ex_packet INFO: EXECUTE PACKET UDEST %02X is us %02X \n",pkt->udest,_mrfid);
      mrf_debug("cmd name %s  req size %d  rsp size %d\n",
                cmd->str,cmd->req_size,cmd->rsp_size);
      if( ( cmd->data != NULL )  && ( cmd->rsp_size > 0 ) && ( (cmd->cflags & MRF_CFLG_NO_RESP) == 0)) {
        mrf_debug("sending data response \n");
        mrf_data_response(bnum,cmd->data,cmd->rsp_size);
        return;

      }
      mrf_debug("pp l12\n");
      // check if command func defined
      if(cmd->func != NULL){
        mrf_debug("executing cmd func");
        (*(cmd->func))(pkt->type,bnum,ifp);
        return;
      }
}

int _mrf_ex_buffer(uint8 bnum){
  MRF_PKT_HDR *pkt;
  I_F owner = mrf_buff_owner(bnum);
  MRF_IF *ifp = mrf_if_ptr(owner);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  mrf_debug("_mrf_ex_buffer bnum %d",bnum);
  if(pkt->type < MRF_NUM_SYS_CMDS){
    const MRF_CMD *cmd = (const MRF_CMD *) &(mrf_sys_cmds[pkt->type]);
    return  _mrf_ex_packet(bnum, pkt, cmd, ifp);
  }
  return -1;
}

int _mrf_process_buff(uint8 bnum)
{
  uint8 len;
  MRF_PKT_HDR *pkt;
  uint8 type;
  mrf_debug("_mrf_process_buff: processing buff number %d our _mrfid = %X \n",bnum,_mrfid);
  I_F owner = mrf_buff_owner(bnum);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  type = pkt->type;
  _mrf_print_packet_header(pkt,owner);

  // check we are hdest 
  if ( pkt->hdest != _mrfid)
    {
      mrf_debug("ERROR:  HDEST %02X is not us %02X - mrf_bus.pkt_error\n",pkt->hdest,_mrfid);
      return -1;
    }
  if(type >= MRF_NUM_SYS_CMDS){
    mrf_debug("unsupported packed type 0x%02X\n",pkt->type);
    return -1;
  }

  MRF_IF *ifp = mrf_if_ptr(owner);
  // don't count ack and retry
  //  if(type < mrf_cmd_resp)
  // ifp->status->stats.rx_pkts++;
  
  // lookup command
  const MRF_CMD *cmd = (const MRF_CMD *) &(mrf_sys_cmds[pkt->type]);
  mrf_debug("looked up command %d %s",pkt->type,cmd->str);
 
  // begin desperate
  
  // a response can substitute for an ack - if usrc and hsrc are same
  if (((pkt->type) == mrf_cmd_resp) && ((pkt->usrc) == (pkt->hsrc))){
    // act like we received and ack
      mrf_debug("got resp_should count as ack \n");
      mrf_task_ack(pkt->type,bnum,ifp);
  }
  // end desperate

  // check if we are udest

  if ( pkt->udest == _mrfid){
      if(type >= mrf_cmd_resp)
        ifp->status->stats.rx_pkts++;     
      if(cmd->cflags & MRF_CFLG_INTR) { // execute in this intr handler
        mrf_debug("executing packet in intr");
        _mrf_ex_packet(bnum, pkt, cmd, ifp); 
      } else {
        int rv = queue_push(&_app_queue,bnum);
        if (rv == 0){
          mrf_debug("buffer %d pushed to app queue ok rv= %d  \n",bnum,rv);
          if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
            mrf_sack(bnum);   
          }
        } else {
            mrf_debug("buffer %d  app queue full, retrying rv = %d  \n",bnum,rv);
            mrf_sretry(bnum);
          
        }
 
      }
  } else {
    //otherwise send segment ack then forward on network
    MRF_ROUTE route;
 
    mrf_nexthop(&route,_mrfid,pkt->udest);
    MRF_IF *ifp = mrf_if_ptr(route.i_f);

    if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
      mrf_sack(bnum);   
    }
    if( mrf_if_tx_queue(route.i_f,bnum) == -1) // then outgoing queue full - need to retry
      mrf_retry(route.i_f,bnum);
    else{

      mrf_debug("INFO:  UDEST %02X : forwarding to %02X on I_F %d  st %d\n",pkt->udest,route.relay,route.i_f,ifp->status->state);
    
      pkt->hdest = route.relay;
      pkt->hsrc = _mrfid;
  
    }
  }
  
}



int _tick_count;

void mrf_sys_init(){
  _tick_count = 0;
  queue_init(&_app_queue);
}


int mrf_foreground(){
  /* empty application queue and return */
  uint8 bnum;
  int rv,cnt = 0;
  while(queue_data_avail(&_app_queue)){
    mrf_debug("appq data available\n");
    bnum = queue_pop(&_app_queue);
    mrf_debug("got bnum %d\n",bnum);
    _mrf_ex_buffer(bnum);
    mrf_debug("rv was %d",rv);
    cnt++;
  }
  //mrf_debug("no more data available returning after %d cmds",cnt);
  return cnt;
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
    mrf_debug("%d\n",_tick_count);
  /*
  if(_tick_count > 100 ){
    _mrf_if_print_all();
    _mrf_buff_print();
    exit(1);
  }
  */
  for ( i = 0 ; i < NUM_INTERFACES ; i++){
    mif = mrf_if_ptr(i);
    qp = &(mif->status->txqueue);
    istate = mif->status->state;
    mrf_debug("tick -  i_f %d state %d  acktimer %d\n",i,istate,mif->status->acktimer);

    // check if i_f busy
    if ( istate == MRF_ST_WAITSACK){
      if_busy = 1;
      if ((mif->status->acktimer) > 0 ){
 
        mif->status->acktimer--;
        if((mif->status->acktimer) == 0){
          mrf_debug("tick - aborting wait for ack  i_f %d  tc %d \n",i,_tick_count);
          mif->status->state = MRF_ST_TX;
        }
      }
 
      if_busy = 1;
    }

    else if ( istate == MRF_ST_ACKDELAY)
      {
        if_busy = 1;
        if ((mif->status->acktimer) > 0 ){
 
          mif->status->acktimer--;
          if((mif->status->acktimer) == 0){
            mrf_debug("tick - send ack i_f %d  tc %d \n",i,_tick_count);
            
            mif->status->state = MRF_ST_ACK;
            (*(mif->type->funcs.send))(i,(uint8 *)&(mif->ackbuff));
          }
        }

      }
    else if (queue_data_avail(qp))
        {

          if_busy = 1;

          bnum = queue_head(qp);
          if_busy = 1;

          bs = _mrf_buff_state(bnum);
          mrf_debug("\nmrf_tick : FOUND txqueue -IF = %d buffer is %d buff state %d tx_timer %d  tc %d\n",
                    i,bnum,bs->state,bs->tx_timer,_tick_count);
     
          // if (_tick_count >10)
          //  exit(1);
          if ( (bs->state) == TXQUEUE ){
            mrf_debug("buffer state is TXQUEUE \n");

        
            if ( (bs->tx_timer) == 0 ){
              if (1) { //(_mrf_if_can_tx(istate)) {
                tb = _mrf_buff_ptr(bnum);
                //mrf_debug("calling send func\n");
                bs->retry_count++;
                mrf_debug("tick - send buff %d i_f %d tc %d retry_count %d\n",
                          bnum,i,_tick_count,bs->retry_count);
                mif->status->state = MRF_ST_TX;
                (*((mif->type->funcs).send))(i,tb);

                bs->state = TX;
                mif->status->state = MRF_ST_WAITSACK;
                mif->status->acktimer = 10;
                bs->tx_timer = 0;
              }
            // queue_pop(qp);
            //count++;
            }
            else {
              (bs->tx_timer) --;  
            //mrf_debug("tx_timer -> %d\n",bs->tx_timer);
            }
          }
          else if ( (bs->state) == TX ){
            mrf_debug("mrf_tick : bs state TX  on IF %d - timer = %d tc %d\n",i,bs->tx_timer,_tick_count);         
 
            if(((bs->tx_timer) ++) > _MRF_TX_TIMEOUT ){
              mrf_debug("mrf_tick : tx timeout on IF %d - timer = %d tc %d\n",i,bs->tx_timer,_tick_count);         
              if ((bs->retry_count ) < _MRF_MAX_RETRY){
                mrf_debug("retry number %d\n",bs->retry_count);
                bs->tx_timer = mif->type->tx_del;
                bs->state = TXQUEUE;
                mif->status->state = MRF_ST_TXQ;
                mif->status->stats.tx_retries++;


              }else{

                mrf_debug("retry limit reached - abort buffer tx\n");
                // maybe send NDR here ..but in meantime
                bs->state = FREE;
                bs->owner = NUM_INTERFACES;
                mif->status->state = MRF_ST_RX;
                queue_pop(qp);
              }                              
            }
            

          }
        }
    // end if can_tx 
  } // for i=0 to NUM_INTERFACES

  if (if_busy == 0 )
    {
      // all i_fs are idle - turn off tick

      mrf_debug("mrf_tick - turning off tick - if_busy = %d  tc = %d\n",if_busy,_tick_count);
      _mrf_if_print_all();
      mrf_tick_disable();
      
    }
  else{
    // mrf_debug("mrf_tick - keeping tick - if_busy = %d  tc = %d\n",if_busy,_tick_count);
    //_mrf_if_print_all();   
    
  }
}


