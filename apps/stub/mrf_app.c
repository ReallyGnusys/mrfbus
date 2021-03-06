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

#include "mrf_sys.h"
#include <mrf_debug.h>

#include "mrf_arch.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mrf_route.h"

//extern uint8 _mrfid;
static uint8 buff[2048];


// this is lnx  - we can use some ram

//static uint8 _last_msg_ids[256];   // keep track of last msg ids received from every possible address


int signal_handler(uint8 signal){
  return 0;
}

static MRF_CMD_RES _appl_fifo_callback(int fd){
  ssize_t s;
  uint8 i;
  mrf_debug("fifo callback for fd %d\n",fd);
  mrf_debug("event on fd %d\n",fd);
  s = read(fd, (char *)buff, 1024); // FIXME need to handle multiple packets
  buff[s] = 0;

  mrf_debug("read %d bytes\n",(int)s);
  _mrf_print_hex_buff(buff,s);

  if ( s < 4){
    mrf_debug("%s","ignoring pkt with len too small \n");
    return MRF_CMD_RES_WARN;
  }
  
  uint8 len = buff[0];
  
    
  
  if (s != len){
    mrf_debug("not credible input buff[0] %u should be same as message length %u \n",len,(unsigned int)s);
    return MRF_CMD_RES_WARN;
  }

  uint8 csum = 0;

  for (i = 0 ; i < len-1 ; i++)
    csum += buff[i];
  
  mrf_debug("calculated csum %u  received csum %u\n",csum,buff[s-1]);

  if ( csum != buff[s-1]) {
    mrf_debug("%s","packet checksum error, discarding\n");
    return MRF_CMD_RES_WARN;

  }

  uint8 udest = buff[1];
  uint8 type = buff[2];
  return  mrf_send_command(udest, type,  &(buff[3]), len -3);

  MRF_ROUTE route;

  mrf_nexthop(&route,_mrfid,udest);

  // get a buffer with null i_f 
  uint8 bnum = mrf_alloc_if(route.i_f);

  MRF_PKT_HDR *pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);



  
  mrf_debug("allocated buffer number %u\n",bnum);

  if (bnum >= _MRF_BUFFS){
    mrf_debug("%s","no buffs left\n");
    return MRF_CMD_RES_ERROR;
  }
  
  pkt->udest = buff[1];
  pkt->type = buff[2];
  pkt->hdest = route.relay;
  pkt->hsrc = _mrfid;
  pkt->usrc = _mrfid;
  pkt->netid = MRFNET;
  // copy and payload bytes
  uint8 *payload = (uint8 *)pkt + sizeof(MRF_PKT_HDR);
  pkt->length = sizeof(MRF_PKT_HDR);

  for (i = 3 ; i < len-1 ; i++){
    payload[i - 3] = buff[i];
    pkt->length++;
  }
  mrf_debug("%s","_appl_fifo_callback : trying to send packet with header:\n");
  mrf_print_packet_header(pkt);
  if( mrf_if_tx_queue(route.i_f,bnum) == -1) {// then outgoing queue full - need to retry
    mrf_debug("%s","looking dodgy sending packet\n");
    mrf_retry(route.i_f,bnum);
  } else {
    mrf_debug("INFO:  UDEST %02X : forwarding to %02X on I_F %d\n",pkt->udest,route.relay,route.i_f);  
  }

}
static int _outfd,_outfds;  // file descs for output pipes - response and structure

int mrf_app_init(){
  char sname[64];
  int appfd, tmp;
  mrf_debug("%s","mrf_app_init stub\n");
  //for (tmp = 0 ; tmp < 256 ; tmp++)
  //  _last_msg_ids = 0 ;
  // need to open input application pipe


#if 1  // no , sadly we will achieve deadlock by trying to open this at start, as
       // the other end ( mrfland ) is trying to do the same thing in the other
       // direction. Please FIXME, get rid of named pipes for comms, and use sockets in preference.
       // in meantime we're opening each time time for writing ... see below
  // open output application response pipe
  sprintf(sname,"%s%d-app-out",SOCKET_DIR,_mrfid);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  mrf_debug("created pipe %s res %d\n",sname,tmp);
  _outfd = open(sname,O_WRONLY);
  mrf_debug("opened out pipe %s fd = %d\n",sname,_outfd);

  // open output application struct pipe  - we need a link partner... hmpff maybe
  sprintf(sname,"%s%d-app-str",SOCKET_DIR,_mrfid);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  mrf_debug("created pipe %s res %d\n",sname,tmp);
  _outfds = open(sname,O_WRONLY);
  mrf_debug("opened out pipe %s fd = %d\n",sname,_outfds);
  mrf_debug("%s","\nmrf_app_init exit\n\n");
 #endif

  sprintf(sname,"%s%d-app-in",SOCKET_DIR,_mrfid);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  mrf_debug("created pipe %s res %d\n",sname,tmp);
  appfd = open(sname,O_RDONLY | O_NONBLOCK);
  mrf_debug("opened pipe %s fd = %d\n\n",sname,appfd);
  mrf_arch_app_callback(appfd,_appl_fifo_callback);

  
}
int structure_to_app(uint8 bnum){
  // just squirt the raw buffer to python app via fifo
  char sname[64];
  uint8 *buff =  (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);
  uint8 len = buff[0];
  mrf_debug("structure to app : bnum %d using opened app data pipe fd = %d\n", bnum, _outfds);


  if(len > _MRF_BUFFLEN){
    mrf_debug("error - length is bonkers %u\n",len);
    return -1;
  }
#if 0
  // FIXME - should prefer sockets
 // open output application struct pipe  - we need a link partner... hmpff maybe
  sprintf(sname,"%s%d-app-str",SOCKET_DIR,_mrfid);
  int tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  mrf_debug("created pipe %s res %d\n",sname,tmp);
  _outfds = open(sname,O_WRONLY | O_NONBLOCK);
  mrf_debug("opened out pipe %s fd = %d\n",sname,_outfds);
#endif
  int bc = write(_outfds, buff,len );

  // close(_outfds);
 mrf_debug("wrote %d bytes to outpipe %s\n",bc,sname);
 _mrf_print_hex_buff(buff,len);
 return 0;
}





int response_to_app(uint8 bnum){
  // just squirt the raw buffer to python app via fifo
  char sname[64];
  uint8 *buff =  (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);
  uint8 len = buff[0];

  if(len > _MRF_BUFFLEN){
    mrf_debug("error - length is bonkers %u\n",len);
    return -1;
  }
#if 0
  // FIXME - we shouldn't be doing this - move to socket connection for stub
  sprintf(sname,"%s%d-app-out",SOCKET_DIR,_mrfid);
  int tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  mrf_debug("created pipe %s res %d\n",sname,tmp);
  _outfd = open(sname,O_WRONLY | O_NONBLOCK);
  mrf_debug("opened out pipe %s fd = %d\n",sname,_outfd);
#endif
  int bc = write(_outfd, buff,len );


  mrf_debug("response to app :wrote %d bytes to outpipe %s from buff %d\n",bc,sname,bnum);
  _mrf_print_hex_buff(buff,len);
  //  close(_outfd);
  return 0;
}

MRF_CMD_RES mrf_task_usr_struct(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  uint8 *buff = (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);

  mrf_debug("mrf_task_usr_struct (stub) : bnum %u type = %u rlen %u msgid %u\n",bnum,resp->type,resp->rlen,resp->msgid);
  _mrf_print_hex_buff(buff,buff[0]);

  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE %s MRFID 0x%X MRFNET 0x%X num_buffs %d num_ifs %d\n",dev_inf->dev_name,dev_inf->mrfid,dev_inf->netid,
              dev_inf->num_buffs,dev_inf->num_ifs);
  }
  else if  (resp->type == mrf_cmd_device_status){
    MRF_PKT_DEVICE_STATUS *if_inf = (MRF_PKT_DEVICE_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE_STATUS   free_buffs %u rx_pkts %u tx_pkts %u tx_retries %u tick_count %u\n",
              if_inf->buffs_free, if_inf->rx_pkts, if_inf->tx_pkts, if_inf->tx_retries,if_inf->tick_count);
  }
  else if (resp->type == mrf_cmd_sys_info){
    MRF_PKT_SYS_INFO *dev_inf = (MRF_PKT_SYS_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("SYS INFO num_cmds %u mrfbus version %s modified %u build %s\n",
              dev_inf->num_cmds, dev_inf->mrfbus_version, dev_inf->modified, dev_inf->build);
  }
  else if  (resp->type == mrf_cmd_if_stats){
    IF_STATS *if_stats = (IF_STATS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("IF STATUS : rx_pkts %u tx_pkts %u tx_acks %u tx_retries %u tx_overruns %u unexp_ack %u alloc_err %u st_err %u\n",if_stats->rx_pkts,if_stats->tx_pkts,if_stats->tx_acks,if_stats->tx_retries,if_stats->tx_overruns, if_stats->unexp_ack, if_stats->alloc_err, if_stats->st_err);
  }  
  else if  (resp->type == mrf_cmd_get_time){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  else if  (resp->type == mrf_cmd_buff_state){
    MRF_PKT_BUFF_STATE *bst = (MRF_PKT_BUFF_STATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    char *bstnames[] = {"FREE","LOADING","LOADED","TXQUEUE","TX","APPIN"};
    if (bst->id > (_MRF_BUFFS - 1))
      mrf_debug("BUFF_STATE : got invalid id %u\n",bst->id);
    else
      mrf_debug("BUFF_STATE : buff %u state %s\n", bst->id, bstnames[bst->state.state]);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  else if (resp->type == mrf_cmd_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if (resp->type == mrf_cmd_app_info){
    MRF_PKT_APP_INFO *app_inf = (MRF_PKT_APP_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("APP INFO name %s num_cmds %u\n",
              app_inf->name,app_inf->num_cmds);
  }
  else if (resp->type == mrf_cmd_app_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("APP CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if  (resp->type == mrf_cmd_test_1){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  else if  (resp->type == mrf_cmd_usr_struct){
    char *buff = (char *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("%s","usr_struct : this is slightly bonkers \n");
    /*
    mrf_debug("%s","hex buff follows:");
    _mrf_print_hex_buff((uint8 *)buff,sizeof(MRF_PKT_DBG_CHR32));
    mrf_debug(":end of hex");
*/
  }  

  else if  (resp->type >= _MRF_APP_CMD_BASE){
    mrf_debug("_MRF_APP_CMD  %d ( mrf_cmd code %d)\n",resp->type - _MRF_APP_CMD_BASE , resp->type);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  
  // squirt complete response packet to higher level app
  mrf_debug("%s","about to send response\n");
  int rc = structure_to_app(bnum);
  mrf_debug("sent structur.. I think rc %d\n",rc);
  

  _mrf_buff_free(bnum);
}

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  /*
    prints response packets
  */
  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  uint8 *buff = (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);

  mrf_debug("mrf_task_usr_resp (stub) : bnum %u type = %u rlen %u msgid %u\n",bnum,resp->type,resp->rlen,resp->msgid);
  _mrf_print_hex_buff(buff,buff[0]);

  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE %s MRFID 0x%X MRFNET 0x%X num_buffs %d num_ifs %d\n",dev_inf->dev_name,dev_inf->mrfid,dev_inf->netid,
              dev_inf->num_buffs,dev_inf->num_ifs);
  }
  else if  (resp->type == mrf_cmd_device_status){
    MRF_PKT_DEVICE_STATUS *if_inf = (MRF_PKT_DEVICE_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE_STATUS   free_buffs %u rx_pkts %u tx_pkts %u tx_retries %u tick_count %u\n",
              if_inf->buffs_free, if_inf->rx_pkts, if_inf->tx_pkts, if_inf->tx_retries,if_inf->tick_count);
  }
  else if (resp->type == mrf_cmd_sys_info){
    MRF_PKT_SYS_INFO *dev_inf = (MRF_PKT_SYS_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("SYS INFO num_cmds %u mrfbus version %s modified %u build %s\n",
              dev_inf->num_cmds, dev_inf->mrfbus_version, dev_inf->modified, dev_inf->build);
  }
  else if  (resp->type == mrf_cmd_if_stats){
    IF_STATS *if_stats = (IF_STATS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("IF STATUS : rx_pkts %u tx_pkts %u tx_acks %u tx_retries %u tx_overruns %u unexp_ack %u alloc_err %u st_err %u\n",if_stats->rx_pkts,if_stats->tx_pkts,if_stats->tx_acks,if_stats->tx_retries,if_stats->tx_overruns, if_stats->unexp_ack, if_stats->alloc_err, if_stats->st_err);
  }  
  else if  (resp->type == mrf_cmd_get_time){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  else if  (resp->type == mrf_cmd_buff_state){
    MRF_PKT_BUFF_STATE *bst = (MRF_PKT_BUFF_STATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    char *bstnames[] = {"FREE","LOADING","LOADED","TXQUEUE","TX","APPIN"};
    if (bst->id > (_MRF_BUFFS - 1))
      mrf_debug("BUFF_STATE : got invalid id %u\n",bst->id);
    else
      mrf_debug("BUFF_STATE : buff %u state %s\n", bst->id, bstnames[bst->state.state]);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  else if (resp->type == mrf_cmd_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if (resp->type == mrf_cmd_app_info){
    MRF_PKT_APP_INFO *app_inf = (MRF_PKT_APP_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("APP INFO name %s num_cmds %u\n",
              app_inf->name,app_inf->num_cmds);
  }
  else if (resp->type == mrf_cmd_app_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("APP CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if  (resp->type == mrf_cmd_test_1){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  else if  (resp->type == mrf_cmd_usr_struct){
    //char *buff = (char *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("%s","usr_struct : fixme for output \n");
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)buff,sizeof(MRF_PKT_DBG_CHR32));
    mrf_debug(":end of hex");
*/
  }  

  else if  (resp->type == _MRF_APP_CMD_BASE){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("_MRF_APP_CMD_BASE TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug("hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(":end of hex");
    */
  }  
  
  // squirt complete response packet to higher level app
  mrf_debug("%s","about to send response\n");
  int rc = response_to_app(bnum);
  mrf_debug("send response.. I think rc %d\n",rc);
  _mrf_buff_free(bnum);

}



MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug("%s","mrf_app_task_test entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get((TIMEDATE *)rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug("%s","mrf_app_task_test exit\n");
  return MRF_CMD_RES_OK;
}



