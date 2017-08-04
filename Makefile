
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

CLEAN:
	rm -rf examples/hostsim/\




BUILD:
	cd examples/hostsim && make CLEANALL
	cd examples/hostsim && make MRFID=0x01
	cd examples/hostsim && make MRFID=0x02
	cd examples/hostsim && make MRFID=0x20
	cd examples/hostsim && make MRFID=0x2f
	cd examples/host && make CLEANALL
	cd examples/host && make MRFID=0x01



BUILDCC:
	cd examples/usbrf && make CLEANALL && make
	cd examples/pt1000usb && make CLEANALL && make

QTEST:
	cd mrflib/tests/iqueue && make clean && make
