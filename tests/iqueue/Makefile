MRFHOME=../..
ARCH=lnx
#ARCH=cc430  this won't work because wrong gcc and ld above - see dir usbrf etc for working makefile
DEVTYPE=hostsim
MRFID=0x00
MRFNET=0x25

MSRC=${MRFHOME}/mrflib/src

MOBJ=./obj
MRFMAKE=.
ARCHDIR=${MRFHOME}/arch/${ARCH}
include ${ARCHDIR}/Makefile.inc
DEVDIR=${ARCHDIR}/devices/${DEVTYPE}


CCFLAGS=-I${MRFHOME}/mrflib/src -I${ARCHDIR}  -I${DEVDIR}  -I${NET} ${APPINCL}  -DMRFID=${MRFID} -DMRFNET=${MRFNET} -DDEVTYPE=${DEVTYPE} -DARCH=${ARCH} -DMRF_ARCH_${ARCH} -DGITSH=${GITSH}  -DGITMOD=${GITMOD}  -DMRFPATH=${MRFPATH} -DMRFBLD=${BUILD} ${ACCFLAGS}  ${DEVCCFLAGS} ${BFLAGS}

TARG=test_01

ALL : MOBJ TARG


MOBJ : 
	mkdir -p ${MOBJ}

MOBJS=${MOBJ}/iqueue.o 

${MOBJ}/iqueue.o : ${MRFHOME}/mrflib/src/iqueue.c ${MRFHOME}/mrflib/src/iqueue.h
	${GCC}  -c $(CCFLAGS) $< -o $@



TARG: ${MOBJS}
	${GCC} ${CCFLAGS} ${MRFMAKE}/test_01.c ${MOBJS} ${ALDFLAGS} -o ${TARG}


clean:
	rm -rf ${TARG}
	rm -rf ${MOBJ}
