#ifndef _RTC_INCLUDED_
#define _RTC_INCLUDED_
#include  <msp430.h>
#include <legacymsp430.h>
#include "cc430f5137.h"
#include "g430_types.h"

typedef struct  __attribute__ ((packed)) timedate {
  uint8 sec;
  uint8 min;
  uint8 hour;
  uint8 day;
  uint8 mon;
  uint8 year;  // 0 = 2000CE  
}TIMEDATE;

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

int _rtc_td_is_valid(TIMEDATE *td); 
void rtc_clear_alarm();
void rtc_clear_eveie();
void rtc_wake_on_sec(uint8 sec);
void rtc_clear_wake_on_sec();
#endif
