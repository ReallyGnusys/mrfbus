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

#include "mrf_route.h"

extern uint8 _mrfid;
static uint8 buff[2048];


static MRF_CMD_RES _appl_fifo_callback(int fd){
  ssize_t s;
  uint8 i;
  mrf_debug("fifo callback for fd %d\n",fd);
  printf("event on fd %d\n",fd);
  s = read(fd, (char *)buff, 1024); // FIXME need to handle multiple packets
  buff[s] = 0;

  printf("read %d bytes\n",(int)s);
  _mrf_print_hex_buff(buff,s);

  if ( s < 4){
    mrf_debug("ignoring pkt with len too small \n");
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
    mrf_debug("packet checksum error, discarding\n");
    return MRF_CMD_RES_WARN;

  }

  uint8 bnum = mrf_alloc_if(NUM_INTERFACES);
  mrf_debug("allocated buffer number %u\n",bnum);

  if (bnum >= _MRF_BUFFS){
    mrf_debug("no buffs left\n");
    return MRF_CMD_RES_ERROR;
  }
  MRF_PKT_HDR *pkt = (MRF_PKT_HDR *)_mrf_buff_ptr(bnum);
  
  pkt->udest = buff[1];
  pkt->type = buff[2];
  MRF_ROUTE route;
  mrf_nexthop(&route,_mrfid,pkt->udest);
  pkt->hdest = route.relay;
  pkt->hsrc = _mrfid;
  pkt->usrc = _mrfid;
  // copy and payload bytes
  uint8 *payload = (uint8 *)pkt + sizeof(MRF_PKT_HDR);
  pkt->length = sizeof(MRF_PKT_HDR);

  for (i = 3 ; i < len-1 ; i++){
    payload[i - 3] = buff[i];
    pkt->length++;
  }
  if( mrf_if_tx_queue(route.i_f,bnum) == -1) {// then outgoing queue full - need to retry
    mrf_debug("looking dodgy sending packet\n");
    mrf_retry(route.i_f,bnum);
  } else {
    mrf_debug("INFO:  UDEST %02X : forwarding to %02X on I_F %d\n",pkt->udest,route.relay,route.i_f);  
  }

}
static int _outfd;

int mrf_app_init(){
  char sname[64];
  int appfd, tmp;
  mrf_debug("mrf_app_init stub\n");

  // need to open input application pipe

  sprintf(sname,"%s%d-app-in",SOCKET_DIR,_mrfid);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  printf("created pipe %s res %d\n",sname,tmp);
  appfd = open(sname,O_RDONLY | O_NONBLOCK);
  printf("opened pipe %s fd = %d\n\n",sname,appfd);
  mrf_arch_app_callback(appfd,_appl_fifo_callback);

  // open output application pipe

  sprintf(sname,"%s%d-app-out",SOCKET_DIR,_mrfid);
  tmp = mkfifo(sname,S_IRUSR | S_IWUSR);
  printf("created pipe %s res %d\n",sname,tmp);
  _outfd = open(sname,O_WRONLY);
  printf("opened out pipe %s fd = %d\n",sname,_outfd);

  

}

int response_to_app(bnum){
  char sname[64];
  int outfd;
  uint8 *buff =  (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);
  uint8 len = buff[0];
  sprintf(sname,"%s%d-app-out",SOCKET_DIR,_mrfid);
  outfd = open(sname, O_WRONLY | O_NONBLOCK);

  printf("response_to_app : bnum %d opened out pipe %s fd = %d\n", bnum, sname, outfd);
  if (outfd == -1 ){
    mrf_debug("failed to open fd = %d\n",outfd);
    return -1;
  }
 if(len > _MRF_BUFFLEN){
   mrf_debug("error - length is bonkers %u\n",len);
   return -1;
 }
 int bc = write(outfd, buff,len );


 mrf_debug("wrote %d bytes to outpipe %s\n",bc,sname);
 _mrf_print_hex_buff(buff,len);
 close(outfd);
 return 0;
}


MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  /*
    prints response packets
  */
  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  uint8 *buff = (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);

  mrf_debug("mrf_task_usr_resp : bnum %u type = %u rlen %u msgid %u\n",bnum,resp->type,resp->rlen,resp->msgid);
  _mrf_print_hex_buff(buff,buff[0]);

  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE %s MRFID 0x%X MRFNET 0x%X num_buffs %d num_ifs %d\n",dev_inf->dev_name,dev_inf->mrfid,dev_inf->netid,
              dev_inf->num_buffs,dev_inf->num_ifs);
  }
  else if  (resp->type == mrf_cmd_device_status){
    MRF_PKT_DEVICE_STATUS *if_inf = (MRF_PKT_DEVICE_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE_STATUS  num_buffs %u free_buffs %u rx_pkts %u tx_pkts %u tx_retries %u \n",if_inf->buffs_total,
              if_inf->buffs_free, if_inf->rx_pkts, if_inf->tx_pkts, if_inf->tx_retries);
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
  else if  (resp->type == mrf_cmd_test_2){
    char *buff = (char *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("TEST_2 : %s \n",buff);
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
  mrf_debug("about to send response\n");
  int rc = response_to_app(bnum);
  mrf_debug("send response.. I think rc %d\n",rc);
  _mrf_buff_free(bnum);

}



MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_app_task_test entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get(rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug("mrf_app_task_test exit\n");
  return MRF_CMD_RES_OK;
}



