#ifndef __MRF_ROUTE_INCLUDED__
#define __MRF_ROUTE_INCLUDED__

#include "mrf_sys.h"
#define SNETSZ  0x20

#ifndef MRFID

//error "MRFID must be defined e.g. use -DMRFID=id in gcc args"
#define MRFID 1
#endif

#define NUM_IFS MRFID == 1 ?  (256/SNETSZ)-1 : (MRFID / SNETSZ) ==0 ? 2 :1 

typedef struct {
  I_F  i_f;
  uint8 relay;  
} MRF_ROUTE;

uint8 mrf_nexthop(MRF_ROUTE *route,uint8 us,uint8 dest);
uint8 num_ifs(uint8 addr);

#endif
