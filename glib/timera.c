#include "timera.h"

static uint16 _ta_of_cnt;

static uint16 _ta0r(){
  return TA0R;
  /*  uint16 curr,last = TA0R;
  while ((curr = TA0R) != last){
    last = curr;
  }
  return curr;
  */
}

uint32  _ta_val(){
  return (uint32)((uint32)_ta_of_cnt * (uint32)65536  + (uint32)TA0R);
}

void get_ta_value(uint32 *val){
  TA0CTL &= ~(MC0 | MC1);// stop counter
  // *val =  (uint32)(_ta_of_cnt * (uint32)65536  + TA0R);
  *val =  _ta_val();
  TA0CTL |= MC_2;  // continue counting
  //  return (_ta_of_cnt << 16) + _ta0r();
}
void reset_ta_value(){
  TA0CTL &= ~(MC0 | MC1);// stop counter
  TA0CTL |= TACLR;
  _ta_of_cnt = 0;
  TA0CTL |= MC_2;  // continue counting
}

void starttimer(){
  _ta_of_cnt = 0;
  TA0CTL = TASSEL1 + ID_0 + ID_1 + MC_2 + TACLR +  TAIE;

}
void starttimer_aclk(){
  _ta_of_cnt = 0;
  TA0CTL = TASSEL_1+ ID_0 + MC_2 + TACLR +  TAIE;

}


uint32 get_ta_count(){
  return _ta_of_cnt;
  /*
  uint16 ta0reg = TA0R;
  
  return  ((uint32)_ta_of_cnt << 16) + ta0reg;
  */

}
static uint16 TA0IV_reg;
interrupt(TIMER0_A0_VECTOR) TIMERA0_ISR(void)
{
  TA0IV_reg = TA0IV;

  switch(TA0IV_reg)        //
    {
    case (TA0IV_TA0IFG):
      _ta_of_cnt++;
      break;
    default: break;
    }

 
}

static uint16 TA1IV_reg;
// why is this triggered when we start timer a0?
interrupt(TIMER0_A1_VECTOR) TIMERA1_ISR(void)
{

  TA1IV_reg = TA0IV;
  switch(TA1IV_reg)        //
    {
    case (TA1IV_TA1IFG):
      _ta_of_cnt++;
      break;
    default: break;
    }

 
}
