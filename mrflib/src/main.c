#include <mrf.h>

const uint8 _mrfid = MRFID;

int mrf_spi_init_cc()  __attribute__ ((constructor));

int main(void){
  int i;
  mrf_init();  
}

