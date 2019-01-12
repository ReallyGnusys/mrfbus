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
	BFLAGS = -g -g3 -Wall
else
	BFLAGS = -g -g3 -O1 -Wall
endif

APP ?= default

## sleep opts deep or light -  light sleep keeps RF reciever on - deep sleep should ultimately shut off all clocks except RTC ACLK
SLEEP ?= light

MSRC=${MRFBUS_HOME}/mrflib/src

#BLDDIR=${MRFMAKE}build

MOBJ=${MRFMAKE}obj
LSTD=${MRFMAKE}lst

BIN=${MRFMAKE}bin
#APPSRC=${MRFMAKE}src
APPDIR=${MRFBUS_HOME}/apps/${APP}
APPSRC=${APPDIR}

TARG=${BIN}/${APP}_${MRFID}
LST=${LSTD}/${APP}_${MRFID}.lst
SIZEREP=${LSTD}/${APP}_${MRFID}.size

NET=${MRFMAKE}../net
ARCHDIR=${MRFBUS_HOME}/arch/${ARCH}
DEVDIR=${ARCHDIR}/devices/${DEVTYPE}

include ${ARCHDIR}/Makefile.inc
include ${DEVDIR}/Makefile.inc

APPMAKEF=${APPDIR}/Makefile.inc

ifneq ($(wildcard ${APPDIR}/Makefile.inc),)
include ${APPMAKEF}
endif

#include ${APPDIR}/Makefile*.inc


APP_CMDS=${APPSRC}/mrf_app_cmds.c

APPINCL=-I${APPSRC}



MOBJS=${MOBJ}/mrf.o ${MOBJ}/mrf_buff.o ${MOBJ}/mrf_sys.o ${MOBJ}/mrf_sys_tasks.o \
	${MOBJ}/mrf_route.o ${MOBJ}/mrf_if.o \
	${MOBJ}/mrf_uart.o ${MOBJ}/mrf_sys_cmds.o ${MOBJ}/mrf_arch.o \
	${MOBJ}/device.o  ${MOBJ}/mrf_app.o ${MOBJ}/mrf_app_cmds.o ${AOBJS} ${DEVOBJS}  ${APPOBJS} \
        ${MOBJ}/main.o

ifeq (,$(wildcard ${APPSRC}/main.c))
     MAIN := ${MSRC}/main.c
else
     MAIN := ${APPSRC}/main.c
endif


DEBUG_LEVEL?=1


CCFLAGS=-I${MRFBUS_HOME}/mrflib/src -I${ARCHDIR}  -I${DEVDIR}  -I${NET} ${APPINCL} -I../ -DMRFID=${MRFID} -DMRFNET=${MRFNET} -DDEVTYPE=${DEVTYPE} -DARCH=${ARCH} -DMRF_ARCH_${ARCH} -DMRF_APP=${APP} -DMRF_APP_${APP} -DGITSH=${GITSH}  -DGITMOD=${GITMOD}  -DMRFPATH=${MRFPATH} -DMRFPROJ=${MRFPROJ} -DSLEEP_${SLEEP} -DMRFBLD=${BUILD} ${ACCFLAGS} -DDEBUG_LEVEL=${DEBUG_LEVEL}  ${DEVCCFLAGS} ${APPCCFLAGS} ${BFLAGS}


ALL : ${BIN} ${MOBJ} ${LSTD} ${MOBJS} ${APPOBJS} ${TARG} ${LST} ${SIZEREP}

clean :
	rm -rf ${MOBJ} ${LSTD} ${BIN}

${BIN}:
	mkdir -p ${BIN}

${MOBJ}:
	echo mkdir -p ${MOBJ}
	mkdir -p ${MOBJ}

main.cpp.out: ${MAIN}
	cpp ${CCFLAGS} ${MAIN} > main.cpp.out

${LSTD}:
	mkdir -p ${LSTD}

${TARG} : ${MOBJS}
	${LD} ${CCFLAGS} ${MOBJS} ${ALDFLAGS} -o ${TARG}

${LST}: ${TARG}
	${OD} ${TARG} > ${LST}

${SIZEREP}: ${TARG}
	${SIZE} ${TARG} > ${SIZEREP}
	cat ${SIZEREP}

${MOBJ}/main.o: ${MAIN}
	${GCC}  -c $(CCFLAGS) ${MAIN}   -o ${MOBJ}/main.o

${MOBJ}/mrf_kdata.o: ../mrf_kdata.c
	${GCC}  -c $(CCFLAGS) ../mrf_kdata.c   -o ${MOBJ}/mrf_kdata.o

${MOBJ}/%.o: ${APPDIR}/%.c
	${GCC}  -c $(CCFLAGS) $< -o $@


${MOBJ}/%.o: ${MSRC}/%.c
	echo arg $@ && echo mobj ${MOBJ}  && ${GCC}  -c $(CCFLAGS) $<  -o $@


${MOBJ}/%.o: ${NET}/%.c
	${GCC}  -c $(CCFLAGS) $< -o $@

${MOBJ}/%.o: ${ARCHDIR}/%.c
	${GCC}  -c $(CCFLAGS) $< -o $@

${MOBJ}/%.o: ${DEVDIR}/%.c
	$(GCC)  -c $(CCFLAGS) $< -o $@
