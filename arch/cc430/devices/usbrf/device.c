
#include "mrf_if.h"

static IF_STATUS _uart0_status, rf0_status;

static MRF_PKT_HDR _uart0_ackbuff, _rf0_ackbuff;

extern const MRF_IF_TYPE mrf_uart_cc_if;
extern const MRF_IF_TYPE mrf_rf_cc_if;

const MRF_IF sys_ifs[] = {
 UART0 : { &_uart0_status, &mrf_uart_cc_if, &_uart0_ackbuff},
 RF0   : { &rf0_status, mrf_rf_cc_if, &_rf0_ackbuff)
};

