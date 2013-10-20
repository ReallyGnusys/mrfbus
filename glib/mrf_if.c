/* mrf bus interface */

// allocate interface structures
#include <mrf_if.h>
MRF_IF _sys_ifs[NUM_INTERFACES];

MRF_IF *mrf_if_ptr(I_F i_f){
  return &_sys_ifs[i_f];
}
void mrf_if_init(){
  int i,j;
  uint8 *dptr;
  for (i = 0 ; i < NUM_INTERFACES ; i++){
    // rough zeroing of data
    dptr = (uint8 *)&_sys_ifs[i];
    for ( j = 0 ; j < sizeof(IF_STATUS) ; j++)
      dptr[j] = 0;    
  }

}

void mrf_if_register(I_F i_f, MRF_IF_TYPE *type){
  _sys_ifs[i_f].type = type;
}
