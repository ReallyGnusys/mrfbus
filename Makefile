
TMP=/tmp/mrf_bus
LOGDIR=${TMP}/log
HOSTFIFO=${TMP}/1-0-in

tests: ${TMP} ${HOSTFIFO} ${LOGDIR} BUILD BUILDCC QTEST

venv :	land/requirements.txt
	cd land && virtualenv venv
	bash && source land/venv/bin/activate && pip install -r land/requirements.txt && exit


${TMP}:
	mkdir ${TMP}

${HOSTFIFO}:
	mkfifo -m 600 ${HOSTFIFO}

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

QTEST:
	cd mrflib/tests/iqueue && make clean && make
