#ifndef __MRF_BUFF_INCLUDED__
#define __MRF_BUFF_INCLUDED__

//#include "xbus.h"
//#include "xb_sys.h"
#include <mrf_if.h>


#include "mrf_sys_structs.h"

typedef  enum { FREE,
		LOADING, // allocated and being written by IF or app 
		LOADED,  // loaded and awaiting classification
		TXQUEUE, // loaded and requires forwarding via an interface ,
                TX,  // currently being transmitted by I_F
                APPIN,  // loaded and requires processing by app
} mrf_buff_state_t;

typedef struct __attribute__ ((packed)){
  mrf_buff_state_t state;
  I_F owner;
  uint16 tx_timer;		
} MRF_BUFF_STATE;


uint8 *mrf_alloc_if(I_F i_f);
void mrf_free(uint8* buff);
void mrf_buff_loaded_if(I_F owner, uint8 *buff,uint8 em);

uint8* _mrf_buff_ptr(uint8 bind);
MRF_BUFF_STATE *_mrf_buff_state(uint8 bnum);

//extern static uint8 _mrf_buff[_MRF_BUFFS][_MRF_BUFFLEN];
#endif
