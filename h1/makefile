XBDEVNAME=\""USBHUB1"\"
SVN:=\""$(echo GITNOSTATUS)"\"

OBJ=obj
COBJ=../obj
NET=../net
SRC=src


GLIB=../glib
GCC=msp430-gcc
LD=msp430-ld
#hub must be ID 01
XBID=0x02
XBNET=25
CCFLAGS=-O0 -I${SRC} -I${GLIB} -I${NET}  -DXB_ID=${XBID}  -DXB_NET=${XBNET} -DMHZ_868 -DSPI_UEXT -Wfatal-errors -mmcu=cc430f5137 -g -DMCLK_8MHZ_DCO -DXB_UART -DXB_HUB  -DXBDEVNAME=${XBDEVNAME} -DSVN=${SVN}
LD=msp430-ld														
OD=msp430-objdump -dt

LIBS=${GLIB}/LCD1x9.c ${GLIB}/hal_pmm.c ${GLIB}/RF1A.c  \
	${NET}/RfRegSettings.c  ${GLIB}/rtc.c ${GLIB}/xbus.c\
	${GLIB}/xb_hw.c ${GLIB}/xb_buff.c ${GLIB}/xbus_route.c\
	${GLIB}/squeue.c ${GLIB}/clock.c ${GLIB}/uart.c\
	${GLIB}/glib.c ${GLIB}/xqueues.c ${GLIB}/adc.c

SOURCES=${SRC}/main.c ${SRC}/xb_routes.c 

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

