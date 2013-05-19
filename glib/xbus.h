#ifndef _XBUS_INCLUDED_
#define _XBUS_INCLUDED_
#include  <msp430.h>
#include <legacymsp430.h>
#include "g430_types.h"
#include "RF1A.h"
#include "rtc.h"


#ifndef XB_ID
#error "XB_ID must be defined - use -DXB_ID=<ID> on GCC command line "
#endif

#ifndef XB_NET
#error "XB_NET must be defined - use -DXB_NET=<ID> on GCC command line "
#endif
#define XB_MAXPKTLEN 64

#include "xb_cmd_def.h"
#include "xb_sys_structs.h"

typedef int (*XBRXFUNCPTR)(XB_CMD_CODE cmd,void *);


typedef struct{
  char *name;
  uint16 size;
  XBRXFUNCPTR func;  // foreground function - executed in foreground loop
}XB_PKT;





/* xbus message codes
   32 message codes encoded in 4:0
   bits 7:5 encode ack,retry and wakeup - see below */
#define XB_PKT_CODEMASK 0x1f
/* first block of codes have time auto prepended */
/*
#define XB_PKT_SETTIME 1
#define XB_PKT_TEMP    2
#define XB_PKT_METEO1  5

// normal ( no time prepend messages ) start here
#define XB_PKT_NORMSTART      0x8

#define XB_PKT_CLRSTATS    0x8
#define XB_PKT_GETSTATS    0x9

#define XB_PKT_PING   0xa
#define XB_PKT_GETTIME 0xb
#define XB_PKT_GETTEMP 0xc
#define XB_PKT_SENDADC1   0xd  
#define XB_PKT_ADC1   0xe   

#define XB_PKT_SENDADC2 0xf   
#define XB_PKT_ADC2   0x10
#define XB_PKT_INFO   0x11   

#define XB_PKT_STAYAWAKE 0x16
#define XB_PKT_SLEEP    0x17
#define XB_PKT_METEO1_COEFFS    0x18
#define XB_PKT_DBG_UINT8    0x19  // 8 16bit uints for general debugging/use
#define XB_PKT_DBG_CHR32    0x1a  // 32 char string for general debugging/use
*/



/* response fields of type message */
/* response types contain received type code in [5:0]
   and set ACK or RETRY bits 
   XB_PKT_WAKEUP is used by hubs to modify peripheral sleep status
*/
#define XB_PKT_ACK    0x80
#define XB_PKT_RETRY    0x40
#define XB_PKT_WAKEUP    0x20

#define XB_RESPONSE  (XB_PKT_ACK | XB_PKT_RETRY)



// XB addresses
// broadcast address
#define XB_BCADDR  0xff
// destination for sensor packets
#define XB_HUBADDR  0x01  



// all structs use whole numbers of  16bit words
// i.e. can be packed on word alignments





typedef enum {
  XB_TX_IDLE,
  XB_TX_BUSY,
  XB_TX_POSTED,
  XB_TX_DELIVERED,
  XB_TX_DEFERRED,
  XB_TX_BADACK,
  XB_TX_NOACK,
  XB_TX_ABORTED,
  XB_TX_STAYAWAKE,
  XB_TX_ERRX} XB_TX_STATUS;


XB_TX_STATUS xbus_despatch(uint8 dest,uint8 type,uint8 *xdata);
int  _xbus_send_txbuff();

XB_TX_STATUS _xbus_transmit(uint8 dest,uint8 type,uint8 *xdata);
void xb_receive_on(void);
void xb_receive_off(void);
void _xb_tick(void);
int _rx_queue_pop(uint8 *buffer);
void xb_active_on_lpm3(void);
void xbus_wakedevices();

uint8 xbus_can_sleep();

void xb_sleep_device(uint8 device);
void xb_wake_device(uint8 device);

void xbus_start_tick();
int xbus_is_idle();
void xbus_clear_stats();
uint8 xbus_copy_stats(uint8 *buffer);
uint8 xbus_copy_info(uint8 *buffer);

int xbus_queue_msg(uint8 type,uint8 *xdata);

/*
unsigned char xb_strobe(unsigned char strobe);

void WriteRfSettings(RF_SETTINGS *pRfSettings);

void WriteSingleReg(unsigned char addr, unsigned char value);
void WriteBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count);
unsigned char ReadSingleReg(unsigned char addr);
void ReadBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count);
void WriteSinglePATable(unsigned char value);
void WriteBurstPATable(unsigned char *buffer, unsigned char count); 
*/

#endif
