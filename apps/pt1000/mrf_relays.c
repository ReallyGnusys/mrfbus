#include "mrf.h"
#include  <msp430.h>
#include <legacymsp430.h>
#include "cc430f5137.h"
#include "mrf_pinmacros.h"

#ifndef NUM_RELAY_CHANNELS
#define NUM_RELAY_CHANNELS 4
#endif

volatile static uint8 _relay_state;


#define _RLA_PORT P3
#define _RLB_PORT P3
#define _RLC_PORT P3
#define _RLD_PORT P3

#ifndef _RLA_BIT
#define _RLA_BIT  4
#endif

#ifndef _RLB_BIT
#define _RLB_BIT  5
#endif
#ifndef _RLC_BIT
#define _RLC_BIT  6
#endif

#ifndef _RLD_BIT
#define _RLD_BIT  7
#endif


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
  if (chan >= NUM_RELAY_CHANNELS)
    return 0;   //FIXME should be error
  if ( val == 0 ){
    _relay_state &=  (uint8)(~( 1 << chan ));
    if (chan == 0 ) {
      PINHIGH(RLA);
    }
    else if (chan == 1 ){
      PINHIGH(RLB);
    }
    else if (chan == 2 ){
      PINHIGH(RLC);
    }
    else if (chan == 3 ){
      PINHIGH(RLD);
    }
  }
  else {
    _relay_state |=  (uint8)( 1 << chan );  
    if (chan == 0 ) {
      PINLOW(RLA);
    }
    else if (chan == 1 ){
      PINLOW(RLB);
    }
    else if (chan == 2 ){
      PINLOW(RLC);
    }
    else if (chan == 3 ){
      PINLOW(RLD);
    }
  }
}


void init_relays(){
  clear_relay_state();
  PINHIGH(RLA);
  OUTPUTPIN(RLA);
  PINHIGH(RLB);
  OUTPUTPIN(RLB);
  PINHIGH(RLC);
  OUTPUTPIN(RLC);
  PINHIGH(RLD);
  OUTPUTPIN(RLD);
}
