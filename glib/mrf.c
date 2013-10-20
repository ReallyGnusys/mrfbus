#include <mrf.h>
#include <stdio.h>


const MRF_PKT_DEVICE_INFO device_info  = {XBDEVNAME ,"1", SVN };

int mrf_init(){  
  mrf_if_init();
  return mrf_arch_init();
}


int mrf_main_loop(){
  return mrf_arch_main_loop();
}

int mrf_time(char *buff){
  TIMEDATE td;
  mrf_rtc_get(&td);
  sprintf(buff,"%02d:%02d:%02d %02d-%02d-%04d",td.hour,td.min,td.sec,td.day,td.mon,td.year+2000);
}


