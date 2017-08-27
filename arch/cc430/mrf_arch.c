/******************************************************************************
*
* Copyright (c) 2012-16 Gnusys Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************************/

#include <mrf_sys.h>
#include  <msp430.h>
#include <legacymsp430.h>
#include <rtc_arch.h>

extern uint8 _mrfid;

uint16 _at_sp;
uint16 _at_sp_1;


inline void mrf_intr_entry(){
  uint16 sp;
  asm("mov r1, %0": [sp] "=r" (sp):);
  _at_sp = *(uint16 *)sp;
  _at_sp_1 = *(uint16 *)(sp+2);
}


//FIXME shouldn't need putchar to make printf etc link. Shouldn't have printf

int putchar(int c){
}


int _print_mrf_cmd(MRF_CMD_CODE cmd){
  // empty for cc
  // should not be calling this in cross platform code.. only during debug.. 
}

int mrf_arch_boot(){

   int i,j,on;
  WDTCTL = WDTPW + WDTHOLD; 
  //receiving = 0;
  //transmitting = 0;
  // sprintf(message,"");
  

  SetVCore(2);   
  UCSCTL6 &= ~(XT1DRIVE0 | XT1DRIVE1);  // low power mode

  init_clock();
  // RSTAT=GetRF1ASTATB();
  //InitButtonLeds();

  P3OUT = 0x00;
  P3DIR = 0x00;
  // LCD1x9_Initialize();
 

  _mrf_receive_enable();
  //rtc_ps0_init(DIV64,ps0_handler);
  //starttimer_aclk();
  // __delay_cycles(10000);
  //get_ta_value(&delay_timer);
  rtc_init();
  rtc_ps0_init(DIV32,_mrf_tick);  // 1KHz tick

  //rtc_ps0_enable(ps0_handler);


  //rtc_rdy_enable(rdy_handler);

 //cdisplaytime();
  // xbus_init();
 // rtc_set(&time1);
  __bis_SR_register(GIE);
  // xb_receive_on();
 
  return 0;
}

/*
void putchar(char c){
}

*/

int mrf_arch_run(){
  int i;
  while(1){
    i = mrf_foreground();
  }
  return 0;
}
int mrf_rtc_get(TIMEDATE *td){
  _rtc_get(td);
  return 0;
}
int mrf_rtc_set(TIMEDATE *td){
  _rtc_set(td);
  return 0;
}


int mrf_tick_enable(){
  rtc_ps0_enable(_mrf_tick);
  __bis_SR_register(GIE);

  return 0;

}
int mrf_tick_disable(){
  rtc_ps0_disable();

  return 0;
}
int  mrf_wake()  {
  // clear LPM3 on reti
  __bic_SR_register(LPM3_bits);

}
int mrf_sleep(){
  __bis_SR_register(LPM3_bits  + GIE);

}
