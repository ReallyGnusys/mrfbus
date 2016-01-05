#ifndef __MRF_SYS_STRUCTS_INCL__
#define __MRF_SYS_STRUCTS_INCL__

#include <mrf_types.h>
//#include <mrf_if.h>
#define MRF_ACK_FLAG_RETRY  1
#define MRF_ACK_FLAG_DELIVERED 2


typedef struct  __attribute__ ((packed)) {
  uint8 length;  // packet length
  uint8 hdest;  // hop destination
  uint8 netid;
  uint8 udest;// ultimate destination of this packet
  uint8 type;
  uint8 hsrc; // hop source
  uint8 usrc; // ultimate source of this packet 
  uint8 msgid; 
}MRF_PKT_HDR ;

typedef struct  __attribute__ ((packed)) timedate {
  uint8 sec;
  uint8 min;
  uint8 hour;
  uint8 day;
  uint8 mon;
  uint8 year;  // 0 = 2000CE  
}TIMEDATE;


typedef struct  __attribute__ ((packed))   {
  uint8 type;
  uint8 msgid; 
  uint8 rlen;  
} MRF_PKT_RESP;

typedef struct  __attribute__ ((packed))   {
  uint8 num_if;
  uint8 errors;
  uint16 tx_retries;
  uint32  rx_pkts;
  uint32 tx_pkts;

} MRF_PKT_IF_INFO;

typedef struct  __attribute__ ((packed))   {
  uint8 i_f;

}MRF_PKT_IF_STAT_REQ;

typedef struct  __attribute__ ((packed))   {
  char dev_name[10];
  uint8 mrfid;
  uint8 mrfnet;
  char mrfbus_version[40];
  uint8 modified;
} MRF_PKT_DEVICE_INFO;


typedef TIMEDATE MRF_PKT_TIMEDATE;
typedef TIMEDATE MRF_TIMEDATE;

typedef struct __attribute__ ((packed)){
  uint32 rx_pkts;
  uint32 tx_acks;
  uint32 tx_pkts;
  uint16 rx_crc_errs;
  uint16 rx_net_errs;
  uint16 rx_short;
  uint16 rx_long;
  uint16 tx_fails;
  uint16 tx_retries;
  uint16 tx_no_eop; 
  uint16 rx_overruns;
  uint16 rx_unexp_ack;
  uint16 tick_err;
  uint16 fatal;
  uint16 txn[4];
} MRF_PKT_STATS;

typedef struct  __attribute__ ((packed))   {
  uint16 data[8];
} MRF_PKT_DBG_UINT8;


typedef struct  __attribute__ ((packed))   {
  char data[32];
} MRF_PKT_DBG_CHR32;

#endif
