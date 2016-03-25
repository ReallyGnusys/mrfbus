
TMP=/tmp/mrf_bus
LOGDIR=${TMP}/log

tests: ${TMP} ${LOGDIR} BUILD BUILDCC


${TMP}:
	mkdir ${TMP}

${LOGDIR}:
	mkdir -p ${LOGDIR}

BUILD:
	cd examples/hostsim && make clean && make MRFID=0x01 && make MRFID=0x02 && make MRFID=0x20 && make MRFID=0x2f
	cd examples/stub && make clean && make


BUILDCC:
	cd examples/usbrf && make
	cd examples/pt1000usb && make
