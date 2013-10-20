#ifndef __XB_IF_INCLUDED__
#define __XB_IF_INCLUDED__
#include "g430_types.h"

// this file varies from device to device
// it describes the interfaces available to the device and
// provides a uniform api for the os 

// interface drivers provide uniform interface for this file to use

// this device uses mote uart and mote rf
//#include "uart.h"



typedef  enum { RF1,
     	        UART1,
		NONE,
		OS,
     	        APP} I_F;

//


typedef struct  __attribute__ ((packed)){
  uint16 over;  // note packets that are too long or short to be valid
  uint16 under;
  uint16 crc_err;
  uint16 rx_pkts;
  uint16 tx_pkts;
} XB_IF_STATS;

#endif
