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
#include "hal_pmm.h"

#include "mrf_pinmacros.h"
#define _WAKE_PORT P1
#define _WAKE_BIT  0


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
/*
int putchar(int c){
  return 0;
}
*/

int _print_mrf_cmd(MRF_CMD_CODE cmd){
  // empty for cc
  // should not be calling this in cross platform code.. only during debug.. 
}

extern void init_clock(void);


int _mrf_wakeup;


int mrf_arch_boot(){

  WDTCTL = WDTPW + WDTHOLD; 
  SFRRPCR |= SYSRSTUP | SYSRSTRE;

  SetVCore(2);   
  UCSCTL6 &= ~(XT1DRIVE0 | XT1DRIVE1);  // low power mode

  init_clock();

  P3OUT = 0x00;
  P3DIR = 0x00;
  // LCD1x9_Initialize();

  //PINHIGH(WAKE);
  //OUTPUTPIN(WAKE);

  _mrf_wakeup = 0;

  rtc_init();
  rtc_ps0_init(DIV32,_mrf_tick);  // 1KHz tick

  __bis_SR_register(GIE);
 
  return 0;
}


int  mrf_wake()  {
  // clear LPM3 on reti
  //WDTCTL = WDTPW + WDTIS_5 + WDTSSEL__ACLK + WDTCNTCL_L;
  //PINHIGH(WAKE);
  _mrf_wakeup = 1;  // only ISR can run __bic_SR_register_on_exit - mrf_wake should only be called via ISR
  //__bic_SR_register_on_exit(LPM3_bits);
  return 0;
}

int mrf_wake_on_exit(){
  if( _mrf_wakeup){
    _mrf_wakeup = 0;
    return 1;
  }
  return  0;
}

int _mrf_woken(){
  return 101;
}

int mrf_sleep(){
  // disable WDT
  //WDTCTL = WDTPW + WDTHOLD; 
  //PINLOW(WAKE);
  __bis_SR_register(LPM3_bits  + GIE);

  _mrf_woken();
  return 0;
}

int mrf_arch_run(){
  int i;
  while(1){
    //WDTCTL = WDTPW + WDTIS_5 + WDTSSEL__ACLK + WDTCNTCL_L;
    mrf_foreground();
    //while( mrf_foreground());
    //if (mrf_app_queue_available() == 0)

    mrf_sleep();
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

void mrf_reset(){
  WDTCTL = 0xDEAD;
}


