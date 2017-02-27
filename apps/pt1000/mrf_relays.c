#include "mrf.h"

#define NUM_RELAY_CHANNELS 2

volatile static uint8 _relay_state;

uint8 get_relay_state(uint8 chan){
  if ( _relay_state & (uint8)( 1 << chan ))
    return 1;
  else
    return 0;
}

uint8 set_relay_state(uint8 chan,uint8 val){
  if ( val == 0 )
    _relay_state &=  (uint8)(~( 1 << chan ));
  else
    _relay_state |=  (uint8)( 1 << chan );  
}


