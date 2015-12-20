#include "mrf_if.h"
#include "device.h"
static IF_STATUS _if_status[NUM_INTERFACES];
static MRF_PKT_HDR _if_ackbuffs[NUM_INTERFACES];

extern const MRF_IF_TYPE mrf_uart_lnx_if;

extern const MRF_IF_TYPE mrf_pipe_lnx_if;

static ifd[NUM_INTERFACES]; // need fds for lnx i_f s

const MRF_IF _sys_ifs[] = {
 [ USB0 ]  =  { &_if_status[0],  &mrf_uart_lnx_if, &_if_ackbuffs[0], &ifd[0]},
 [ PIPE1 ] =  { &_if_status[1],  &mrf_pipe_lnx_if, &_if_ackbuffs[1], &ifd[1]},
 [ PIPE2 ] =  { &_if_status[2], &mrf_pipe_lnx_if, &_if_ackbuffs[2], &ifd[2]},
 [ PIPE3 ] =  { &_if_status[3], &mrf_pipe_lnx_if, &_if_ackbuffs[3], &ifd[3]}
};
