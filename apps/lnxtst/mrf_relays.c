#ifndef __MRF_RELAYS_INCLUDED__
#define __MRF_RELAYS_INCLUDED__


#include "mrf.h"
#ifndef NUM_RELAY_CHANNELS
#define NUM_RELAY_CHANNELS 4
#endif


uint16 _relay_state;


void clear_relay_state(){
  _relay_state = 0;
  mrf_debug(4,"clear_relay_state 0x%u\n",_relay_state);
}

uint16 relay_state(){
  mrf_debug(4,"relay_state 0x%u\n",_relay_state);
  return _relay_state;
}

uint8 get_relay_state(uint8 chan){
  mrf_debug(4,"get_relay_state 0x%u\n",_relay_state);
  if ( _relay_state & (uint16)( 1 << chan ))
    return 1;
  else
    return 0;
}

uint8 set_relay_state(uint8 chan,uint8 val){
  if (chan >= NUM_RELAY_CHANNELS)
    return 0;   //FIXME should be error
  if ( val == 0 ){
    _relay_state &=  ~((uint16)( 1 << chan ));
  }
  else {
    _relay_state |=  (uint16)( 1 << chan );
  }
  mrf_debug(4,"set_relay_state 0x%u\n",_relay_state);

  return 0;
}


void init_relays(){
  clear_relay_state();
}
#endif
