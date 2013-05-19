XBDEVNAME=\""METEO1"\"
SVN:=\""$(shell svnversion -n .)"\"


OBJ=obj
COBJ=../obj
NET=../net
SRC=src
XBDEVNAME=\""METEO1"\"
SVN:=\""$(shell svnversion -n .)"\"

GLIB=../glib
GCC=msp430-gcc
LD=msp430-ld
XBID=0x11
XBNET=25
CFLAGS=-O0 -I${SRC} -I${GLIB} -I${NET} -DXB_ID=${XBID}  -DXB_NET=${XBNET} -DMHZ_868 -DSPI_UEXT -Wfatal-errors -mmcu=cc430f5137 -g -DMCLK_4MHZ_DCO -DXBDEVNAME=${XBDEVNAME} -DSVN=${SVN}

CCFLAGS=-c ${CFLAGS}

LD=msp430-ld
OD=msp430-objdump -dt


COBJS=${COBJ}/spi.o ${COBJ}/LCD1x9.o ${COBJ}/hal_pmm.o ${COBJ}/RF1A.o  \
	${COBJ}/rtc.o  ${COBJ}/RfRegSettings.o\
	${COBJ}/modtc.o  ${COBJ}/timera.o ${COBJ}/adc.o\
	${COBJ}/squeue.o  ${COBJ}/glib.o ${COBJ}/xqueues.o ${COBJ}/i2c.o\
	${COBJ}/sht21.o ${COBJ}/mpl115a2.o ${COBJ}/xb_sys_tasks.o \
	${COBJ}/xb_hw.o
#	${COBJ}/modtc.o  ${COBJ}/timera.o ${COBJ}/clock.o


OBJS=${OBJ}/main.o ${OBJ}/xbus.o ${OBJ}/clock.o 



BIN=bin
LST=lst

ALL: ${OBJ} ${COBJ} BIN LST 

CLEAN:
	rm  ${BIN} ${LST}  ${OBJS} ${COBJS} ${LIBOBJS}

${OBJ}:
	mkdir -p ${OBJ}

${COBJ}:
	mkdir -p ${COBJ}




BIN: ${OBJS} ${COBJS}
	${GCC} -Os ${CFLAGS} ${OBJS}  ${COBJS} -o ${BIN}


LST: ${BIN}
	${OD} ${BIN} > ${LST}


${OBJ}/main.o :	${SRC}/main.c
	 ${GCC} ${CCFLAGS} ${SRC}/main.c -o ${OBJ}/main.o

${COBJ}/spi.o :	${GLIB}/spi.c
	${GCC} ${CCFLAGS} ${GLIB}/spi.c -o ${COBJ}/spi.o

${COBJ}/LCD1x9.o :	${GLIB}/LCD1x9.c
	${GCC} ${CCFLAGS} ${GLIB}/LCD1x9.c -o ${COBJ}/LCD1x9.o

${COBJ}/hal_pmm.o :	${GLIB}/hal_pmm.c
	${GCC} ${CCFLAGS} ${GLIB}/hal_pmm.c -o ${COBJ}/hal_pmm.o
${COBJ}/RF1A.o :	${GLIB}/RF1A.c
	${GCC} ${CCFLAGS} ${GLIB}/RF1A.c -o ${COBJ}/RF1A.o

${COBJ}/RfRegSettings.o :	${NET}/RfRegSettings.c
	${GCC} ${CCFLAGS} ${NET}/RfRegSettings.c -o ${COBJ}/RfRegSettings.o

${COBJ}/rtc.o :	${GLIB}/rtc.c ${GLIB}/rtc.h
	${GCC} ${CCFLAGS} ${GLIB}/rtc.c -o ${COBJ}/rtc.o

${OBJ}/xbus.o :	${GLIB}/xbus.c 	${GLIB}/xbus.h $(wildcard ${NET}/xbus*.h) ${GLIB}/rtc.h
	${GCC} ${CCFLAGS} ${GLIB}/xbus.c -o ${OBJ}/xbus.o

${COBJ}/modtc.o :	${GLIB}/modtc.c
	${GCC} ${CCFLAGS} ${GLIB}/modtc.c -o ${COBJ}/modtc.o
${COBJ}/timera.o :	${GLIB}/timera.c
	${GCC} ${CCFLAGS} ${GLIB}/timera.c -o ${COBJ}/timera.o
${OBJ}/clock.o :	${GLIB}/clock.c
	${GCC} ${CCFLAGS} ${GLIB}/clock.c -o ${OBJ}/clock.o

${COBJ}/adc.o :	${GLIB}/adc.c
	${GCC} ${CCFLAGS} ${GLIB}/adc.c -o ${COBJ}/adc.o

${COBJ}/squeue.o :	${GLIB}/squeue.c ${GLIB}/squeue.h
	${GCC} ${CCFLAGS} ${GLIB}/squeue.c -o ${COBJ}/squeue.o

${COBJ}/glib.o :	${GLIB}/glib.c ${GLIB}/glib.h
	${GCC} ${CCFLAGS} ${GLIB}/glib.c -o ${COBJ}/glib.o

${COBJ}/xqueues.o :	${GLIB}/xqueues.c ${GLIB}/xqueues.h
	${GCC} ${CCFLAGS} ${GLIB}/xqueues.c -o ${COBJ}/xqueues.o

${COBJ}/i2c.o :	${GLIB}/i2c.c ${GLIB}/i2c.h
	${GCC} ${CCFLAGS} ${GLIB}/i2c.c -o ${COBJ}/i2c.o

${COBJ}/sht21.o :	${GLIB}/sht21.c ${GLIB}/sht21.h
	${GCC} ${CCFLAGS} ${GLIB}/sht21.c -o ${COBJ}/sht21.o

${COBJ}/mpl115a2.o :	${GLIB}/mpl115a2.c ${GLIB}/mpl115a2.h
	${GCC} ${CCFLAGS} ${GLIB}/mpl115a2.c -o ${COBJ}/mpl115a2.o


${COBJ}/xb_sys_tasks.o :	${GLIB}/xb_sys_tasks.c ${GLIB}/xb_sys_tasks.h
	${GCC} ${CCFLAGS} ${GLIB}/xb_sys_tasks.c -o ${COBJ}/xb_sys_tasks.o

${COBJ}/xb_hw.o :	${GLIB}/xb_hw.c ${GLIB}/xb_hw.h
	${GCC} ${CCFLAGS} ${GLIB}/xb_hw.c -o ${COBJ}/xb_hw.o
