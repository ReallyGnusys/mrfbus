#include <mrf.h>

const uint8 _mrfid = MRFID;

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){

  MRF_PKT_HDR *hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???!

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
mrf_debug("mrf_task_usr_resp :  type = %d\n",resp->type);
  // send a seg ack when we get resp


  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE %s MRFID 0x%X MRFNET 0x%X mrfbus version %s modified %u \n",dev_inf->dev_name,dev_inf->mrfid,dev_inf->netid,
              dev_inf->mrfbus_version,dev_inf->modified);
  }
  else if  (resp->type == mrf_cmd_device_status){
    MRF_PKT_DEVICE_STATUS *if_inf = (MRF_PKT_DEVICE_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    mrf_debug("DEVICE_STATUS  num_buffs %u free_buffs %u rx_pkts %u tx_pkts %u tx_retries %u \n",if_inf->buffs_total,
              if_inf->buffs_free,if_inf->rx_pkts,if_inf->tx_pkts,if_inf->tx_retries);
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
  _mrf_buff_free(bnum);

}



int main(void){
  char buff[256];
  int i;
  mrf_debug("\nmain entry\n");
  _print_mrf_cmd(mrf_cmd_device_info);

  mrf_init();
  mrf_time(buff);
  //printf("MRF_TIME IS %s\n",buff);
  //return mrf_main_loop();
  while(1){
    usleep(1000000);
    i = mrf_foreground();
  }
  
}


