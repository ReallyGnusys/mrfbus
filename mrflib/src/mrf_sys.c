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

#define TX_TIMEOUT_VAL 20   //FIXME prob needs to be i/f dependent
//extern uint8 _mrfid;

static BuffQueue _app_queue;

extern const MRF_CMD mrf_sys_cmds[MRF_NUM_SYS_CMDS];
extern const MRF_CMD mrf_app_cmds[MRF_NUM_APP_CMDS];

const MRF_CMD *mrf_cmd_ptr(uint8 type){
  if (type >= MRF_NUM_SYS_CMDS)
    return NULL;
  return &mrf_sys_cmds[type];
}

const MRF_CMD *mrf_app_cmd_ptr(uint8 type){
  if (type >= MRF_NUM_APP_CMDS)
    return NULL;
  return &mrf_app_cmds[type];
}

static uint8 _txmsgid;

/*
uint8 _mrf_response_type(uint8 type){
  return type | 0x80;
}
*/




const MRF_CMD * _mrf_cmd(uint8 type){
  /*return mrf cmd for type hdr param*/
    // lookup command
  uint8 app_cnum = type - _MRF_APP_CMD_BASE;
  if(type < MRF_NUM_SYS_CMDS){
    return mrf_cmd_ptr(type);
  }
  else if(app_cnum < MRF_NUM_APP_CMDS) {
    return mrf_app_cmd_ptr(app_cnum);
  }
  else  {
    mrf_debug("unsupported packed type 0x%02X\n",type);
    return NULL;
  }

}

int needs_ack(uint8 type){
  // FIXME app cmds are always considered ack
  if(type < MRF_NUM_SYS_CMDS){
    return (((mrf_cmd_ptr(type)->cflags) & MRF_CFLG_NO_ACK) == 0);
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
uint16 mrf_scopy(void *src,void *dst, size_t nbytes){  //string copy
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
 mrf_nexthop(&route,MRFID,hdr->hsrc);
 mrf_debug("mrf_sack : for addr %d msgid 0x%02x\n",hdr->hsrc,hdr->msgid);
 //mrf_debug("mrf_sack : for addr %d orig header is\n",hdr->hsrc);
 //mrf_print_packet_header(hdr);
 //mrf_debug("route if is %d\n",route.i_f);


 const MRF_IF *if_ptr = mrf_if_ptr(route.i_f);

 ACK_TAG acktag;

 acktag.type  = mrf_cmd_ack;
 acktag.msgid = hdr->msgid;
 acktag.dest   = hdr->hsrc;

 if(if_ptr->ackqueue->push(acktag) == 0){
   mrf_debug("mrf_sack : pushed acktag to ackqueue for %s i_f %d  num items %d\n",if_ptr->name,route.i_f,if_ptr->ackqueue->items());
 }
 else {
   mrf_debug("ERROR failed to push acktag to ackqueue for %s num items %d\n",if_ptr->name,if_ptr->ackqueue->items());
 }
 /*
 if_ptr->ackbuff->length = sizeof(MRF_PKT_HDR);
 if_ptr->ackbuff->hdest = hdr->hsrc;
 if_ptr->ackbuff->udest = hdr->hsrc;
 if_ptr->ackbuff->netid = MRFNET;
 if_ptr->ackbuff->type  = mrf_cmd_ack;

 if_ptr->ackbuff->hsrc = MRFID;
 if_ptr->ackbuff->usrc = MRFID;
 if_ptr->ackbuff->msgid = hdr->msgid;
 if_ptr->status->acktimer =  if_ptr->type->tx_del;

 if_ptr->status->state = MRF_ST_ACKDELAY;
 */
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

 mrf_nexthop(&route,MRFID,hdr->hsrc);
 const MRF_IF *if_ptr = mrf_if_ptr(route.i_f);


 ACK_TAG acktag;

 acktag.type  = mrf_cmd_retry;
 acktag.msgid = hdr->msgid;
 acktag.dest   = hdr->hsrc;

 if(if_ptr->ackqueue->push(acktag) == 0){
   mrf_debug("pushed acktag to ackqueue for %s num items %d\n",if_ptr->name,if_ptr->ackqueue->items());
 }
 else {
   mrf_debug("ERROR failed to push acktag to ackqueue for %s num items %d\n",if_ptr->name,if_ptr->ackqueue->items());
 }

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

  mrf_nexthop(&route,MRFID,dest);

  if ((bnum = mrf_alloc_if(route.i_f)) == _MRF_BUFFS){
    mrf_debug("%s","\nERROR mrf_send_structure : failed to allocate buffer!!!\n");
    return -1;
  }
  mrf_debug("mrf_send_structure :  bnum %d  len %d\n",bnum,len);

  MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(((uint8 *)hdr)+ sizeof(MRF_PKT_HDR));

  //MRF_IF *ifp = mrf_if_ptr(route.i_f);
  hdr->udest = dest;
  hdr->hdest = route.relay;
  hdr->usrc = MRFID;
  hdr->hsrc = MRFID;
  hdr->netid = MRFNET;
  hdr->msgid = _txmsgid++;
  uint8 *dptr = mrf_response_buffer(bnum);
  for (i = 0 ; i < len ; i++ )
    *(dptr + i) = data[i];


 resp->rlen = len;
 resp->type = code;
 resp->msgid = hdr->msgid;  // FIXME - for usr_struct this has no meaning, only real response
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

  mrf_nexthop(&route,MRFID,dest);

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

  mrf_nexthop(&route,MRFID,dest);

  mrf_debug("route.i_f %u route.relay %u\n",route.i_f, route.relay);

 //MRF_IF *ifp = mrf_if_ptr(route.i_f);
 hdr->udest = dest;
 if (hdr->udest == MRFID){
   hdr->hdest = MRFID;
 }
 else {
   hdr->hdest = route.relay;

 }
 hdr->hsrc = MRFID;
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

 if (dest == MRFID) {
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
     mrf_debug("queued buffer command type=%u  bnum was %u i_f %u\n",type,bnum,route.i_f);
     mrf_if_print_info(route.i_f);
   }

 return 0;
}



int mrf_send_command( uint8 dest, uint8 type,  uint8 *data, uint8 len){


  return _mrf_send_command_src(MRFID ,  dest,  type,   data,  len);

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
 hdr->usrc = MRFID;
 hdr->hsrc = MRFID;

 resp->rlen = rlen;
 resp->type = hdr->type;
 resp->msgid = hdr->msgid;
 hdr->type = mrf_cmd_resp; //_mrf_response_type(hdr->type);
 hdr->length = sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_RESP) + rlen;

 //hdr->msgid = _txmsgid++;  // NO! resp should have same msgid as initiator packet

#ifdef HOST_STUB
 mrf_debug("HOST_STUB defined MRFID %d  hdr->udest %d\n",MRFID, hdr->udest);
 if ((MRFID == 1 ) && (hdr->udest == 0 )){
   hdr->hdest = 0;
   mrf_debug("calling response to app for bnum %d\n",bnum);
   response_to_app(bnum);
   _mrf_buff_free(bnum);
   return 0;
 }

#endif

 mrf_nexthop(&route,MRFID,hdr->udest);

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


 if (hdr->usrc == MRFID )  {   // don't send NDR to ourself! - ony for packets we are relaying
   // FIXME maybe do something useful here
   // what does host 0x01 do for packets from server (notionally 0 )
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
 hdr->usrc = MRFID;
 hdr->hsrc = MRFID;

 hdr->msgid = _txmsgid++;

 hdr->type = mrf_cmd_ndr;
 hdr->length = sizeof(MRF_PKT_HDR) + sizeof(MRF_PKT_NDR);



#ifdef HOST_STUB
 mrf_debug("HOST_STUB defined MRFID %d  hdr->udest %d\n",MRFID, hdr->udest);
 if ((MRFID == 1 ) && (hdr->udest == 0 )){
   hdr->hdest = 0;
   mrf_debug("calling response to app for bnum %d\n",bnum);
   response_to_app(bnum);
   _mrf_buff_free(bnum);
   return 0;
 }

#endif

 mrf_nexthop(&route,MRFID,hdr->udest);

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
    cname = mrf_cmd_ptr(hdr->type)->str;

  mrf_debug("%s","**************************************\n");

  mrf_debug("PACKET %s  LEN %d \n",cname,hdr->length);

  mrf_debug(" HSRC 0x%02X HDEST 0x%02X  LEN %02d  MSGID 0x%02X   \n",hdr->hsrc,hdr->hdest,hdr->length,hdr->msgid);
  mrf_debug(" USRC 0x%02X UDEST 0x%02X  NETID 0x%02X type 0x%02X  \n",hdr->usrc,hdr->udest,hdr->netid,hdr->type);
  mrf_debug("%s","**************************************\n");
}



int _mrf_ex_packet(uint8 bnum, MRF_PKT_HDR *pkt, const MRF_CMD *cmd,const MRF_IF *ifp){
      mrf_debug("\n_mrf_ex_packet INFO: EXECUTE PACKET UDEST %02X is us %02X \n",pkt->udest,MRFID);
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


  int rv = _app_queue.push(bnum);  //queue_push(&_app_queue,bnum);

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
  mrf_debug("mrf_buff_forward: processing buff number %d our MRFID = %X  \n",bnum,MRFID);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  type = pkt->type;

#ifdef HOST_STUB
  if ((MRFID == 1 ) && (pkt->udest == 0 )){
   mrf_debug("mrf_buff_forward: this one is for app/server udest is  %d our MRFID = %X  \n",pkt->udest,MRFID);
   response_to_app(bnum);
   _mrf_buff_free(bnum);
   return 0;
  }
#endif

  MRF_ROUTE route;
  mrf_nexthop(&route,MRFID,pkt->udest);
  //const MRF_IF *ifp = mrf_if_ptr(route.i_f);
  pkt->hdest = route.relay;
  pkt->hsrc = MRFID;
  pkt->netid = MRFNET;


  if ((pkt->type != mrf_cmd_ack) && ( pkt->hdest == 33)) {
      _Dbg_fw2();

  }
  mrf_debug("_mrf_buff_forward : udest is 0x%x route.i_f is %u route.relay %u\n",pkt->udest,route.i_f,route.relay);
  _Dbg_fw();
  if( mrf_if_tx_queue(route.i_f,bnum) == -1){ // then outgoing queue full - need to retry
    // mrf_if_tx_queue increments if_status.stats.tx_overruns if it returns -1 here
    mrf_debug("WARN:  could not queue tx of buff %d on on i_f %d  \n",bnum,route.i_f);

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
  mrf_debug("_mrf_process_buff: processing buff number %d our MRFID = %X owner i_f %d \n",bnum,MRFID, owner);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  type = pkt->type;
  mrf_print_packet_header(pkt);

  // check we are hdest
  if ( pkt->hdest != MRFID)
    {
      mrf_debug("ERROR:  HDEST %02X is not us %02X - mrf_bus.pkt_error\n",pkt->hdest,MRFID);
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
    mrf_debug("buffer_responded bnum %u\n",resptxbuff);
    if (resptxbuff < _MRF_BUFFS){
      mrf_debug("RESP counts as ack for buffer %d\n",resptxbuff);
      //ifp->status->stats.tx_pkts++;
      //ifp->status->state = MRF_ST_TX_COMPLETE;
      //if( pkt->udest != MRFID){
      //  mrf_debug("udest %u was not us , freeing buff %d",pkt->udest,resptxbuff);
        //_mrf_buff_free(resptxbuff);
      // }
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

  if ( pkt->udest == MRFID){

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
  _app_queue = BuffQueue();
  // queue_init(&_app_queue);
}

int mrf_app_queue_available(){

  return _app_queue.data_avail(); // queue_data_avail(&_app_queue);
}

int mrf_foreground(){
  /* empty application queue and return */
  uint8 bnum;
  int rv,cnt = 0;
  //while(queue_data_avail(&_app_queue)){
  while(_app_queue.data_avail()){
    mrf_debug("%s","mrf_foreground : appq data available\n");

    //bnum = (uint8)queue_pop(&_app_queue);
    bnum = *(_app_queue.pop());
    mrf_debug("mrf_foreground : got bnum %d\n",bnum);

    if (bnum >= MRF_BNUM_SIGNAL_BASE){
      mrf_debug("mrf_foreground : calling signal_handler - signum %d\n",bnum - MRF_BNUM_SIGNAL_BASE);
      rv = signal_handler(bnum - MRF_BNUM_SIGNAL_BASE);  // signal_handler must be defined by app
    }
    else
      rv = _mrf_ex_buffer(bnum);
    mrf_debug("rv was %d\n",rv);
    cnt++;
  }
  //mrf_debug("no more data available returning after %d cmds",cnt);
  return cnt;
}

static void if_to_idle(IF_STATUS *ifs){

  ifs->tx_complete = 0;
  ifs->waiting_resp = 0;
  ifs->resp_timer = 0;
  ifs->timer = 0;
  ifs->tx_retried = 0;
  ifs->resp_received = 0;
  ifs->state = IDLE;

}

// should only be called by _mrf_tick
static int send_next_queued(int i,const MRF_IF *mif){
  BuffQueue *qp;
  MRF_BUFF_STATE *bs;
  IF_STATUS *ifs =  mif->status;
  uint8 bnum,bn;
  MRF_PKT_HDR *pkt;
  uint8 *tb;

  qp = mif->txqueue;
  if (qp->data_avail()==0)
    {
    mrf_debug("%s","ERROR -send_next_queued nothing in txqueue! \n");
    ifs->stats.st_err += 1;
    //ifs->state = MRF_ST_IDLE;
    return -1;
    }

  bnum = *(qp->head());

  bs = _mrf_buff_state(bnum);
  pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);

  mrf_debug("send_next_queued : FOUND txqueue -IF = %d buffer is %d buff state %d  tc %d tx_complete %d\n",
            i,bnum,bs->state,_tick_count,ifs->tx_complete);

  if (ifs->tx_complete){
    mrf_debug("ERROR - send_next_queued tx_complete on i_f %d\n",i);
    return(-1);
  }

  if (ifs->waiting_resp){
    mrf_debug("ERROR - send_next_queued waiting_resp on i_f %d\n",i);
    return(-1);
  }
  // if (_tick_count >10)
  //  exit(1);


  tb = _mrf_buff_ptr(bnum);
  //mrf_debug("calling send func\n");
  //bs->retry_count++;
  if (ifs->resp_timer != 0 ){
    mrf_debug("ERROR !! resp_timer %d when sending next queued\n",ifs->resp_timer);

  }

  bs->state = TX;
  if (needs_ack(pkt->type)){
    ifs->resp_timer = 100;
    ifs->waiting_resp = 1;
  }
  else{
    ifs->resp_timer = 0;
    ifs->waiting_resp = 0;
  }

  ifs->tx_complete = 0;
  ifs->resp_received = 0;

  ifs->state = TX_BUFF;
  //else
  mrf_debug("send_next_queued - send buff %d tc %d retry_count %d  i_f %d state to %s\n",
            bnum,_tick_count,bs->retry_count,
            i,mrf_if_state_name((I_F)i));

  //ifs->stats.tx_pkts += 1;   // only inc this stat if ack recieved - i.e. in ack/resp handlers
  if((*(mif->type->funcs.send))((I_F)i,tb)==-1){
    //send func errored - back out and send NDR

    bn = *(qp->pop());
    mrf_debug("ERROR send func failed on i_f %d while sending bnum %d",i,bnum);
    if (bn!=bnum){
      mrf_debug("ERROR!! - popped bn %d != bnum %d previously at head!",bn,bnum);

    }
    mrf_ndr(NDR_I_F_ERROR,bnum);  // this reuses buffer for ndr

    if_to_idle(ifs);

    return -1;
  }

  ifs->timer = TX_TIMEOUT_VAL;

  return 0;
}


int send_next_ack(int i,const MRF_IF *mif){
  const ACK_TAG *atp;
  IF_STATUS *ifs =  mif->status;

  if (mif->ackqueue->data_avail() == 0){
    mrf_debug("%s","ERROR -nothing in ackqueue! \n");
    ifs->stats.st_err += 1;
    return -1;
  }

  if (ifs->tx_complete){
    mrf_debug("ERROR - send_next_ack tx_complete on i_f %d\n",i);
    return(-1);
  }

  mrf_debug("ackqueue nitems %d items %d qip %d qop %d\n",
            mif->ackqueue->get_nitems(),mif->ackqueue->items(),
            mif->ackqueue->get_qip(),mif->ackqueue->get_qop());

  atp = mif->ackqueue->pop();

  mrf_debug("ackqueue popped nitems %d items %d qip %d qop %d\n",
            mif->ackqueue->get_nitems(),mif->ackqueue->items(),
            mif->ackqueue->get_qip(),mif->ackqueue->get_qop());

  if (atp == NULL){
    mrf_debug("%s","ERROR tick ackqueue.pop got NULL\n");
    ifs->stats.st_err += 1;
    return -1;
  }else{
    mif->ackbuff->length = sizeof(MRF_PKT_HDR);
    mif->ackbuff->hdest = atp->dest;
    mif->ackbuff->udest = atp->dest;
    mif->ackbuff->netid = MRFNET;
    mif->ackbuff->type  = atp->type;

    mif->ackbuff->hsrc = MRFID;
    mif->ackbuff->usrc = MRFID;
    mif->ackbuff->msgid = atp->msgid;

    mrf_debug("send_next_ack i_f %d  tc %d \n",i,_tick_count);
    mrf_print_packet_header(mif->ackbuff);
    ifs->tx_complete = 0;
    ifs->state = TX_ACK;

    ifs->stats.tx_acks += 1;
    if((*(mif->type->funcs.send))((I_F)i,(uint8 *)(mif->ackbuff))== -1){
      mrf_debug("ERROR i_f %d send func error sending ack",i);
      ifs->state = IDLE;
      return -1;

    }
    return 0;
  }
}


void _mrf_tick(){
  const MRF_IF *mif;
  IF_STATUS *ifs;
  int i;
  uint8 *tb;
  MRF_BUFF_STATE *bs;
  uint8 bnum;
  uint8 if_busy = 0;
  BuffQueue *qp;

  const ACK_TAG *atp;
  _tick_count++;
  if ( (_tick_count % 1000 ) == 0)
    mrf_debug("%d\n",_tick_count);
  if_busy = 0;
  for ( i = 0 ; i < NUM_INTERFACES ; i++){


    mif = mrf_if_ptr((I_F)i);
    ifs =  mif->status;
    qp = mif->txqueue;

    // dec timers
    if(ifs->timer > 0){
      if_busy = 1;
      ifs->timer--;
    }
    if(ifs->resp_timer > 0){
      if_busy = 1;
      ifs->resp_timer--;
    }




    if((ifs->state == DELAY_ACK)||(ifs->state == DELAY_BUFF)){
      if_busy = 1;
      if(ifs->timer==0){
        if(ifs->state==DELAY_ACK){
          //if(mif->status->channel_clear){
          mrf_debug(" i_f %d  DELAY_ACK complete\n",i);
          send_next_ack(i,mif);
        } else {  // should be buff , but painfully similiar logic
          if(mif->status->channel_clear){
            mrf_debug(" i_f %d  DELAY_BUFF complete\n",i);
            send_next_queued(i,mif);
          }else { // restart timer
            ifs->timer = mif->type->tx_del;
            ifs->state = DELAY_BUFF;
            mif->status->channel_clear = (*(mif->type->funcs.clear))((I_F)i);
          }
        }
        continue;
      }
    }
    else if ((ifs->state == TX_ACK)||(ifs->state == TX_BUFF)){
      if_busy = 1;

      if(ifs->tx_complete) {
        ifs->timer = 0;
        if(ifs->state == TX_ACK) { // ACK has been transmitted on channel go to idle
          ifs->stats.tx_acks++;
          if(ifs->waiting_resp){
            ifs->state = IDLE;
            ifs->tx_complete = 0;
          }
          else
            if_to_idle(ifs);
          continue;
        } else { // BUFF has been transmitted on channel - decide if can be freed etc.
          if(ifs->waiting_resp){ // go to idle - don't do anything - idle state checks this before allowing new tx buff
            ifs->tx_complete = 0;
            ifs->state = IDLE;
            continue;
          }else { // can free buff and pop queue
            bnum = *(qp->pop());
            mrf_debug("MRF_ST_TX_COMPLETE i_f %d freeing buff %d \n",i,bnum);
            _mrf_buff_free(bnum);
            ifs->stats.tx_pkts++;
            if_to_idle(ifs);
          }
        }

      }


    }



    if (ifs->state == IDLE){
      if(ifs->waiting_resp) {
        if_busy = 1;
        if(ifs->resp_received) {
          // got response for buff transfer
          bnum = *(qp->pop());
          mrf_debug("Response received on i_f %d freeing buff %d \n",i,bnum);
          _mrf_buff_free(bnum);
          ifs->stats.tx_pkts++;
          if_to_idle(ifs);

        }
        else if (ifs->tx_retried) {
          bnum = *(qp->head());
          mrf_debug("I_F %d  buffer %d has been retried - FIXME dropping for now\n",i,bnum);
          bnum = *(qp->pop());
          mrf_debug("Response received on i_f %d freeing buff %d \n",i,bnum);
          _mrf_buff_free(bnum);
          ifs->stats.tx_retried++;
          if_to_idle(ifs);
        }
        else if (ifs->resp_timer==0){
          // timeout on buffer waiting for resp
          bnum = *(qp->head()); // slightly penible to inc retry count on buffer
          bs = _mrf_buff_state(bnum);
          bs->retry_count++;
          ifs->stats.tx_retries++;
          mrf_debug("WARNING timeout waiting for response:  i_f %d  buff %d retry_count %d\n",i,bnum,bs->retry_count);
          if_to_idle(ifs);  // clear flags
        }

      }



      if (mif->ackqueue->data_avail()){ // always send ackqueue in IDLE
        mrf_debug("idle i_f %d has ackqueue data\n",i);
        if_busy = 1;

        ifs->timer = mif->type->ack_del;
        //mif->status->channel_clear = (*(mif->type->funcs.clear))((I_F)i);  // don't worry about CCA for acks
        ifs->state = DELAY_ACK;
      }

      else if((ifs->waiting_resp==0)&&qp->data_avail()){
        if_busy = 1;
        mrf_debug("idle i_f %d has queue data and is not waiting for response\n",i);
        bnum = *(qp->head());

        bs = _mrf_buff_state(bnum);
        mrf_debug("queue_head bnum %d retry_count %d\n",bnum,bs->retry_count);

        if(bs->retry_count >= _MRF_MAX_RETRY){
          mrf_debug("WARN retry limit reached  i_f %d bnum %d retry_count %d - abort buffer tx\n",i,bnum,bs->retry_count);
          ifs->stats.tx_errors++;
          mrf_debug("%s","retry limit reached - abort buffer tx\n");
          //ifs->stats.tx_errors++;



#if 0
          // maybe send NDR here ..but in meantime
          bs->state = FREE;   // effectively mrf_buff_free
          bs->owner = NUM_INTERFACES;
#else
          mrf_ndr(NDR_MAX_RETRIES,bnum);  // this reuses buffer for ndr
#endif
          qp->pop();
          if_to_idle(ifs);

        }else {
          ifs->timer = mif->type->tx_del;
          ifs->state = DELAY_BUFF;

          mrf_debug("mrf_tick : sending buffer -IF = %d buffer is %d buff state %d retry_count %d  tc %d\n",
                    i,bnum,bs->state,bs->retry_count,_tick_count);

          if (((MRF_PKT_HDR *)_mrf_buff_ptr(bnum))->type==mrf_cmd_resp)  // resp just goes on timer, like ack
            mif->status->channel_clear = 1;
          else
            mif->status->channel_clear = (*(mif->type->funcs.clear))((I_F)i);

        }

      }


    }
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
