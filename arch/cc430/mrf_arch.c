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

static struct wake_stats_t {
  uint32 sleep;
  uint32 wake;
  uint32 wake_on_exit;
  uint32 wake_on_exit_yes;
  uint32 wake_on_exit_no;
  uint32 woken;
} wake_stats;


  
int mrf_arch_boot(){

  wake_stats.sleep = 0;
  wake_stats.wake = 0;
  wake_stats.wake_on_exit = 0;
  wake_stats.wake_on_exit_yes = 0;
  wake_stats.wake_on_exit_no = 0;
  wake_stats.woken = 0;
  
  WDTCTL = WDTPW + WDTHOLD; 
  SFRRPCR |= SYSRSTUP | SYSRSTRE;

#ifdef MCLK_4MHZ_DCO   //FIXME - need better way
  SetVCore(0);
#else
  SetVCore(2);
#endif
  UCSCTL6 &= ~(XT1DRIVE0 | XT1DRIVE1);  // low power mode

  init_clock();

  P3OUT = 0x00;
  P3DIR = 0x00;
  // LCD1x9_Initialize();

  //PINHIGH(WAKE);
  //OUTPUTPIN(WAKE);

  _mrf_wakeup = 0;

  rtc_init();
 
  return 0;
}



int  mrf_wake()  {
  // clear LPM3 on reti
  //WDTCTL = WDTPW + WDTIS_5 + WDTSSEL__ACLK + WDTCNTCL_L;
  //PINHIGH(WAKE);
  wake_stats.wake++;
  _mrf_wakeup = 1;  // only ISR can run __bic_SR_register_on_exit - mrf_wake should only be called via ISR
  //__bic_SR_register_on_exit(LPM3_bits);
  return 0;
}

int mrf_wake_on_exit(){
  wake_stats.wake_on_exit++;

  if( _mrf_wakeup){
    wake_stats.wake_on_exit_yes++;
    _mrf_wakeup = 0;
    return 1;
  }
  wake_stats.wake_on_exit_no++;
  return  0;
}

int _mrf_woken(){
  wake_stats.woken++;

  return 101;
}

int mrf_sleep(){ 
  // disable WDT
  //WDTCTL = WDTPW + WDTHOLD; 
  //PINLOW(WAKE);
  wake_stats.sleep++;
  __bis_SR_register(LPM3_bits);    // (LPM3_bits  + GIE);
  // shouldn't need GIE here, it should always be set in foreground anyway
  // and this should only be called from foreground

  _mrf_woken();
  return 0;
}

int mrf_arch_run(){
  int i;
  rtc_ps0_init(DIV32,_mrf_tick);  // 1KHz tick
  __bis_SR_register(GIE);  // GIE should always be set for foreground process

  while(1){
    //WDTCTL = WDTPW + WDTIS_5 + WDTSSEL__ACLK + WDTCNTCL_L;
    mrf_foreground();
    //while( mrf_foreground());
    //if (mrf_app_queue_available() == 0)
#ifndef SLEEP_none
    mrf_sleep();
#endif    
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
  // __bis_SR_register(GIE);

  return 0;

}
int mrf_tick_disable(){
  rtc_ps0_disable();

  return 0;
}

void mrf_reset(){
  WDTCTL = 0xDEAD;
}


