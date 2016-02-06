MRFPATH=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
MRFMAKE=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
#$(shell pwd)
GITSH=$(shell cd ${MRFPATH}; git rev-parse HEAD)

ifeq ($(shell  cd ${MRFPATH}; git status | grep '^\s*modified'),)
	GITMOD=0
else
	GITMOD=1
endif

# BUILD is debug or prod
BUILD ?= debug

ifeq ($(BUILD),debug)
	BFLAGS = -g
else
	BFLAGS = -O2
endif

APP ?= default


MSRC=${MRFHOME}/mrflib/src

MOBJ=${MRFMAKE}obj

BIN=${MRFMAKE}bin
#APPSRC=${MRFMAKE}src
APPSRC=${MRFHOME}/apps/${APP}

TARG=${BIN}/${DEVTYPE}_${MRFID}
NET=${MRFMAKE}../net
ARCHDIR=${MRFHOME}/arch/${ARCH}
DEVDIR=${ARCHDIR}/devices/${DEVTYPE}

include ${ARCHDIR}/Makefile.inc
include ${DEVDIR}/Makefile.inc



#ifeq (,$(wildcard ${APPSRC}/mrf_app_tasks.c))
#     APP_TASKS=${MSRC}/mrf_app_tasks.c
#else
#     APP_TASKS=${APPSRC}/mrf_app_tasks.c
#endif      

APP_CMDS=${APPSRC}/mrf_app_cmds.c
APPINCL=-I${APPSRC}



MOBJS=${MOBJ}/mrf.o ${MOBJ}/mrf_buff.o ${MOBJ}/mrf_sys.o ${MOBJ}/mrf_sys_tasks.o \
	${MOBJ}/mrf_route.o ${MOBJ}/mrf_if.o ${MOBJ}/iqueue.o \
	${MOBJ}/mrf_uart.o ${MOBJ}/mrf_sys_cmds.o ${MOBJ}/mrf_arch.o \
	${MOBJ}/device.o  ${MOBJ}/mrf_app.o ${MOBJ}/mrf_app_cmds.o ${AOBJS} ${DEVOBJS}

ifeq (,$(wildcard ${APPSRC}/main.c))
     MAIN := ${MSRC}/main.c
else
     MAIN := ${APPSRC}/main.c
endif      

CCFLAGS=-I${MRFHOME}/mrflib/src -I${ARCHDIR}  -I${DEVDIR}  -I${NET} ${APPINCL}  -DMRFID=${MRFID} -DMRFNET=${MRFNET} -DDEVTYPE=${DEVTYPE} -DARCH=${ARCH} -DMRF_ARCH_${ARCH} -DGITSH=${GITSH}  -DGITMOD=${GITMOD}  -DMRFPATH=${MRFPATH} -DMRFBLD=${BUILD} ${ACCFLAGS}  ${DEVCCFLAGS} ${BFLAGS}


ALL : ${BIN} ${OBJ} ${MOBJS} ${TARG}

clean : 
	rm -rf ${MOBJ}
	rm -rf ${BIN}
	rm -rf ${TARG}

${BIN}:
	mkdir -p ${BIN}

${TARG} :
	${GCC} ${CCFLAGS} ${MAIN} ${MOBJS} ${ALDFLAGS} -o ${TARG}

${MOBJ}/mrf_app.o: ${APPSRC}/mrf_app.c
	mkdir -p ${MOBJ}
	${GCC}  -c $(CCFLAGS) ${APPSRC}/mrf_app.c -o $@


${MOBJ}/mrf_app_cmds.o: ${APP_CMDS}
	mkdir -p ${MOBJ}
	${GCC}  -c $(CCFLAGS) ${APP_CMDS} -o $@




${MOBJ}/%.o: ${MSRC}/%.c
	mkdir -p ${MOBJ}
	${GCC}  -c $(CCFLAGS) $< -o $@


${MOBJ}/%.o: ${NET}/%.c
	mkdir -p ${MOBJ}
	${GCC}  -c $(CCFLAGS) $< -o $@

${MOBJ}/%.o: ${ARCHDIR}/%.c
	mkdir -p ${MOBJ}
	${GCC}  -c $(CCFLAGS) $< -o $@

${MOBJ}/%.o: ${DEVDIR}/%.c
	mkdir -p ${MOBJ}
	$(GCC)  -c $(CCFLAGS) $< -o $@


