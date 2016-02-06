GCC=msp430-gcc
LD=msp430-ld

AOBJS=${MOBJ}/mrf_rf.o  ${MOBJ}/_mrf_rf1.o ${MOBJ}/clock.o ${MOBJ}/hal_pmm.o ${MOBJ}/RfRegSettings.o \
	${MOBJ}/mrf_uart_cc.o  ${MOBJ}/rtc.o

ifdef MRF_SPI_B
      ${MOBJ}/mrf_spi_cc.o
endif


ACCFLAGS= -mmcu=cc430f5137  -DMHZ_868 

# -g
