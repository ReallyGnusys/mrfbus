#ifndef __MRF_BUFF_INCLUDED__
#define __MRF_BUFF_INCLUDED__

//#include "xbus.h"
//#include "xb_sys.h"
#include <mrf_if.h>


#include "mrf_sys_structs.h"



// emergency buffer - each I/F should have one so if case buffer
// allocation fails a retry can be sent to the originator
// only the header part of the message and the link tokens ([LQI|CSUMOK, RSSI] )

typedef struct __attribute__ ((packed)){
  uint8 len;
  uint8 hdr[sizeof(MRF_PKT_HDR)];
  uint8 tok[2];
} EBUFF;



uint8 *mrf_alloc_if(I_F i_f);
void mrf_free(uint8* buff);
void mrf_buff_loaded_if(I_F owner, uint8 *buff,uint8 em);

uint8 *_mrf_buff_ptr(uint8 bind);

//extern static uint8 _mrf_buff[_MRF_BUFFS][_MRF_BUFFLEN];
#endif
