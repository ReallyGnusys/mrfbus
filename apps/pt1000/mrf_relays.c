#include "mrf.h"
#include  <msp430.h>
#include <legacymsp430.h>
#include "cc430f5137.h"
#include "mrf_pinmacros.h"

#define NUM_RELAY_CHANNELS 2

volatile static uint8 _relay_state;


#define _RLA_PORT P3
#define _RLA_BIT  5

#define _RLB_PORT P3
#define _RLB_BIT  4


void clear_relay_state(){
  _relay_state = 0;
}

uint8 relay_state(){
  return _relay_state;
}

uint8 get_relay_state(uint8 chan){
  if ( _relay_state & (uint8)( 1 << chan ))
    return 1;
  else
    return 0;
}

uint8 set_relay_state(uint8 chan,uint8 val){
  if ( val == 0 ){
    _relay_state &=  (uint8)(~( 1 << chan ));
    if (chan == 0 ) {
      PINLOW(RLA);
    }
    else if (chan == 1 ){
      PINLOW(RLB);
    }
  }
  else
    _relay_state |=  (uint8)( 1 << chan );  
    if (chan == 0 ) {
      PINHIGH(RLA);
    }
    else if (chan == 1 ){
      PINHIGH(RLB);
    }
}


void init_relays(){
  clear_relay_state();
  PINLOW(RLA);
  OUTPUTPIN(RLA);
  PINLOW(RLB);
  OUTPUTPIN(RLB);
}
