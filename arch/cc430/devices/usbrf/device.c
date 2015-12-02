
#include "mrf_if.h"

//#include "mrf_uart.h"
extern MRF_IF_TYPE rf_if_type;
extern MRF_IF_TYPE usb_if_type;
int mrf_device_init(){
  mrf_if_register(UART0,&rf_if_type);


}
