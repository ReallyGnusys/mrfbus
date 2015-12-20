
#include "mrf_if.h"
#include "device.h"
static IF_STATUS _if_status[NUM_INTERFACES];
static MRF_PKT_HDR _if_ackbuffs[NUM_INTERFACES];


extern const MRF_IF_TYPE mrf_uart_cc_if;
extern const MRF_IF_TYPE mrf_rf_cc_if;

const MRF_IF _sys_ifs[] = {
 [UART0] = { &_if_status[0], &mrf_uart_cc_if, &_if_ackbuffs[0]},
 [RF0]   = { &_if_status[1], &mrf_rf_cc_if  , &_if_ackbuffs[1]}
};

