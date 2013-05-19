#ifndef __XB_SYS_STRUCTS_INCL__
#define __XB_SYS_STRUCTS_INCL__
#include "rtc.h"

#define XB_ACK_FLAG_RETRY  1
#define XB_ACK_FLAG_DELIVERED 2


typedef struct  __attribute__ ((packed))   {
  uint8 flags[16];
} XB_PKT_ACK;


typedef struct  __attribute__ ((packed))   {
  char dev_name[16];
  char dev_version[6];
  char xbus_version[6];
} XB_PKT_DEVICE_INFO;


typedef TIMEDATE XB_PKT_TIMEDATE;

typedef struct __attribute__ ((packed)){
  uint32 rx_pkts;
  uint32 tx_acks;
  uint32 tx_pkts;
  uint16 rx_crc_errs;
  uint16 rx_net_errs;
  uint16 tx_fails;
  uint16 tx_retries;
  uint16 tx_no_eop; 
  uint16 rx_overruns;
  uint16 rx_unexp_ack;
  uint16 tick_err;
  uint16 fatal;
  uint16 txn[4];
} XB_PKT_STATS;

typedef struct  __attribute__ ((packed))   {
  uint16 data[8];
} XB_PKT_DBG_UINT8;


typedef struct  __attribute__ ((packed))   {
  char data[32];
} XB_PKT_DBG_CHR32;

#endif
