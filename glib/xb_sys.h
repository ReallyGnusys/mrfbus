#ifndef __XB_SYS_INCLUDED__
#define __XB_SYS_INCLUDED__
/* xbus "protected" symbols , not for application use */

typedef enum {
  XB_ST_NONE,
  XB_ST_IDLE,
  XB_ST_TX,
  XB_ST_WAITSACK,  // wait for segment ack
  XB_ST_WAITUACK,  // wait for ultimate ack
  XB_ST_CHKACK,
  XB_ST_RX,
  XB_ST_ACKDELAY,
  XB_ST_ACK,
} XB_STATE;


typedef struct {
  int len;
  int csumok;
  uint8 linktoks[2];
} RX_PKT_INFO;

typedef struct  __attribute__ ((packed)) {
  uint8 hdest;  // hop destination
  uint8 udest;// ultimate destination of this packet
  uint8 netid;
  uint8 type;
  uint8 hsrc; // hop source
  uint8 usrc; // ultimate source of this packet 
  uint16 msgid;
  /*
  uint8 rssi;  // rssi info byte
  uint8 lqi;  // lqi info byte
  */
}XB_PKT_HDR ;


typedef volatile struct  __attribute__ ((packed)){
 uint8  len;
 XB_PKT_HDR hdr;
 XB_PKT_TIMEDATE td; 
 uint8 xdata[64+2];  // RSSI and CRC appended
} XB_RX_BUFF  ;


extern volatile XB_RX_BUFF _xb_rx_buff;
typedef enum {
  XB_CMD_RES_RETRY,
  XB_CMD_RES_ACCEPTED,
  XB_CMD_RES_IGNORE,
  XB_CMD_RES_ERROR
} XB_CMD_RES;
//#define XB_CMD_RES int




typedef XB_CMD_RES (*XB_CMD_FUNC)(XB_CMD_CODE cmd, XB_RX_BUFF *buff );


#ifndef HOSTBUILD
#define XB_CMD_FUNC_DEC(dec)   dec
#else
#define XB_CMD_FUNC_DEC(dec)   NULL
#endif

/* variable command flags  - MSBs of command type code*/
#define XB_VFLG_PAYLOAD 0x20
#define XB_VFLG_INITF   0x40
#define XB_VFLAG_SEND   0x80

#define XB_VFLAG_MASK   (XB_VFLG_PAYLOAD | XB_VFLG_INITF | XB_VFLAG_SEND )
#define XB_CODE_MASK   ~XB_VFLAG_MASK
/* constant flags */
#define XB_CFLG_NACK 1   // send ack when received 
#define XB_CFLG_INTR    2  // task is run in interrupt handler

typedef struct {
  const char *str;
  const uint8 cflags;
  const uint8 size;
  void *data;
  XB_CMD_FUNC func;
} XB_CMD;
#endif
