#ifndef __DEVICE_INCLUDED__
#define __DEVICE_INCLUDED__


// device type usbrf - one usb and one rf interface

typedef  enum { UART0,
     	        RF0,
                NUM_INTERFACES} I_F;

// 8 buffs for allocation by sys
#define _MRF_BUFFS 8

#endif
