
TMP=/tmp/mrf_bus
LOGDIR=${TMP}/log
HOSTFIFO=${TMP}/1-0-in
STUBFIFO=${TMP}/0-app-in
APPFIFO=${TMP}/0-app-str

tests: ${TMP} FIFOS ${LOGDIR} BUILD BUILDCC QTEST

venv :	land/requirements.txt
	./build_venv


${TMP}:
	mkdir ${TMP}

FIFOS:
	rm -f ${HOSTFIFO} && mkfifo -m 600 ${HOSTFIFO}
	rm -f ${STUBFIFO} && mkfifo -m 600 ${STUBFIFO}
	rm -f ${APPFIFO} && mkfifo -m 600 ${APPFIFO}

${LOGDIR}:
	mkdir -p ${LOGDIR}

BUILD:
	cd examples/hoststub && make clean && make MRFID=0x01
	cd examples/hostsim && make clean
	cd examples/hostsim && make MRFID=0x02
	cd examples/hostsim && make MRFID=0x20
	cd examples/hostsim && make MRFID=0x2f
	cd examples/printstub && make clean && make


BUILDCC:
	cd examples/usbrf && make clean && make
	cd examples/pt1000usb && make clean && make

QTEST:
	cd mrflib/tests/iqueue && make clean && make
