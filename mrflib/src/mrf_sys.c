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

#include "mrf_types.h"

#include "mrf_debug.h"
#include "mrf_buff.h"
#include "mrf_if.h"
#include "mrf_sys_structs.h"
#include "mrf_route.h"
#include "mrf_sys_cmds.h"

#define _MRF_TX_TIMEOUT 10   // FIXME - what is this? - we also have ACKTIMER
#define _MRF_MAX_RETRY 4


#define ACKTIMER_VAL 100   //FIXME prob needs to be i/f dependent
//extern uint8 _mrfid;

static IQUEUE _app_queue;
extern const MRF_CMD mrf_sys_cmds[MRF_NUM_SYS_CMDS];
extern const MRF_CMD mrf_app_cmds[MRF_NUM_APP_CMDS];


static uint8 _txmsgid;

/*
uint8 _mrf_response_type(uint8 type){
  return type | 0x80;
}
*/


const MRF_CMD *mrf_cmd_ptr(uint8 type){
  if (type >= MRF_NUM_SYS_CMDS)
    return NULL;
  return &mrf_sys_cmds[type];
}


const MRF_CMD * _mrf_cmd(uint8 type){
  /*return mrf cmd for type hdr param*/
    // lookup command
  uint8 app_cnum = type - _MRF_APP_CMD_BASE;
  if(type < MRF_NUM_SYS_CMDS){
    return &(mrf_sys_cmds[type]);
  }
  else if(app_cnum < MRF_NUM_APP_CMDS) {
    return &(mrf_app_cmds[app_cnum]);
  }
  else  {
    mrf_debug("unsupported packed type 0x%02X\n",type);
    return NULL;
  }

}

int needs_ack(uint8 type){
  // FIXME app cmds are always considered ack
  if(type < MRF_NUM_SYS_CMDS){
    return ((mrf_sys_cmds[type].cflags & MRF_CFLG_NO_ACK) == 0);
  }
  if(type >=  _MRF_APP_CMD_BASE){
    return 1;
  }
  return 0;
}

int valid_cmd(uint8 type){
  // token effort to check validity
  if(type < MRF_NUM_SYS_CMDS){
    return 1;
  }
  if(type >=  _MRF_APP_CMD_BASE){
    return 1;
  }
  return 0;

}



uint16 mrf_copy(void *src,void *dst, size_t nbytes){
  uint16 i;
  for ( i = 0 ; i < nbytes ; i++ ){
    *((uint8 *)dst + i) =  *((uint8 *)src + i);
  }
  return i;
}
uint16 mrf_scopy(void *src,void *dst, size_t nbytes){
  uint16 i;
  uint8 sc;
  for ( i = 0 ; i < nbytes ; i++ ){
    sc = *((uint8 *)src + i);
    *((uint8 *)dst + i) =  sc;
    if (sc == '\0')
      break;
  }
  return i;
}


// send segment ack for buffer
int mrf_sack(uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 if (hdr->hsrc == 0 ){
   mrf_debug("mrf_sack : never acking address 0  0x%02x\n",hdr->hsrc);
   return 0;
 }
 mrf_nexthop(&route,_mrfid,hdr->hsrc);
 mrf_debug("mrf_sack : for addr %d msgid 0x%02x\n",hdr->hsrc,hdr->msgid);
 //mrf_debug("mrf_sack : for addr %d orig header is\n",hdr->hsrc);
 //mrf_print_packet_header(hdr);
 //mrf_debug("route if is %d\n",route.i_f);
 const MRF_IF *if_ptr = mrf_if_ptr(route.i_f);
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

 //mrf_debug("%s","mrf_sack : exit, ackbuff is\n");
 //mrf_print_packet_header(if_ptr->ackbuff);

 mrf_tick_enable();
 return 0;
}

// send segment retry for buffer
int mrf_sretry(uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_ROUTE route;
 if (hdr->hsrc == 0 )  // never ack or retry 0 
   return 0;
 
 mrf_nexthop(&route,_mrfid,hdr->hsrc);
 const MRF_IF *if_ptr = mrf_if_ptr(route.i_f);
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
 return 0;
}




uint8 *mrf_response_buffer(uint8 bnum){
  return _mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_RESP);

}

int mrf_data_response(uint8 bnum,const uint8 *data,uint8 len){
  int i;
  uint8 *dptr = mrf_response_buffer(bnum);
  for ( i = 0 ; i < len ; i++ )
    *(dptr + i) = data[i];

  return mrf_send_response(bnum, len);
}
//FIXME way too much near identical code in different functions around here


int mrf_send_structure(uint8 dest, uint8 code,  uint8 *data, uint8 len){
  uint8 bnum;
  uint8 i;
  MRF_ROUTE route;

 // deliver buffer to dest

  mrf_nexthop(&route,_mrfid,dest);

  if ((bnum = mrf_alloc_if(route.i_f)) == _MRF_BUFFS){
    return -1;
  }
  mrf_debug("\nmrf_send_structure :  bnum %d  len %d\n",bnum,len);

  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));

  //MRF_IF *ifp = mrf_if_ptr(route.i_f);
  hdr->udest = dest; 
  hdr->hdest = route.relay;
  hdr->usrc = _mrfid;
  hdr->hsrc = _mrfid;
  hdr->netid = MRFNET; 
  hdr->msgid = _txmsgid++;
  uint8 *dptr = mrf_response_buffer(bnum);
  for (i = 0 ; i < len ; i++ )
    *(dptr + i) = data[i];

 
 resp->rlen = len;
 resp->type = code;
 hdr->type = mrf_cmd_usr_struct; //_mrf_response_type(hdr->type);
 hdr->length = sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_RESP) + len;
 mrf_debug("%s","mrf_send_resp, responding - header follows(1)\n");
 //mrf_print_packet_header(hdr);
 _mrf_buff_state(bnum)->state = LOADED; //
 if( mrf_if_tx_queue(route.i_f,bnum) == -1){ // then outgoing queue full - need to retry
   // mrf_if_tx_queue increments if_status.stats.tx_overruns if it returns -1 here
   
   return -1;
 }
 else
   {
     mrf_debug("sent structure resp->rlen = %u resp->type=%u\n",resp->rlen,resp->type);
   }

 return 0;  
}

#ifdef HOST_STUB
extern int response_to_app(uint8 bnum);

#endif






int _mrf_send_command_src(uint8 usrc , uint8 dest, uint8 type,  uint8 *data, uint8 len){
  uint8 bnum;
  uint8 i;
  MRF_ROUTE route;

  mrf_nexthop(&route,_mrfid,dest);

  if ((bnum = mrf_alloc_if(route.i_f)) == _MRF_BUFFS){
    mrf_debug("%s","ERROR : failed to alloc  buffer in mrf_send_command..fatal");    
    return -1;
  }
  mrf_debug("mrf_send_command_src :  using bnum %d  len %d\n",bnum,len);

  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
  uint8 *pl = (uint8 *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));

 
  for (i = 0 ; i < len ; i++ )
    *(pl + i) = data[i];
   


 // deliver buffer to dest

  mrf_nexthop(&route,_mrfid,dest);

  mrf_debug("route.i_f %u route.relay %u\n",route.i_f, route.relay);
  
 //MRF_IF *ifp = mrf_if_ptr(route.i_f);
 hdr->udest = dest;
 if (hdr->udest == _mrfid){
   hdr->hdest = _mrfid;
 }
 else {
   hdr->hdest = route.relay;

 }
 hdr->hsrc = _mrfid;
 hdr->usrc = usrc;

 hdr->netid = MRFNET; 
 hdr->msgid = _txmsgid++;
 hdr->type = type;
 hdr->length = sizeof(MRF_PKT_HDR) + len ;  //FIXME just send headers for now - no payloads  + sizeof(MRF_PKT_RESP) + len;

 mrf_debug("%s","mrf_send_command : this is our header\n");
 
 mrf_print_packet_header(hdr);
 
 #ifdef HOST_STUB
 // FIXME - cheap way of returning receipts for messages relayed - send squirt back the buffer to be transmitted
 response_to_app(bnum);

 #endif

 if (dest == _mrfid) {
   // process buffer
   mrf_debug("%s","hold onto your hats... trying to process locally..\n");
   _mrf_process_buff(bnum);
   return 0;
 }

 
 _mrf_buff_state(bnum)->state = LOADED; //
 mrf_debug("attempting to queue bnum %u for tx on i_f %u\n",bnum,route.i_f);
 if( mrf_if_tx_queue(route.i_f,bnum) == -1){ // then outgoing queue full - need to retry

   // mrf_if_tx_queue increments if_status.stats.tx_overruns if it returns -1 here
   _mrf_buff_free(bnum);
   mrf_debug("failed to queue bnum %u for transmit\n",bnum);

   return -1;
 }
 else
   {
     mrf_debug("sent command type=%u  bnum was %u i_f %u\n",type,bnum,route.i_f);
   }

 return 0;  
}



int mrf_send_command( uint8 dest, uint8 type,  uint8 *data, uint8 len){


  return _mrf_send_command_src(_mrfid ,  dest,  type,   data,  len);

}

// this is used by host to relay commands from server
// prob shouldn't be in scope for all builds
// FIXME - move to host app
int mrf_send_command_root( uint8 dest, uint8 type,  uint8 *data, uint8 len){

  return _mrf_send_command_src(0 ,  dest,  type,   data,  len);


} 
int mrf_send_response(uint8 bnum,uint8 rlen){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));
 MRF_ROUTE route;
 mrf_debug("mrf_send_response :  bnum %d  rlen %d\n",bnum,rlen);

 // turning buffer around - deliver to usrc
 hdr->udest = hdr->usrc; 

 // turnaround buffer and add response
 hdr->usrc = _mrfid;
 hdr->hsrc = _mrfid;

 resp->rlen = rlen;
 resp->type = hdr->type;
 resp->msgid = hdr->msgid;
 hdr->type = mrf_cmd_resp; //_mrf_response_type(hdr->type);
 hdr->length = sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_RESP) + rlen;

 //hdr->msgid = _txmsgid++;  // NO! resp should have same msgid as initiator packet
 
#ifdef HOST_STUB
 mrf_debug("HOST_STUB defined _mrfid %d  hdr->udest %d\n",_mrfid, hdr->udest);
 if ((_mrfid == 1 ) && (hdr->udest == 0 )){
   hdr->hdest = 0;
   mrf_debug("calling response to app for bnum %d\n",bnum);
   response_to_app(bnum);
   _mrf_buff_free(bnum);
   return 0;
 }
 
#endif
 
 mrf_nexthop(&route,_mrfid,hdr->udest);

 hdr->hdest = route.relay;

 //const MRF_IF *ifp = mrf_if_ptr(route.i_f);
 //mrf_debug("mdr l0 -r.if %d  istate %d\n",route.i_f,ifp->status->state);


 return mrf_if_tx_queue(route.i_f,bnum);
 
}



// should be 2 situations where this is called
// code = RECD_SRETRY  - mrf_task_sretry has run upon receipt of mrf_retry command from nexthop 
// code = MAX_RETRIES  - this device has hit retry limit trying to tx to nexthop

int mrf_ndr(uint8 code, uint8 bnum){
 MRF_PKT_HDR *hdr  = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 
 MRF_PKT_NDR *ndr = (MRF_PKT_NDR *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));
 MRF_ROUTE route;
 mrf_debug("mrf_ndr :  bnum %d code %d \n",bnum,code);
 // this buffer might contain recieved mrf_retry  or max retried tx buffer
 // either way we copy msgid,  hdest and hsrc of this buffer to ndr packet
 mrf_print_packet_header(hdr);


 if (hdr->usrc == _mrfid )  {   // don't send NDR to ourself! - ony for packets we are relaying
   // FIXME maybe so something useful here
   mrf_debug("packet usrc is us %d\n",hdr->usrc);
    _mrf_buff_free(bnum);
  return 0;

 }
 ndr->code  = code;
 ndr->msgid = hdr->msgid;
 ndr->hdest = hdr->hdest;
 ndr->hsrc  = hdr->hsrc;
 mrf_debug("mrf_ndr : code %d msgid %d hdest %d hsrc %d : here is header\n",ndr->code,ndr->msgid,ndr->hdest,ndr->hsrc);

 mrf_print_packet_header(hdr);

 // turning buffer around - deliver to usrc
 hdr->udest = hdr->usrc; 

 // turnaround buffer and add response
 hdr->usrc = _mrfid;
 hdr->hsrc = _mrfid;

 hdr->msgid = _txmsgid++;

 hdr->type = mrf_cmd_ndr;
 hdr->length = sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_NDR);

 
 
#ifdef HOST_STUB
 mrf_debug("HOST_STUB defined _mrfid %d  hdr->udest %d\n",_mrfid, hdr->udest);
 if ((_mrfid == 1 ) && (hdr->udest == 0 )){
   hdr->hdest = 0;
   mrf_debug("calling response to app for bnum %d\n",bnum);
   response_to_app(bnum);
   _mrf_buff_free(bnum);
   return 0;
 }
 
#endif
 
 mrf_nexthop(&route,_mrfid,hdr->udest);

 hdr->hdest = route.relay;

 //const MRF_IF *ifp = mrf_if_ptr(route.i_f);
 //mrf_debug("mdr l0 -r.if %d  istate %d\n",route.i_f,ifp->status->state);


 return mrf_if_tx_queue(route.i_f,bnum);
 
}



static const uint8 _appcname[] = "APP_CMD";

void mrf_print_packet_header(MRF_PKT_HDR *hdr){

  const uint8 *cname;
  if (hdr->type >= _MRF_APP_CMD_BASE)
    cname = _appcname;
  else
    cname = mrf_sys_cmds[hdr->type].str;

  mrf_debug("%s","**************************************\n");

  mrf_debug("PACKET %s  LEN %d \n",cname,hdr->length);
  
  mrf_debug(" HSRC 0x%02X HDEST 0x%02X  LEN %02d  MSGID 0x%02X   \n",hdr->hsrc,hdr->hdest,hdr->length,hdr->msgid);
  mrf_debug(" USRC 0x%02X UDEST 0x%02X  NETID 0x%02X type 0x%02X  \n",hdr->usrc,hdr->udest,hdr->netid,hdr->type);
  mrf_debug("%s","**************************************\n");
}



int _mrf_ex_packet(uint8 bnum, MRF_PKT_HDR *pkt, const MRF_CMD *cmd,const MRF_IF *ifp){
      mrf_debug("\n_mrf_ex_packet INFO: EXECUTE PACKET UDEST %02X is us %02X \n",pkt->udest,_mrfid);
      mrf_debug("cmd name %s  req size %u  rsp size %u cflags %x cmd->data %p\n",
                cmd->str,cmd->req_size,cmd->rsp_size,cmd->cflags,cmd->data);
      if( ( cmd->data != NULL )  && ( cmd->rsp_size > 0 )) {  
        mrf_debug("%s","sending data response \n");
        mrf_data_response(bnum,(uint8 *)cmd->data,cmd->rsp_size);
        return 0;
      }
      mrf_debug("%s","pp l12\n");
      // check if command func defined
      if(cmd->func != NULL){
        mrf_debug("%s","executing cmd func\n");
        (*(cmd->func))((MRF_CMD_CODE)pkt->type,bnum,ifp);
      }
      return 0;
}

int mrf_app_queue_push(uint8 bnum){
  int rv = queue_push(&_app_queue,bnum);
  
  if ( rv == 0) {
    mrf_debug("%s","queue pushed.. waking\n");
    mrf_wake();
  }
  else{
    mrf_debug("prob pushing on queue - rv = %d\n",rv);
  }
  return rv;
}


int _mrf_ex_buffer(uint8 bnum){
  MRF_PKT_HDR *pkt;
  I_F owner = mrf_buff_owner(bnum);
  const MRF_IF *ifp = mrf_if_ptr(owner);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  mrf_debug("_mrf_ex_buffer bnum %d\n",bnum);
  const MRF_CMD *cmd =  _mrf_cmd(pkt->type);
  
  if (cmd == NULL){
    mrf_debug("_mrf_ex_buffer cmd was null..abort (pkt->type was %d)\n",pkt->type);
    return -1;
  }
  
  mrf_debug("packet type %d\n",pkt->type);
  return  _mrf_ex_packet(bnum, pkt, cmd, ifp);

}

int _Dbg_fh(){
  return -2;
}
int _Dbg_fw(){
  return -3;
}
int _Dbg_fw2(){
  return 2;
}


// FIXME lots of duplicated code here wrt. data_response / ex packet - need to clean this up
static int _mrf_buff_forward(uint8 bnum){
  MRF_PKT_HDR *pkt;
  uint8 type;
  mrf_debug("mrf_buff_forward: processing buff number %d our _mrfid = %X  \n",bnum,_mrfid);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  type = pkt->type;

#ifdef HOST_STUB
  if ((_mrfid == 1 ) && (pkt->udest == 0 )){
   mrf_debug("mrf_buff_forward: this one is for app/server udest is  %d our _mrfid = %X  \n",pkt->udest,_mrfid);
   response_to_app(bnum);
   _mrf_buff_free(bnum);
   return 0;
  }
#endif
  
  MRF_ROUTE route;
  mrf_nexthop(&route,_mrfid,pkt->udest);
  //const MRF_IF *ifp = mrf_if_ptr(route.i_f);
  pkt->hdest = route.relay;
  pkt->hsrc = _mrfid;
  pkt->netid = MRFNET;


  if ((pkt->type != mrf_cmd_ack) && ( pkt->hdest == 33)) {
      _Dbg_fw2();

  }
  mrf_debug("udest is 0x%x route.i_f is %u route.relay %u\n",pkt->udest,route.i_f,route.relay);
  _Dbg_fw();
  if( mrf_if_tx_queue(route.i_f,bnum) == -1){ // then outgoing queue full - need to retry
    // mrf_if_tx_queue increments if_status.stats.tx_overruns if it returns -1 here
    mrf_sretry(bnum);
  }
  else{

    mrf_debug("INFO:  UDEST %02X : forwarding to %02X on I_F %d \n",pkt->udest,route.relay,route.i_f);  
  }
  if (pkt->hdest == 1){
    _Dbg_fh();
    
  }
  return 0;
}

extern uint8 buffer_responded(uint8 bnum, const MRF_IF *ifp);


int _mrf_process_buff(uint8 bnum)
{
  MRF_PKT_HDR *pkt;
  MRF_PKT_RESP *resp;
  MRF_PKT_PING_RES *pingres;
  uint8 type, resptxbuff;
  I_F owner = mrf_buff_owner(bnum);
  mrf_debug("_mrf_process_buff: processing buff number %d our _mrfid = %X owner i_f %d \n",bnum,_mrfid, owner);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  type = pkt->type;
  mrf_print_packet_header(pkt);

  // check we are hdest 
  if ( pkt->hdest != _mrfid)
    {
      mrf_debug("ERROR:  HDEST %02X is not us %02X - mrf_bus.pkt_error\n",pkt->hdest,_mrfid);
      _mrf_buff_free(bnum);

      return -1;
    }

  const MRF_IF *ifp = mrf_if_ptr(owner);
  // don't count ack and retry
  //  if(type < mrf_cmd_resp)
  // ifp->status->stats.rx_pkts++;
  

  // begin desperate
  
  // a response can substitute for an ack - if usrc and hsrc are same
  // note : this func is called by mrf_send_command - stub app - but can't hit this block in that sitn
  // unless the server has gone bonkers .. or possibly a test client... FIXME!
  if ((pkt->usrc !=  0) && (pkt->type == mrf_cmd_resp) && (pkt->usrc == pkt->hsrc)){  
    // act like we received an ack - regardless of whether we are udest
    
    
    resptxbuff = buffer_responded(bnum,ifp);
    mrf_debug("buffer_responded bnum %u",resptxbuff);
    if (resptxbuff < _MRF_BUFFS){
      mrf_debug("RESP counts as ack for buffer %d",resptxbuff);
      ifp->status->stats.tx_pkts++;

      if( pkt->udest != _mrfid){
        mrf_debug("udest %u was not us , freeing buff %d",pkt->udest,resptxbuff);
        _mrf_buff_free(resptxbuff);
      }
    }
    else {
      mrf_debug("%s","ERROR got resp, but wasn't expecting one!!");

    }
    // mrf_task_ack(pkt->type,bnum,ifp); 

    resp = (MRF_PKT_RESP *)((void *)pkt + sizeof(MRF_PKT_HDR));

    if (resp->type == mrf_cmd_ping) {  // nasty bodge to append get rssi and lqi for response to ping_test
      pingres = (MRF_PKT_PING_RES *)((void *)pkt + sizeof(MRF_PKT_HDR)+ sizeof(MRF_PKT_RESP));
      pingres->from_rssi = *((uint8 *)pkt + pkt->length);
      pingres->from_lqi = *((uint8 *)pkt + (pkt->length) + 1);
      
    }
  }
  // end desperate

  // check if we are udest

  if ( pkt->udest == _mrfid){

    // lookup command
    const MRF_CMD *cmd = _mrf_cmd(type);
  
    if(cmd == NULL){  // how to manage packets that don't execute - i.e. unsolicited datagrams from sensors, that don't have any meaning in our app. For stub/hub applications we need to pass them to python regardless.
      // note : adding usr_struct sys command every node must provide at least a null handler
      mrf_debug("big trouble 29339 - we got a packet but don't recognise type %d\n",type);
      return -1;
    }

    mrf_debug("looked up command %d %s\n",pkt->type,cmd->str);
 

    if(cmd->cflags & MRF_CFLG_INTR) { // execute in this intr handler
      mrf_debug("%s","executing packet in intr\n");
      _mrf_ex_packet(bnum, pkt, cmd, ifp); 
    } else {
      mrf_debug("%s","pushing to app queue\n");
      int rv = mrf_app_queue_push(bnum);
      if (rv == 0){
        mrf_debug("buffer %d pushed to app queue ok rv= %d  \n",bnum,rv);  
        //if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){    // Try to do away with segment ack when we are target, resp should follow shortly
        //  mrf_debug("%s","sending segment ack\n");
        //  mrf_sack(bnum);   
        
      } else {
        mrf_debug("buffer %d  app queue full, retrying rv = %d  \n",bnum,rv);
        mrf_sretry(bnum); 
      }
    }
  } else if ( valid_cmd(pkt->type)) {
    //otherwise send segment ack then forward on network
    //if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
    if(needs_ack(pkt->type)){
      mrf_sack(bnum);   
    }
    return _mrf_buff_forward(bnum);
  }

  return 0; // if not our address we come here... should have stat
}


uint32  _tick_count;
uint16  _idle_count;

void mrf_sys_init(){
  _tick_count = 0;
  _idle_count = 0;
  _txmsgid = 0;
  queue_init(&_app_queue);
}

int mrf_app_queue_available(){

  return queue_data_avail(&_app_queue);
}

int mrf_foreground(){
  /* empty application queue and return */
  uint8 bnum;
  int rv,cnt = 0;
  while(queue_data_avail(&_app_queue)){
    mrf_debug("%s","mrf_foreground : appq data available\n");

    bnum = (uint8)queue_pop(&_app_queue);
    mrf_debug("mrf_foreground : got bnum %d\n",bnum);

    if (bnum >= MRF_BNUM_SIGNAL_BASE){
      mrf_debug("mrf_foreground : calling signal_handler - signum %d\n",bnum - MRF_BNUM_SIGNAL_BASE);
      rv = signal_handler(bnum - MRF_BNUM_SIGNAL_BASE);  // signal_handler must be defined by app
    }
    else
      rv = _mrf_ex_buffer(bnum);
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
  const MRF_IF *mif; 
  int i;
  uint8 *tb;
  MRF_BUFF_STATE *bs;
  uint8 bnum;
  IF_STATE istate;
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
    mif = mrf_if_ptr((I_F)i);
    qp = &(mif->status->txqueue);
    istate = mif->status->state;
    if ( istate > MRF_ST_IDLE)
      mrf_debug("tick -  i_f %d state %d  acktimer %d\n",i,istate,mif->status->acktimer);

    // check if i_f busy
    if ( istate == MRF_ST_WAITSACK){
      if_busy = 1;
      if ((mif->status->acktimer) > 0 )
        mif->status->acktimer--;
      if((mif->status->acktimer) == 0){
        mrf_debug("tick - aborting wait for ack  i_f %d  tc %d \n",i,_tick_count);
        mif->status->state = MRF_ST_TX;
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
            mrf_print_packet_header(mif->ackbuff);
            mif->status->state = MRF_ST_ACK;
            mif->status->stats.tx_acks += 1;
            (*(mif->type->funcs.send))((I_F)i,(uint8 *)(mif->ackbuff));
          }
        }
      }
    else if ( istate == MRF_ST_ACK ){
      mif->status->state = MRF_ST_RX;
      if_busy = 1;  // wait for next tick to check queues before marking idle

    }
    else if (queue_data_avail(qp))
        {

          if_busy = 1;

          bnum = queue_head(qp);
          if_busy = 1;

          bs = _mrf_buff_state(bnum);
          MRF_PKT_HDR *pkt;
          pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);

          mrf_debug("mrf_tick : FOUND txqueue -IF = %d buffer is %d buff state %d tx_timer %d  tc %d\n",
                    i,bnum,bs->state,bs->tx_timer,_tick_count);
     
          // if (_tick_count >10)
          //  exit(1);
          if ( (bs->state) == TXQUEUE ){
            mrf_debug("%s","buffer state is TXQUEUE \n");

        
            if ( (bs->tx_timer) == 0 ){
                tb = _mrf_buff_ptr(bnum);
                //mrf_debug("calling send func\n");
                bs->retry_count++;
                mrf_debug("tick - send buff %d i_f %d tc %d retry_count %d\n",
                          bnum,i,_tick_count,bs->retry_count);
                mif->status->state = MRF_ST_TX;
                //mif->status->stats.tx_pkts += 1;   // only inc this stat if ack recieved - i.e. in ack/resp handlers
                (*(mif->type->funcs.send))((I_F)i,tb);

                //const MRF_CMD *cmd = _mrf_cmd(pkt->type);

                if (!needs_ack(pkt->type)){   //cmd->cflags & MRF_CFLG_NO_ACK) {
                  //bs->state = TX;
                  mif->status->state = MRF_ST_RX; // FIXME what is this if state really about?
                  _mrf_buff_free(bnum);
                  queue_pop(qp);

                } else {
                  bs->state = TX;
                  mif->status->state = MRF_ST_WAITSACK;
                  mif->status->acktimer = ACKTIMER_VAL;
                }
                bs->tx_timer = 0;
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

                mrf_debug("%s","retry limit reached - abort buffer tx\n");
                mif->status->stats.tx_errors++;

                
                
#if 0                
                // maybe send NDR here ..but in meantime
                bs->state = FREE;   // effectively mrf_buff_free
                bs->owner = NUM_INTERFACES;
#else

                mrf_ndr(NDR_MAX_RETRIES,bnum);  // this reuses buffer for ndr
                
                //pkt->

#endif                
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

#ifdef SLEEP_deep
        mrf_sleep_deep();  // must be defined by app for now
#endif
     
      
    }
  else{
    _idle_count = 0;
    // mrf_debug("mrf_tick - keeping tick - if_busy = %d  tc = %d\n",if_busy,_tick_count);
    //_mrf_if_print_all();   
    
  }
}


int mrf_app_signal(uint8 signum){
  mrf_app_queue_push(MRF_BNUM_SIGNAL_BASE + signum);


}
