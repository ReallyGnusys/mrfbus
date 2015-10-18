#include "xb_buff.h"


// mrfbus routes packets contained in buffers defined in xb_buff.h
// packets destined for local device are processed locally


int mrf_init(){
  xb_buff_init();
}

const XB_PKT_DEVICE_INFO device_info  = {XBDEVNAME ,"1", GITSH };

volatile XB_PKT_TIMEDATE xb_timedate;
