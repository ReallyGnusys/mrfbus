ALL : xb_host_uart intest insel timer polltest mtest
GCC=gcc
LD=ld
ARCH=x86_64
DEVTYPE=host
MRFID=0x01
MRFNET=0x25
GITSH=\""$(git rev-parse HEAD)"\"
#MBDEVNAME=\""HOST_ROUTER"\"

clean : 
	rm mtest

SRCS=../glib/mrf.c ../arch/${ARCH}/mrf_arch.c ../glib/mrf_buff.c ../glib/mrf_sys.c ../glib/mrf_sys_tasks.c ../glib/mrf_if.c  ../glib/mrf_route.c  ../net/mrf_cmds.c ../glib/iqueue.c
CCFLAGS=-DXBDEVNAME=${XBDEVNAME} -DSVN=${SVN} -I../glib  -I../arch/${ARCH}  -I../devices/${DEVTYPE} -I../net  -DMRFID=${MRFID} -DMRFNET=${MRFNET} -DDEVTYPE=${DEVTYPE} -DARCH=${ARCH}

intest : intest.c ../glib/xb_buff.c
	${GCC} ${CCFLAGS}  -I../glib -I../arch/i686 intest.c ../glib/xb_buff.c -lrt -o intest

mtest: mtest.c ${SRCS}
	${GCC} ${CCFLAGS}  mtest.c ${SRCS}  -lrt -o mtest

insel : insel.c ../glib/xb_buff.c
	${GCC} ${CCFLAGS}  -I../glib -I../arch/i686 insel.c ../glib/xb_buff.c -lrt -o insel

timer : timer.c ../glib/xb_buff.c
	${GCC} ${CCFLAGS}  -I../glib -I../arch/i686 timer.c ../glib/xb_buff.c -lrt -o timer

xb_host_uart : xb_host_uart.c
	gcc xb_host_uart.c -o xb_host_uart

polltest : polltest.c ../glib/xb_buff.c
	${GCC} ${CCFLAGS}  -I../glib -I../arch/i686 polltest.c ../glib/xb_buff.c -lrt -o polltest
