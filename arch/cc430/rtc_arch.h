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

#ifndef _RTC_INCLUDED_
#define _RTC_INCLUDED_
#include  <msp430.h>
#include <legacymsp430.h>
#include "cc430f5137.h"
#include "mrf_types.h"
#include "mrf_sys_structs.h"

void rtc_init(void);
int rtc_get(TIMEDATE *td);
void rtc_set(TIMEDATE *td);

void rtc_wake_on_min();

void rtc_sleep_mins(uint8);
uint8 rtc_min_reached();
uint8 rtc_alarm_flag();
typedef enum{
  DIV2,DIV4,DIV8,DIV16,DIV32,DIV64,DIV128,DIV256} RTCDIVCODE;

void rtc_ps0_init(RTCDIVCODE div,VFUNCPTR func);
void rtc_ps0_enable(VFUNCPTR func);
void rtc_ps0_disable();
void rtc_rdy_enable(VFUNCPTR func);

int _rtc_td_is_valid(TIMEDATE *td); 
void rtc_clear_alarm();
void rtc_clear_eveie();
void rtc_wake_on_sec(uint8 sec);
void rtc_clear_wake_on_sec();
#endif
