GCC=msp430-gcc
LD=msp430-ld

AOBJS=${MOBJ}/mrf_rf.o  ${MOBJ}/_mrf_rf1.o ${MOBJ}/clock.o ${MOBJ}/hal_pmm.o ${MOBJ}/RfRegSettings.o \
	${MOBJ}/mrf_uart_cc.o  ${MOBJ}/rtc.o

ACCFLAGS= -mmcu=cc430f5137 -DMHZ_868
