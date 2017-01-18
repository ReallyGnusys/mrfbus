###############################################################################
# Copyright (C) 2012-2016 Gnusys Ltd
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

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
LSTD=${MRFMAKE}lst

BIN=${MRFMAKE}bin
#APPSRC=${MRFMAKE}src
APPSRC=${MRFHOME}/apps/${APP}

TARG=${BIN}/${DEVTYPE}_${MRFID}
NET=${MRFMAKE}../net
ARCHDIR=${MRFHOME}/arch/${ARCH}
DEVDIR=${ARCHDIR}/devices/${DEVTYPE}

include ${ARCHDIR}/Makefile.inc
include ${DEVDIR}/Makefile.inc


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


ALL : ${BIN} ${MOBJ} ${MOBJS} ${TARG}

clean : 
	rm -rf ${MOBJ}
	rm -rf ${BIN}
	rm -rf ${TARG}

${BIN}:
	mkdir -p ${BIN}

${MOBJ}:
	mkdir -p ${MOBJ}

${TARG} :
	${GCC} ${CCFLAGS} ${MAIN} ${MOBJS} ${ALDFLAGS} -o ${TARG}

${MOBJ}/mrf_app.o: ${APPSRC}/mrf_app.c
	${GCC}  -c $(CCFLAGS) ${APPSRC}/mrf_app.c -o $@


${MOBJ}/mrf_app_cmds.o: ${APP_CMDS}
	${GCC}  -c $(CCFLAGS) ${APP_CMDS} -o $@




${MOBJ}/%.o: ${MSRC}/%.c
	${GCC}  -c $(CCFLAGS) $< -o $@


${MOBJ}/%.o: ${NET}/%.c
	${GCC}  -c $(CCFLAGS) $< -o $@

${MOBJ}/%.o: ${ARCHDIR}/%.c
	${GCC}  -c $(CCFLAGS) $< -o $@

${MOBJ}/%.o: ${DEVDIR}/%.c
	$(GCC)  -c $(CCFLAGS) $< -o $@


