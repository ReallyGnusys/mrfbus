#include "mrf_route.h"
#include "stdio.h"
#include "stdlib.h"

void troute(uint8 src,uint8 dest){
  uint8 hop,nhop,lim  = 0;
  MRF_ROUTE route;
  hop = src;
  printf("Route from %X to %X\n",src,dest);
  while (hop != dest){
    nhop = mrf_nexthop(&route,hop,dest);
    printf("hop %X to %X ( numifs %d ) R.relay %X R.i_f %d\n",hop,nhop,num_ifs(hop),route.relay,route.i_f);
    
    hop = nhop;

    if(lim++ > 10)
      exit(1);
    
  }
}
        

void main(){
  troute(0x1,0x2f);
  troute(0x1,0x2);

  troute(0x1,0x4);

  troute(0x2f,0x1);
  
  troute(0x4f,0x2f);
  troute(0x4,0x2f);

  troute(0x2,0x8F);
  troute(0x2,0xFE);
  troute(0xFE,0x0);
 troute(0x0,0xFE);



}
