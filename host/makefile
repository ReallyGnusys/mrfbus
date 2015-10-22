GCC=gcc
LD=ld
ARCH=x86_64
#ARCH=cc430
DEVTYPE=host
MRFID=0x01
MRFNET=0x25
GITSH=\""$(git rev-parse HEAD)"\"
BIN=bin/${ARCH}
TARG=${BIN}/${DEVTYPE}_${MRFID}
MOBJ=../mrflib/obj/${ARCH}/${DEVTYPE}
MSRC=../mrflib/src
NET=../net
ARCHDIR=../arch/${ARCH}

MOBJS=${MOBJ}/mrf.o ${MOBJ}/mrf_buff.o ${MOBJ}/mrf_sys.o ${MOBJ}/mrf_sys_tasks.o ${MOBJ}/mrf_route.o ${MOBJ}/mrf_if.o ${MOBJ}/iqueue.o \
	${MOBJ}/mrf_cmds.o ${MOBJ}/mrf_arch.o

ALL : ${BIN} ${OBJ} ${MOBJS} ${TARG}



#MBDEVNAME=\""HOST_ROUTER"\"




clean : 
	rm -rf ${MOBJ}
	rm -rf ${BIN}
	rm -rf ${TARG}

${BIN}:
	mkdir -p ${BIN}






CCFLAGS=-DXBDEVNAME=${XBDEVNAME} -DSVN=${SVN} -I../mrflib/src  -I../arch/${ARCH}  -I../devices/${DEVTYPE} -I../net  -DMRFID=${MRFID} -DMRFNET=${MRFNET} -DDEVTYPE=${DEVTYPE} -DARCH=${ARCH} -DGITSH=${GITSH}



${TARG} :
	${GCC} ${CCFLAGS}  ttest.c ${MOBJS}  -lrt -o ${TARG}

${MOBJ}/%.o: ${MSRC}/%.c
	mkdir -p ${MOBJ}
	gcc  -c $(CCFLAGS) $< -o $@


${MOBJ}/%.o: ${NET}/%.c
	mkdir -p ${MOBJ}
	gcc  -c $(CCFLAGS) $< -o $@

${MOBJ}/%.o: ${ARCHDIR}/%.c
	mkdir -p ${MOBJ}
	gcc  -c $(CCFLAGS) $< -o $@
