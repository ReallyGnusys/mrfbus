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
 MRF_PKT_HDR sack;

int mrf_sack(uint8 bnum){
 MRF_PKT_HDR *hdr = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum); 

 MRF_ROUTE route;
 //mrf_debug("mrf_sack :  bnum %d  \n",bnum);
 sack.length = sizeof(MRF_PKT_HDR);
 sack.udest = hdr->hsrc;
 sack.hdest = hdr->hsrc;
 sack.hsrc = MRFID;
 sack.usrc = MRFID;
 sack.netid = MRFNET;

 sack.usrc = MRFID;
 sack.type = mrf_cmd_ack;  


 mrf_nexthop(&route,MRFID,sack.hdest);
 mrf_tx_buffer(route.i_f,(uint8 *)&sack);

}


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
 
 mrf_tx_bnum(route.i_f,bnum);
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


  // lookup command
  const MRF_CMD *cmd = (const MRF_CMD *) &(mrf_cmds[pkt->type]);


  // check if we are udest

  if ( pkt->udest == MRFID)
    {
      //mrf_debug("INFO: EXECUTE PACKET UDEST %02X is us %02X \n",pkt->udest,MRFID);
      // printf("cmd name %s  len %d \n",cmd->str,cmd->size);
      if( ( cmd->data != NULL )  && ( cmd->size > 0 ) && ( (cmd->cflags & MRF_CFLG_NO_RESP) == 0)) {
        //printf("sending data response \n");
  
        mrf_data_response(bnum,cmd->data,cmd->size);
        return;

      }
      if(cmd->func != NULL){
        (*(cmd->func))(pkt->type,bnum);
      }
      if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
        mrf_sack(bnum);

      }
      
    }
  else{
    //otherwise send segment ack then forward on network

    if((cmd->cflags & MRF_CFLG_NO_ACK) == 0){
      mrf_sack(bnum);
    }
    MRF_ROUTE route;

    mrf_nexthop(&route,MRFID,pkt->udest);
    mrf_debug("INFO:  UDEST %02X : forwarding to %d on I_F %d \n",pkt->udest,route.relay,route.i_f);
    
    pkt->hdest = route.relay;
    pkt->hsrc = MRFID;
  
    mrf_tx_bnum(route.i_f,bnum);

  }
  
}
