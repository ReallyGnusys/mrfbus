MRFNET=0x25
ARCH=x86_64
DEVICE=host
CFLAGS=-Isrc -I../arch/${ARCH} -I../devices/${DEVICE} -I../net -DGITSH=${GITSH} -DMRFNET=${MRFNET}
GITSH=\""$(git rev-parse HEAD)"\"
ARCHDIR=../arch/${ARCH}

SRCS := $(wildcard src/*.c)

NET=../net

OBJ=obj/${ARCH}/${DEVICE}
DEV=../devices/${DEVICE}

ALL: ${OBJ} ${OBJ}/mrf.o ${OBJS} built

${OBJ}:
	echo "hi" ; mkdir -p ${OBJ}


clean :
	rm -f ${OBJS}
	rm -rf ${OBJ}



OBJS=${OBJ}/mrf.o ${OBJ}/mrf_buff.o ${OBJ}/mrf_sys.o ${OBJ}/mrf_sys_tasks.o ${OBJ}/mrf_route.o ${OBJ}/mrf_if.o ${OBJ}/iqueue.o \
	${OBJ}/mrf_cmds.o ${OBJ}/mrf_arch.o	

built : ${OBJS}
	echo "${OBJS}" > built

#${OBJ}/mrflib.o : ${OBJS}
#	gcc -c ${OBJS} -o ${OBJ}/mrflib.o



${OBJ}/%.o: src/%.c
	gcc  -c $(CFLAGS) $(CPPFLAGS) $< -o $@

${OBJ}/mrf_cmds.o : ${NET}/mrf_cmds.c
	gcc  -c $(CFLAGS) $(CPPFLAGS) ${NET}/mrf_cmds.c  -o ${OBJ}/mrf_cmds.o

${OBJ}/mrf_arch.o : ${ARCHDIR}/mrf_arch.c
	gcc  -c $(CFLAGS) $(CPPFLAGS) ${ARCHDIR}/mrf_arch.c  -o ${OBJ}/mrf_arch.o

#${OBJ}/%.o : src/%.c
#	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o ${OBJ}/$@

${OBJ}/mrflib.o : ${OBJS}
	gcc -c  -o ${OBJ}/mrflib.o  ${CFLAGS} ${OBJS}
