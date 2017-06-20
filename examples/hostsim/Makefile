MRFHOME=../..
ARCH=lnx
#ARCH=cc430  this won't work because wrong gcc and ld above - see dir usbrf etc for working makefile
DEVTYPE=hostsim
MRFID ?= 0x01
MRFNET=0x25
ifeq ($(MRFID),0x01)
  APP=host
  APPCCFLAGS=-DHOST_STUB
else
  APP=default
endif

include ${MRFHOME}/Makefile.inc

