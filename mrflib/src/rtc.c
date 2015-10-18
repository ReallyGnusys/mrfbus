
#include "rtc.h"

#define RTC_YEAR0  2000
#define RTC_MAXYEAR 255

static TIMEDATE UpdateTime;
static volatile TIMEDATE ReadTime;

static int8 UpdateFlag,ReadFlag,AlarmFlag;


static VFUNCPTR _rtc_ps0_handler;
static VFUNCPTR _rtc_rdy_handler;

static inline uint8  _y16_to_y8(uint16 y){
  y = y - RTC_YEAR0;
  y = y % RTC_MAXYEAR;
  return (uint8)y;
}
static inline uint16  _y8_to_y16(uint8 y){
 return  y + RTC_YEAR0;
}


void _rtc_default_ps0(void)
{
}
void _rtc_default_rdy(void)
{
}

static struct counts {
   uint32 seccnt;
   uint32 pscnt;
   uint32 pscnt2;
   uint32 reticnt;

} rtc_counts;

void _set_valid_datetime(){
  RTCSEC = 0;
  RTCMIN = 0;
  RTCHOUR = 0;
  RTCDAY = 1;
  RTCMON = 1;
  RTCYEAR =  _y8_to_y16(12);
}
//debug
TIMEDATE set_err_td;
int   set_err_cnt;

uint8 _rtc_wake_on_sec_flag, _rtc_wake_on_sec;
void rtc_init(){
  RTCCTL01 = RTCMODE;  /*set calendar mode*/
  _set_valid_datetime();
  _rtc_wake_on_sec_flag = 0;

  UpdateFlag = 0;
  ReadFlag = 0;
  AlarmFlag = 0;
  rtc_counts.seccnt = 0;
  rtc_counts.pscnt = 0;
  rtc_counts.pscnt2 = 0;
  rtc_counts.reticnt = 0;
  // allow ints on time update ( sec )
  _rtc_ps0_handler = _rtc_default_ps0;
  _rtc_rdy_handler = _rtc_default_rdy;
  //debug
   set_err_cnt = 0;


}

void rtc_wake_on_sec(uint8 sec){
  _rtc_wake_on_sec_flag = 1;
  _rtc_wake_on_sec = sec;
  RTCCTL01 |= RTCRDYIE;
}
void rtc_clear_wake_on_sec(){
  _rtc_wake_on_sec_flag = 0;
  RTCCTL01 &= ~RTCRDYIE;
}



void rtc_rdy_enable(VFUNCPTR func){
  _rtc_rdy_handler = func;
  // enable interrupt
  RTCCTL01 |= RTCRDYIE;
}

void rtc_ps0_init(RTCDIVCODE div,VFUNCPTR func){
  // disable interrupt
  RTCPS0CTL &= ~0x2;  

  _rtc_ps0_handler = func;

  //relying on enum order and encoding to set RT0IP bits
  RTCPS0CTL |= (uint16)div << 2;

  // clear flag
  RTCPS0CTL &= ~0x1;  

}
void rtc_ps0_enable(VFUNCPTR func){
  _rtc_ps0_handler = func;
  // enable interrupt
  RTCPS0CTL &= ~0x1;  
  RTCPS0CTL |= 0x2;    
}

void rtc_ps0_disable(){
  //  _rtc_ps0_handler = _rtc_default_ps0;
  RTCPS0CTL &= ~0x2;  
  // disable interrupt
  RTCPS0CTL &= ~0x1;  
 
}


int  _rtc_td_eq(TIMEDATE *td1,TIMEDATE *td2){  
  if(td1->sec != td2->sec) return 0;
  if(td1->min != td2->min ) return 0;
  if(td1->hour != td2->hour) return 0;
  if(td1->day != td2->day) return 0;
  if(td1->mon != td2->mon) return 0;
  if(td1->year != td2->year) return 0;
  return 1;
}

void _rtc_td_cpy(TIMEDATE *source,TIMEDATE *dest){  
  dest->sec = source->sec;
  dest->min = source->min;
  dest->hour = source->hour;
  dest->day = source->day;
  dest->mon = source->mon;
  dest->year = source->year;
}

uint8 daysinmonth(uint8 month, uint8 year){
  uint8 leap = (year % 4) == 0;
  
  switch(month){
  case 9:  return 30;
  case 4:  return 30;
  case 6:  return 30;
  case 11:  return 30;
  case 2:  
    if (leap) 
      return 29;
    else
      return 28;
  default: return 31;

  }  

}


void _rtc_td_clr(TIMEDATE *td){  
  td->sec = 0;
  td->min = 0;
  td->hour = 0;
  td->day = 1;
  td->mon = 1;
  td->year = 0;
}

int _rtc_td_isclr(TIMEDATE *td){  
  if (td->sec != 0) return 0;
  if (td->min != 0) return 0;
  if (td->hour != 0) return 0;
  if (td->day != 1) return 0;
  if (td->mon != 1) return 0;
  if (td->year != 0) return 0;
  return 1;
}

int _rtc_td_is_valid(TIMEDATE *td){  
  if (td->sec > 59 ) return 0;
  if (td->min >  59) return 0;
  if (td->hour > 24) return 0;
  if ((td->day < 1) || (td->day > 31)) return 0;
  if ((td->mon  < 1) || (td->mon > 12)) return 0;
  if (td->year < 12) return 0;  // xbus did not exist before 2012!
  return 1;
}

void _rtc_set_error(TIMEDATE *td){
  _rtc_td_cpy(td,&set_err_td);  
  set_err_cnt++;
}

void _rtc_get(TIMEDATE *td){
  td->sec = RTCSEC;
  td->min = RTCMIN;
  td->hour = RTCHOUR;
  td->day = RTCDAY;
  td->mon = RTCMON;
  td->year = _y16_to_y8(RTCYEAR) ;
}
void _rtc_set(TIMEDATE *td){
  RTCCTL01 |= RTCHOLD  ;
  RTCSEC = td->sec  ;
  RTCMIN = td->min  ;
  RTCHOUR = td->hour  ;
  RTCDAY = td->day  ;
  RTCMON = td->mon  ;
  RTCYEAR = _y8_to_y16(td->year)  ;
  RTCCTL01 &= ~RTCHOLD  ;

}


void rtc_set(TIMEDATE *td)
{
  TIMEDATE setval,rdval;
  _rtc_td_cpy(td,&setval);

  if (!_rtc_td_is_valid(&setval))
    {
      _rtc_set_error(&setval);
      return;
    }

  _rtc_set(&setval);
  _rtc_get(&rdval);
  while(!_rtc_td_eq(&setval,&rdval)  || !_rtc_td_is_valid(&rdval)){
    _rtc_set(&setval);
    _rtc_get(&rdval);

  }
  
  /*
  UpdateTime.sec = td->sec;
  UpdateTime.min = td->min;
  UpdateTime.hour = td->hour;
  UpdateTime.day = td->day;
  UpdateTime.mon = td->mon;
  UpdateTime.year = td->year;
  UpdateFlag = 1;
  RTCCTL0 |= RTCRDYIE ;
  */


  /*
  RTCSEC = td->sec;
  RTCMIN = td->min;
  RTCHOUR = td->hour;
  RTCDAY = td->day;
  RTCMON = td->mon;
  RTCYEAR = _y8_to_y16(td->year);
  */
}







TIMEDATE _td_debug;

void _rtc_get_error(TIMEDATE *td){
  _rtc_td_cpy(td,&_td_debug);
  __delay_cycles(1);
}
TIMEDATE _rtc_readval;

#define RTC_GET_TIMEOUT 4

int rtc_get_hang(TIMEDATE *td){

  while(!(RTCIV & RTCRDYIFG))
    __delay_cycles(5);
  
  
  _rtc_get(td);
  return 0;
}
 
int rtc_get(TIMEDATE *td){

  TIMEDATE t1,t2,*old,*new,*tmp;
  uint8 count = 0;
  old = &t1;
  new = &t2;

  
  
  _rtc_get(old);
  _rtc_get(new);
 
  while(!_rtc_td_eq(old,new) && (count < RTC_GET_TIMEOUT) || !_rtc_td_is_valid(new)){

    count++;
    // swap old and new
    tmp = old;
    old = new;
    new = tmp;
    // refresh new
    _rtc_get(new);
  }

  if (_rtc_td_is_valid(new) == 0){

    _rtc_get_error(new);
  }
  if ( count >= RTC_GET_TIMEOUT){
    // error getting consistent reading, send cleared structure
    _rtc_td_clr(new);
    _rtc_td_cpy(new,td);    
    return -1;
  }

  
  
  _rtc_td_cpy(new,td); 
  //*td = t1;
  return 0;
}

  
TIMEDATE *rtc_on_sec(){
  
  ReadFlag = 1;
  while(ReadFlag) 
    {
      RTCCTL0 |= RTCRDYIE ;
      __delay_cycles(100);
    }
  return (TIMEDATE*)&ReadTime;
}

volatile static uint8 _min_reached;


void alarm_gone(){
  __delay_cycles(1);

}

uint8  rtc_min_reached(){
  return _min_reached;
}
void rtc_wake_on_min(){
  _min_reached = 0;
  RTCCTL0 |= RTCTEVIE ;
    
}


void rtc_clear_eveie(){
  RTCCTL0 &= ~RTCTEVIE ;
  _min_reached = 0;
 

}

void rtc_sleep_til(TIMEDATE *td){  
  RTCAMIN = td->min ;
  RTCAHOUR = td->hour;
  RTCADAY = td->day;
  RTCADOW = RTCDOW;
  RTCCTL0 |=  RTCAIE;
  __bis_SR_register(LPM3_bits  + GIE);

}

void rtc_clear_alarm(){
  RTCAHOUR =0;
  RTCADAY = 0;
  RTCADOW = 0; 
  RTCAMIN = 0;
  RTCCTL0 &=  ~(RTCAIE |RTCAIFG) ;
   


}


void rtc_sleep_mins(uint8 mins){
  TIMEDATE td;
  int correction;
  rtc_get(&td);
  if (td.sec > 50 )
    correction = 1;
  else
    correction = 0;
  RTCAHOUR =0;
  RTCADAY = 0;
  RTCADOW = 0; 
  RTCAMIN = (td.min + mins + correction ) % 60 | 0x80 ;
  RTCCTL0 &=  ~RTCAIFG ;
  RTCCTL0 |=  RTCAIE;
  AlarmFlag = 0;
  //__bis_SR_register(LPM3_bits  + GIE);
}

uint8 rtc_alarm_flag(){
  return AlarmFlag;
}
static unsigned int RTCIVREG;

void rtc_debug(){
  __delay_cycles(1);
}

void rtc_dbg1(){
  __delay_cycles(1);
}
void rtc_dbg2(){
  __delay_cycles(1);
}

interrupt(RTC_VECTOR) RTC_ISR()
{
  RTCIVREG = RTCIV;
  switch(RTCIVREG){
  case RTCIV_RTCRDYIFG: /* update timedate */
    rtc_counts.seccnt++;
    if (_rtc_wake_on_sec_flag){
      rtc_dbg1();
      if (RTCSEC == _rtc_wake_on_sec){
	rtc_dbg2();

        __bic_SR_register_on_exit(LPM3_bits);    
      }
	
    }
    //(*_rtc_rdy_handler)();
    if(UpdateFlag){
      RTCSEC = UpdateTime.sec;
      RTCMIN = UpdateTime.min;
      RTCHOUR = UpdateTime.hour;
      RTCDAY = UpdateTime.day;
      RTCMON = UpdateTime.mon;
      RTCYEAR = _y8_to_y16(UpdateTime.year);
      UpdateFlag = 0;
    }
    if(ReadFlag){
      ReadTime.sec = RTCSEC;
      ReadTime.min = RTCMIN;
      ReadTime.hour = RTCHOUR;
      ReadTime.day = RTCHOUR;
      ReadTime.mon = RTCMON;
      ReadTime.year =  _y16_to_y8(RTCYEAR);
      ReadFlag = 0;
    }
    break;
  case RTC_RTCAIFG:
    AlarmFlag = 1;
    __bic_SR_register_on_exit(LPM3_bits);    
    alarm_gone();
    break;

  case  RTC_RTCTEVIFG:
    _min_reached = 1;
    __bic_SR_register_on_exit(LPM3_bits);    
    break;
  case  RTC_RT0PSIFG:
    rtc_counts.pscnt++;
    (*_rtc_ps0_handler)();
    rtc_counts.pscnt2++;
    break;
   
     
  default:
    
    break;

  }
  
  rtc_counts.reticnt++;

  
}
