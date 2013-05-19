#include <stdio.h>

int daysinmonth(int month, int year){
  int leap = (year % 4) == 0;
  
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

int _rtc_td_add_secs(TIMEDATE *td,uint8 secs){  
  uint8 temp1,temp2;
  if (secs > 59 ) return -1;
  temp1 = td->sec + secs;
  if (temp1 < 60 ){
    td->sec = temp1;
    return 0;
  }
  td->sec = temp1 - 60; 
  temp1 = td->min + 1;
  if (temp1 < 60 ){
    td->min = temp1;
    return 0;
  }
  td->min = 0; 
  temp1 = td->hour + 1;
  if (temp1 < 24 ){
    td->hour = temp1;
    return 0;
  }
  td->hour = 0;
  temp1 = td->day + 1;
  temp2 = daysinmonth(month, year);
  if (temp1 <= temp2)
    {
      td->day = temp1;
      return 0;
    }
  td->day = 1;
  temp1 = td->mon + 1;
  if (temp1 < 13){
    td->mon = temp1;
    return 0
  }
  td->mon = 1;
  td->year++;
  return 0;
}


void main(){
  int year,month;
  for (year = 2010; year < 2016 ; year++){
    printf("\n%d",year);
    for (month = 1 ; month <= 12 ; month++)
      printf("\n %d  %d ",month,daysinmonth(month,year));
  }
}
