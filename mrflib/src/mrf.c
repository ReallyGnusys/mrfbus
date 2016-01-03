#include <mrf.h>
#include <stdio.h>

#define _QUOTE_SYM_(SYM)  #SYM
#define SYM_NAME(name) _QUOTE_SYM_(name)
#define _CONCAT_(A,B) # A ## B

#define _DEVNAME_STR_  SYM_NAME(DEVTYPE)

//#define _DEVNAME_STR_  SYM_NAME(_CONCAT_(DEVTYPE,MRFID))

const MRF_PKT_DEVICE_INFO device_info  = {_DEVNAME_STR_ ,"1", GITSH };
int _print_mrf_cmd(MRF_CMD_CODE cmd);

int mrf_init(){
#ifdef MRF_ARCH_lnx
  mrf_debug("mrf_init: entry\n");
  _print_mrf_cmd(mrf_cmd_device_info);
  mrf_if_init();
  mrf_sys_init();
  return mrf_arch_init();
#else
  mrf_arch_init();
  mrf_if_init();
  mrf_sys_init();
  return 0;
#endif
}

/*
int mrf_main_loop(){
  return mrf_arch_main_loop();
}
*/
int mrf_time(char *buff){
  TIMEDATE td;
  mrf_rtc_get(&td);
  return sprintf(buff,"%02d:%02d:%02d %02d-%02d-%04d",td.hour,td.min,td.sec,td.day,td.mon,td.year+2000);
}


