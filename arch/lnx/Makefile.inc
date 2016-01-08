GCC=gcc
LD=ld
AOBJS=${MOBJ}/mrf_pipe_lnx.o ${MOBJ}/mrf_uart_lnx.o
ALDFLAGS=-lpthread -lrt