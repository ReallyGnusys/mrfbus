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
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include "mrf_route.h"
#include "mrf_relays.h"
//extern uint8 _mrfid;
static uint8 buff[2048];


#ifndef SERVER_PORT
#define SERVER_PORT  3141
#endif


// this is lnx  - we can use some ram

//static uint8 _last_msg_ids[256];   // keep track of last msg ids received from every possible address


int init_sockaddr (struct sockaddr_in *name,
               const char *hostname,
               uint16_t port)
{
  struct hostent *hostinfo;

  name->sin_family = AF_INET;
  name->sin_port = htons (port);
  hostinfo = gethostbyname (hostname);
  if (hostinfo == NULL)
    {
      mrf_debug(5,"%s","init_sockaddr error getting hostinfo for localhost");
      return -1;
    }
  name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
  return 0;
}



int tick_task();


int signal_handler(uint8 signal){
    if (signal == APP_SIG_TICK)
    return tick_task();
  return 0;
}

static int appfd , servfd;  // file desc for listener socket




static int _outfd,_outfds;  // file descs for output pipes - response and structure

static int _app_tick_cnt;
int mrf_app_init(){
  char sname[64];
  int  tmp;
  _app_tick_cnt = 0;
  mrf_debug(5,"%s","mrf_app_init host\n");
  init_relays();
  mrf_app_tick_enable(2);

  //mrf_arch_app_callback(_appl_fifo_callback);  // we want arch to manage a TCP server for server.py to access

}

void print_mstats(MRF_PKT_LNX_MEM_STATS *mstats){

  mrf_debug(5,"%s","**************************************\n");
  mrf_debug(5,"%s","MRF_PKT_LNX_MEM_STATS\n");
  mrf_debug(5," sz 0x%x res 0x%x share 0x%x text 0x%x  lib 0x%x data 0x%x\n",
            mstats->sz,mstats->res,mstats->share,mstats->text,mstats->lib,mstats->data);
  mrf_debug(5," relay_state 0x%x \n",mstats->relay_state);
  mrf_debug(5,"%s","**************************************\n");
}

MRF_CMD_RES mrf_task_usr_struct(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  uint8 *buff = (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);

  mrf_debug(5,"mrf_task_usr_struct (stub) : bnum %u type = %u rlen %u msgid %u\n",bnum,resp->type,resp->rlen,resp->msgid);
  _mrf_print_hex_buff(buff,buff[0]);

  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"DEVICE %s MRFID 0x%X MRFNET 0x%X num_buffs %d num_ifs %d\n",dev_inf->dev_name,dev_inf->mrfid,dev_inf->netid,
              dev_inf->num_buffs,dev_inf->num_ifs);
  }
  else if  (resp->type == mrf_cmd_device_status){
    MRF_PKT_DEVICE_STATUS *if_inf = (MRF_PKT_DEVICE_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"DEVICE_STATUS   free_buffs %u rx_pkts %u tx_pkts %u tx_retries %u tick_count %u\n",
              if_inf->buffs_free, if_inf->rx_pkts, if_inf->tx_pkts, if_inf->tx_retries,if_inf->tick_count);
  }
  else if (resp->type == mrf_cmd_sys_info){
    MRF_PKT_SYS_INFO *dev_inf = (MRF_PKT_SYS_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"SYS INFO num_cmds %u mrfbus version %s modified %u build %s\n",
              dev_inf->num_cmds, dev_inf->mrfbus_version, dev_inf->modified, dev_inf->build);
  }
  else if  (resp->type == mrf_cmd_if_stats){
    IF_STATS *if_stats = (IF_STATS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"IF STATUS : rx_pkts %u tx_pkts %u tx_acks %u tx_retries %u tx_overruns %u unexp_ack %u alloc_err %u st_err %u\n",if_stats->rx_pkts,if_stats->tx_pkts,if_stats->tx_acks,if_stats->tx_retries,if_stats->tx_overruns, if_stats->unexp_ack, if_stats->alloc_err, if_stats->st_err);
  }
  else if  (resp->type == mrf_cmd_get_time){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }
  else if  (resp->type == mrf_cmd_buff_state){
    MRF_PKT_BUFF_STATE *bst = (MRF_PKT_BUFF_STATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    char *bstnames[] = {"FREE","LOADING","LOADED","TXQUEUE","TX","APPIN"};
    if (bst->id > (_MRF_BUFFS - 1))
      mrf_debug(5,"BUFF_STATE : got invalid id %u\n",bst->id);
    else
      mrf_debug(5,"BUFF_STATE : buff %u state %s\n", bst->id, bstnames[bst->state.state]);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }
  else if (resp->type == mrf_cmd_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if (resp->type == mrf_cmd_app_info){
    MRF_PKT_APP_INFO *app_inf = (MRF_PKT_APP_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"APP INFO name %s num_cmds %u\n",
              app_inf->name,app_inf->num_cmds);
  }
  else if (resp->type == mrf_cmd_app_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"APP CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if  (resp->type == mrf_cmd_test_1){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }
  else if  (resp->type == mrf_cmd_usr_struct){
    char *buff = (char *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"%s","usr_struct : this is slightly bonkers \n");
    /*
    mrf_debug(5,"%s","hex buff follows:");
    _mrf_print_hex_buff((uint8 *)buff,sizeof(MRF_PKT_DBG_CHR32));
    mrf_debug(5,":end of hex");
*/
  }

  else if  (resp->type >= _MRF_APP_CMD_BASE){
    mrf_debug(5,"_MRF_APP_CMD  %d ( mrf_cmd code %d)\n",resp->type - _MRF_APP_CMD_BASE , resp->type);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }


  _mrf_buff_free(bnum);
}

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  /*
    prints response packets
  */
  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  uint8 *buff = (uint8 *)(_mrf_buff_ptr(bnum)+ 0L);

  mrf_debug(5,"mrf_task_usr_resp (stub) : bnum %u type = %u rlen %u msgid %u\n",bnum,resp->type,resp->rlen,resp->msgid);
  _mrf_print_hex_buff(buff,buff[0]);

  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"DEVICE %s MRFID 0x%X MRFNET 0x%X num_buffs %d num_ifs %d\n",dev_inf->dev_name,dev_inf->mrfid,dev_inf->netid,
              dev_inf->num_buffs,dev_inf->num_ifs);
  }
  else if  (resp->type == mrf_cmd_device_status){
    MRF_PKT_DEVICE_STATUS *if_inf = (MRF_PKT_DEVICE_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"DEVICE_STATUS   free_buffs %u rx_pkts %u tx_pkts %u tx_retries %u tick_count %u\n",
              if_inf->buffs_free, if_inf->rx_pkts, if_inf->tx_pkts, if_inf->tx_retries,if_inf->tick_count);
  }
  else if (resp->type == mrf_cmd_sys_info){
    MRF_PKT_SYS_INFO *dev_inf = (MRF_PKT_SYS_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"SYS INFO num_cmds %u mrfbus version %s modified %u build %s\n",
              dev_inf->num_cmds, dev_inf->mrfbus_version, dev_inf->modified, dev_inf->build);
  }
  else if  (resp->type == mrf_cmd_if_stats){
    IF_STATS *if_stats = (IF_STATS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"IF STATUS : rx_pkts %u tx_pkts %u tx_acks %u tx_retries %u tx_overruns %u unexp_ack %u alloc_err %u st_err %u\n",if_stats->rx_pkts,if_stats->tx_pkts,if_stats->tx_acks,if_stats->tx_retries,if_stats->tx_overruns, if_stats->unexp_ack, if_stats->alloc_err, if_stats->st_err);
  }
  else if  (resp->type == mrf_cmd_get_time){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }
  else if  (resp->type == mrf_cmd_buff_state){
    MRF_PKT_BUFF_STATE *bst = (MRF_PKT_BUFF_STATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    char *bstnames[] = {"FREE","LOADING","LOADED","TXQUEUE","TX","APPIN"};
    if (bst->id > (_MRF_BUFFS - 1))
      mrf_debug(5,"BUFF_STATE : got invalid id %u\n",bst->id);
    else
      mrf_debug(5,"BUFF_STATE : buff %u state %s\n", bst->id, bstnames[bst->state.state]);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }
  else if (resp->type == mrf_cmd_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if (resp->type == mrf_cmd_app_info){
    MRF_PKT_APP_INFO *app_inf = (MRF_PKT_APP_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"APP INFO name %s num_cmds %u\n",
              app_inf->name,app_inf->num_cmds);
  }
  else if (resp->type == mrf_cmd_app_cmd_info){
    MRF_PKT_CMD_INFO *cmd_inf = (MRF_PKT_CMD_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"APP CMD INFO type %u name %s cflags 0x%02X req_size %u rsp_size %u\n",
              cmd_inf->type,cmd_inf->name,cmd_inf->cflags,cmd_inf->req_size,cmd_inf->rsp_size);
  }
  else if  (resp->type == mrf_cmd_test_1){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }
  else if  (resp->type == mrf_cmd_usr_struct){
    //char *buff = (char *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"%s","usr_struct : fixme for output \n");
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)buff,sizeof(MRF_PKT_DBG_CHR32));
    mrf_debug(5,":end of hex");
*/
  }

  else if  (resp->type == _MRF_APP_CMD_BASE){
    TIMEDATE *td = (TIMEDATE *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug(5,"_MRF_APP_CMD_BASE TIMEDATE : %u-%u-%u  %u:%u:%u \n",td->day,td->mon,td->year,td->hour,td->min,td->sec);
    /*
    mrf_debug(5,"hex buff follows:");
    _mrf_print_hex_buff((uint8 *)td,sizeof(TIMEDATE));
    mrf_debug(5,":end of hex");
    */
  }

  _mrf_buff_free(bnum);

}



MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug(5,"%s","mrf_app_task_test entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get((TIMEDATE *)rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug(5,"%s","mrf_app_task_test exit\n");
  return MRF_CMD_RES_OK;
}

int get_mem_stats(MRF_PKT_LNX_MEM_STATS *mst){

  //MRF_PKT_LNX_MEM_STATS *mst = (MRF_PKT_LNX_MEM_STATS *)mrf_response_buffer(bnum);

  FILE* statm = fopen( "/proc/self/statm", "r" );
  mrf_rtc_get(&(mst->td));

  fscanf(statm,"%d %d %d %d %d %d %d",
         &(mst->sz),&(mst->res),&(mst->share),&(mst->text),&(mst->lib),&(mst->data),&(mst->dt));
  fclose(statm);

  mrf_debug(5,"get_mem_stats sz %d  mst->res %d\n",mst->sz,mst->res);

  // extra features..
  uint16 rs = relay_state();
  mst->relay_state = (uint16)relay_state();
  mrf_debug(5,"get_mem_stats rs 0x%u  mst->relay_state 0x%u\n",rs,mst->relay_state);

  return 0;

}

static MRF_PKT_LNX_MEM_STATS _memstats;

int tick_task(){
  _app_tick_cnt++;
  get_mem_stats(&_memstats);

  if ((_app_tick_cnt > 2)){
    mrf_send_structure(0,  _MRF_APP_CMD_BASE + mrf_app_cmd_mstats,  (uint8 *)&_memstats, sizeof(MRF_PKT_LNX_MEM_STATS));
  }
  return 0;

}


MRF_CMD_RES mrf_app_mem_stats(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  mrf_debug(5,"%s","mrf_app_mem_stats entry\n");

  get_mem_stats((MRF_PKT_LNX_MEM_STATS *)mrf_response_buffer(bnum));

  mrf_send_response(bnum,sizeof(MRF_PKT_LNX_MEM_STATS));
  mrf_debug(5,"%s","mrf_app_mem_stats exit\n");
  return MRF_CMD_RES_OK;
}




MRF_CMD_RES mrf_app_set_relay(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_RELAY_STATE *rs;
  mrf_debug(5,"%s","mrf_app_set_relay entry\n");
  rs = (MRF_PKT_RELAY_STATE *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  set_relay_state(rs->chan,rs->val);
  rs->val = get_relay_state(rs->chan);
  mrf_debug(5,"mrf_app_set_relay exit rs->val = 0x%x relay_state 0x%x\n",rs->val,relay_state());

  mrf_data_response( bnum,(uint8 *)rs,sizeof(MRF_PKT_RELAY_STATE));
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_app_get_relay(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp){
  MRF_PKT_RELAY_STATE *rs;
  mrf_debug(5,"%s","mrf_app_get_relay entry\n");
  rs = (MRF_PKT_RELAY_STATE *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  rs->val = get_relay_state(rs->chan);
  mrf_data_response( bnum,(uint8 *)rs,sizeof(MRF_PKT_RELAY_STATE));
  return MRF_CMD_RES_OK;
}
