#include <mrf_sys.h>
#include  <msp430.h>
#include <legacymsp430.h>
#include <rtc.h>

extern uint8 _mrfid;


extern MRF_IF_TYPE rf_if_type;

//FIXME shouldn't need putchar to make printf etc link. Shouldn't have printf

int putchar(int c){
}




int mrf_arch_init(){

   int i,j,on;
  WDTCTL = WDTPW + WDTHOLD; 
  //receiving = 0;
  //transmitting = 0;
  // sprintf(message,"");
  

  SetVCore(0);   
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
 
  //rtc_ps0_enable(ps0_handler);


  //rtc_rdy_enable(rdy_handler);

 //cdisplaytime();
  // xbus_init();
 // rtc_set(&time1);
  __bis_SR_register(GIE);

  // xb_receive_on();
 
 
}

/*
void putchar(char c){
}

*/

int mrf_arch_main_loop(){
  return 0;
}
int mrf_rtc_get(TIMEDATE *td){
  return 0;
}


int mrf_tick_enable(){
  rtc_ps0_init(DIV32,_mrf_tick);  // 1KHz tick
  return 0;

}
int mrf_tick_disable(){
  return 0;
}
