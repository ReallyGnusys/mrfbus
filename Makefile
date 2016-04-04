
TMP=/tmp/mrf_bus
LOGDIR=${TMP}/log

tests: ${TMP} ${LOGDIR} BUILD BUILDCC


${TMP}:
	mkdir ${TMP}

${LOGDIR}:
	mkdir -p ${LOGDIR}

BUILD:
	cd examples/hostsim && make clean
	cd examples/hostsim && make MRFID=0x00
	cd examples/hostsim && make MRFID=0x01
	cd examples/hostsim && make MRFID=0x02
	cd examples/hostsim && make MRFID=0x20
	cd examples/hostsim && make MRFID=0x2f
	cd examples/host && make clean && make
	cd examples/stub && make clean && make
	cd examples/printstub && make clean && make


BUILDCC:
	cd examples/usbrf && make clean && make
	cd examples/pt1000usb && make clean && make
