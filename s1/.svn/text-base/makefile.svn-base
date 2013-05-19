OBJ=./obj
SRC=./src
GLIB=../glib
GCC=msp430-gcc
LD=msp430-ld
#hub must be ID 01
XBID=0x01
XBNET=25
CCFLAGS=-O0 -I${SRC} -I${GLIB} -DXB_ID=${XBID}  -DXB_NET=${XBNET} -DMHZ_868 -DSPI_UEXT -Wfatal-errors -mmcu=cc430f5137 -g -DXB_MODE_SNIFFER -DMCLK_8MHZ_DCO
LD=msp430-ld
OD=msp430-objdump -dt

LIBS=${GLIB}/spi.c ${GLIB}/LCD1x9.c ${GLIB}/hal_pmm.c ${GLIB}/RF1A.c  \
	${GLIB}/rtc.c ${GLIB}/xbus.c ${GLIB}/clock.c \
	${GLIB}/modtc.c ${GLIB}/uart.c ${GLIB}/squeue.c ${GLIB}/glib.c

SOURCES=${SRC}/main.c ${SRC}/RfRegSettings.c

#SOURCES=${SRC}/main.c ${SRC}/RF1A.c ${SRC}/hal_pmm.c ${SRC}/RfRegSettings.c \
#	${SRC}/spi.c ${SRC}/LCD1x9.c




BIN=bin
LST=lst
ALL: BIN LST

CLEAN:
	rm  ${BIN} ${LST}

BIN: ${SOURCES} ${LIBS}
	${GCC} -Os ${CCFLAGS}    ${SOURCES} ${LIBS} -o ${BIN}

#BIN: ${SOURCES} ${LIBS}
#	${GCC} -Os ${CCFLAGS}    ${SOURCES}  ${LIBS} -o ${BIN}

#BIN: ${OBJS}
#	${GCC} ${CCFLAGS}   --Wfatal-errors  ${OBJS}  -o ${BIN}

LST: ${BIN}
	${OD} ${BIN} > ${LST}

