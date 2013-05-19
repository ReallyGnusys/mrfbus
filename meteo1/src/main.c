/******************************************************************************
*xbus weather station METEO1 - main program
* Copyright  Gnusys Ltd ( www.gnusys.com ) 2012
******************************************************************************/


#include  <msp430.h>
#include <legacymsp430.h>
#include "g430_types.h"

#include "RF_Toggle_LED_Demo.h"
#include "Temp.h"
//#include "spi.h"
#include "i2c.h"
#include "stdio.h"
#include "rtc.h"
#include "timera.h"
#include "adc.h"
//#include  <intrinsics.h>
#include "LCD1x9.h"

#include "xqueues.h"

#include "mpl115a2.h"
#include "sht21.h"




const int USIZE32 = sizeof(uint32);
const int USIZE16 = sizeof(uint16);
const int USIZE8 = sizeof(uint8);
const int SIZE32 = sizeof(int32);
const int SIZE16 = sizeof(int16);
const int SIZE8 = sizeof(int8);



volatile unsigned char buttonPressed = 0;


volatile char dummychar;

static uint16 vcc;

//float tempc;

char message[20];
char msg_buffer[32];
int count;
int modtcCx4;
int mtcC;
int mtcR;

//void init_clock(void);

#if 1
#include "xbus.h"
#include "xb_usr_structs.h"
#include "xb_cmd_def.h"


XB_PKT_MODTC temp_msg;
XB_PKT_METEO1 meteo_msg;
XB_PKT_METEO1_COEFFS meteo_coeffs_msg;
XB_TX_STATUS txres;

void tx_success(XB_TX_STATUS res){
  
  txres = res;
}

void tx_fail(XB_TX_STATUS res){
  
  txres = res;
}
uint8 xbtemp[24];
void ttemp_end(){
  __delay_cycles(10);
}

XB_TX_STATUS transmit_temp(uint8 whole, uint8 part){
  XB_TX_STATUS res,ires;
  uint8 *src,i;
  return 0;

  
}

XB_TX_STATUS transmit_meteo1(XB_PKT_METEO1 *msg){
#if 0
  XB_TX_STATUS res,ires;
  uint8 *src,i;

  xbtemp[0] = XB_HUBADDR;
  xbtemp[1] = xb_cmd_meteo1;    
  src = (uint8 *)msg;
  for ( i = 0 ; i < sizeof(XB_PKT_METEO1);i++)
    xbtemp[2+i] = src[i];

  xq_tx_push(xbtemp,sizeof(XB_PKT_METEO1)+2);
  xbus_start_tick();
  while ( xbus_is_idle() == 0)
    __delay_cycles(10);
  ttemp_end();
#endif
  return 0;

  
}
XB_TX_STATUS transmit_meteo1_coeffs(XB_PKT_METEO1_COEFFS *msg){
#if 0
  XB_TX_STATUS res,ires;
  uint8 *src,i;
  xbtemp[0] = XB_HUBADDR;
  xbtemp[1] = xb_cmd_meteo1_coeffs;    
  src = (uint8 *)msg;
  for ( i = 0 ; i < sizeof(XB_PKT_METEO1_COEFFS);i++)
    xbtemp[2+i] = src[i];

  xq_tx_push(xbtemp,sizeof(XB_PKT_METEO1_COEFFS)+2);
  xbus_start_tick();
  while ( xbus_is_idle() == 0)
    __delay_cycles(10);
#endif

  return 0;
 
  
}

// initialise measurements
void StartMeteoMeas(){

  sht21_start_temp();
  adc_supply(0);
  

}
int info_hub(){
  int i;
  xbtemp[0] = XB_HUBADDR;
  xbtemp[1] = xb_cmd_device_info;  
  i = xbus_copy_info(&xbtemp[2]);
  /*
  for ( i = 0 ; i < sizeof(XB_DEVICEINFO) ; i++)
    xbtemp[2 + i] = src[i];
  */

  xq_tx_push(xbtemp,sizeof(XB_PKT_DEVICE_INFO)+2);
  xbus_start_tick();

  while ( xbus_is_idle() == 0)
    __delay_cycles(10); 
  return 0;
}
TIMEDATE timedate;
/*
void display_info(uint8 twhole, uint8 tpart){
  if( rtc_get(&timedate) == 0)
    sprintf(message,"%02d:%02d %2dC",timedate.hour,timedate.min,twhole);
  else
    sprintf(message,"BAD");
  LCD1x9_Write(message);


}
*/
#endif

unsigned char whole,part;

unsigned char measuretemp(){
  XB_TX_STATUS status;
  modtcCx4 = modtc_celx4();
  mtcC = modtcCx4/4;
  mtcR =  25*(modtcCx4%4);

  if (mtcC < 0xff)
    whole = (unsigned char) mtcC;
  else
    whole = 0xff;
  if (mtcR < 0xff)
    part = (unsigned char) mtcR;
  else
    part = 0xff;
  
  status = transmit_temp(whole,part);
  // tempc = (float)modtcCx4 * 0.25;
  count++;

  // round for display
  if (part >= 0x80)
    if (whole < 0xfe) whole += 1;
  // display_info(whole,part);

  if (status == XB_TX_STAYAWAKE)
    return 1;
  else return 0;


}


unsigned char measure(){
  XB_TX_STATUS status;
  modtcCx4 = modtc_celx4();
  mtcC = modtcCx4/4;
  mtcR =  25*(modtcCx4%4);

  if (mtcC < 0xff)
    whole = (unsigned char) mtcC;
  else
    whole = 0xff;
  if (mtcR < 0xff)
    part = (unsigned char) mtcR;
  else
    part = 0xff;
  
  status = transmit_temp(whole,part);
  // tempc = (float)modtcCx4 * 0.25;
  count++;

  // round for display
  if (part >= 0x80)
    if (whole < 0xfe) whole += 1;
  // display_info(whole,part);

  if (status == XB_TX_STAYAWAKE)
    return 1;
  else return 0;


}

#if 0
void displaytime(){
  if( rtc_get(&timedate) == 0)
    sprintf(message,"%2d.%02d:%02d",timedate.hour,timedate.min,timedate.sec);
  else
    sprintf(message,"BAD");
  LCD1x9_Write(message);


}

#endif

void dummyfunc(){
  // if(dummychar)
  //  dummychar = 0;
      __no_operation(); 

}

void turn_off_tick(){
  rtc_ps0_disable();
  __delay_cycles(50);

}


void tick_off(){
  __delay_cycles(1);

}




void sent_sleep(){
  while(!xbus_is_idle())
    __delay_cycles(10);
 
  turn_off_tick();
  tick_off();
}

int Temperature;

uint32 timerVal;

extern uint8  RSTAT;
int TIMEX;
unsigned char GetRF1ASTATB(void);
//unsigned char GetRF1ASTATB(void){
// return Strobe(RF_SNOP);    
//}

static uint32 rtc_ps0_cnt;

void ps0_handler(){
  rtc_ps0_cnt++;
}

static uint32 ta0_cnt_per_sec;
static uint32 rtc_rdy_cnt;



void rdy_handler(){
  rtc_rdy_cnt++;
  get_ta_value(&ta0_cnt_per_sec);
  //ta0_cnt_per_sec = get_ta_value();
  reset_ta_value();
  
}
void	end_l1(){
  __delay_cycles(10);
}
void	end_l2(){
  __delay_cycles(10);
}
void	about_to_sleep(){
  __delay_cycles(10);
}
void	after_sleep(){
}
void	temp_measured(){
  __delay_cycles(10);
}

void	decision_time(){
  __delay_cycles(10);
}
static TIMEDATE time1;

uint32 delay_timer;

uint16 sht21_temp,sht21_rh;
uint16 mpl_pmeas,mpl_tmeas;
uint8 mplres[4];
uint8 mplcoeffs[8];

int mm_errors;
void mm_error(){
  mm_errors++;
  __delay_cycles(1);
}

int meteo_measure(){

  if (sht21_start_temp() !=0 ) {
    mm_error();
  }
  if(sht21_read_measurement(&sht21_temp) != 0){
    mm_error();

  }

   sht21_start_rh();
   sht21_read_measurement(&sht21_rh);

   mpl115a_start_convert();
   mpl115a_conv_delay();
   mpl115a_read_results();
   mpl115a_copy_results(mplres);

   mpl_pmeas = mplres[0] * 256 + mplres[1];
   mpl_tmeas = mplres[2] * 256 + mplres[3];   


   vcc = adc_measure_supply();

   meteo_msg.adc = vcc;
   meteo_msg.sens_temp_adc = sht21_temp;
   meteo_msg.sens_humid_adc = sht21_rh;
   meteo_msg.baro_temp_adc  = mpl_tmeas;
   meteo_msg.baro_press_adc  = mpl_pmeas;
   transmit_meteo1(&meteo_msg);

}

uint16 dgb_u16_buff[8];
char dgb_c32_buff[32];


//int receiving,transmitting;
int main( void )
{  
  int i,j,on;
  WDTCTL = WDTPW + WDTHOLD; 
  //receiving = 0;
  //transmitting = 0;
  // sprintf(message,"");
  dgb_u16_buff[0] = 0x1234;
  dgb_u16_buff[1] = SYSRSTIV ;

  dgb_u16_buff[2] = SFRIFG1 ;

  dgb_u16_buff[3] = PMMCTL0;
  dgb_u16_buff[4] = SVSMHCTL;

  dgb_u16_buff[5] = SVSMLCTL;
  dgb_u16_buff[6] = PMMIFG;
  dgb_u16_buff[7] = PMMRIE;

  mm_errors = 0;
  rtc_ps0_cnt = 0;
  rtc_rdy_cnt = 0;
  time1.sec = 0;
  time1.min = 0;
  time1.hour = 0;
  time1.day = 1;
  time1.mon = 1;
  time1.year = 12;

  rtc_ps0_cnt = 0;
  SetVCore(0);   
  UCSCTL6 &= ~(XT1DRIVE0 | XT1DRIVE1);  // low power mode

  init_clock();
  // RSTAT=GetRF1ASTATB();
  //InitButtonLeds();

  P3OUT = 0x00;
  P3DIR = 0x00;
  // LCD1x9_Initialize();
 
  TIMEX = 0;
 
  //rtc_ps0_init(DIV64,ps0_handler);
  //starttimer_aclk();
  // __delay_cycles(10000);
  //get_ta_value(&delay_timer);
 
  //rtc_ps0_enable(ps0_handler);


  //rtc_rdy_enable(rdy_handler);
  sprintf(dgb_c32_buff,"Hello from debug");

 //cdisplaytime();
 xbus_init();
 // rtc_set(&time1);
  __bis_SR_register(GIE);

 xb_receive_on();
 /*
 xbus_queue_msg(xb_cmd_dbg_chr32,dgb_c32_buff);
 xbus_queue_msg(xb_cmd_dbg_uint8,(uint8 *)dgb_u16_buff);

 //vcc = adc_supply(0);
 */
 info_hub(); // send device info to hub (also sets time)

 UCSCTL6 |= XT2OFF;

 
 //spi_init();
  
   i2c_init();
   for ( i = 0 ; i < 100; i++){
     __delay_cycles(1000);

   }
   mpl115a_init();

   mpl115a_copy_coeffs(mplcoeffs);
   meteo_coeffs_msg.a0 = mplcoeffs[0]*256 + mplcoeffs[1];
   meteo_coeffs_msg.b1 = mplcoeffs[2]*256 + mplcoeffs[3];
   meteo_coeffs_msg.b2 = mplcoeffs[4]*256 + mplcoeffs[5];
   meteo_coeffs_msg.c12 = mplcoeffs[6]*256 + mplcoeffs[7];
   transmit_meteo1_coeffs(&meteo_coeffs_msg);

   meteo_measure();

   
  sprintf(dgb_c32_buff,"First wake");

  //rtc_ps0_enable();

  rtc_wake_on_min();
  about_to_sleep();
  //PMMIFG = 0 ;
  // PMMRIE = 0 ;
  PMMCTL0_H = 0xA5;
  PMMIFG = 0 ;
  PMMCTL0_H = 0x00;

  dgb_u16_buff[0] = 0x1235;
  dgb_u16_buff[1] = SYSRSTIV ;

  dgb_u16_buff[2] = SFRIFG1 ;

  dgb_u16_buff[3] = PMMCTL0;
  dgb_u16_buff[4] = SVSMHCTL;

  dgb_u16_buff[5] = SVSMLCTL;
  dgb_u16_buff[6] = PMMIFG;
  dgb_u16_buff[7] = PMMRIE;
  //  xbus_queue_msg(xb_cmd_dbg_uint8,(uint8 *)dgb_u16_buff);
  while ( xbus_is_idle() == 0)
    __delay_cycles(10);

  if ( xbus_can_sleep()){
    Strobe( RF_SXOFF );  // need this or get POR after exit from sleep!
    __bis_SR_register(LPM3_bits  + GIE);
  }
  
  xb_receive_on();

  //  xbus_queue_msg(xb_cmd_dbg_chr32,dgb_c32_buff);
  while ( xbus_is_idle() == 0)
    __delay_cycles(10);

  while(1){
    rtc_sleep_mins(1);
  __bis_SR_register(GIE);
    Strobe( RF_SIDLE );
    Strobe( RF_SFTX  );    
    meteo_measure();

    while(xbus_is_idle() == 0){
      __delay_cycles(50);
    }
    decision_time();

    if ( xbus_can_sleep())
      { // normal sleep
	Strobe( RF_SXOFF );
	/*
	rtc_wake_on_min();
	*/
	about_to_sleep();
	__bis_SR_register(LPM3_bits  + GIE);

	after_sleep();
      }
    else 
      {
	// stay awake
	//rtc_wake_on_min();
	xb_receive_on();
	while (!rtc_alarm_flag() && !xbus_can_sleep()){
	  while( xq_app_data_avail())
	    xq_app_pop(msg_buffer);   

	}

	if(!rtc_alarm_flag())
	  sent_sleep();
	/*
	if (xbus_can_sleep()){  // check if loop broken by sleep enable
	  Strobe( RF_SXOFF );
	  rtc_wake_on_min();
	  __bis_SR_register(LPM3_bits  + GIE);
	}
	*/

	end_l1();
      }
    end_l2();
  }


  


  return 0;
}

/*
interrupt(TIMER1_A0_VECTOR)  TIMER1_A0_ISR(void)
{
  TA1CCR0 += 50000;                         // Add Offset to CCR0
  TACOUNT++;
  if ((TACOUNT % TADIV) == 0) {
     TAEXIT++;
     __bic_SR_register_on_exit(LPM3_bits);
    }
  //TAEXIT++;

  //__bic_SR_register_on_exit(LPM3_bits);

}
*/

static unsigned int P1IVREG;

interrupt(PORT1_VECTOR) PORT1_ISR (void) 
{
  // switch(__even_in_range(P1IV, 16))
 
  P1IVREG = P1IV;
  switch(P1IVREG)
  {
    case  0: break;
    case  2:break;                         // P1.0 IFG
    case  4: 
      P1IE &= ~BIT1;                             // Debounce by disabling buttons
      buttonPressed = 1;
      //   __bic_SR_register_on_exit(LPM3_bits); // Exit active   
      break;                         // P1.1 IFG
    case  6: break;                         // P1.2 IFG
    case  8: break;                         // P1.3 IFG
    case 10: break;                         // P1.4 IFG
    case 12: break;                         // P1.5 IFG
    case 14: break;                         // P1.6 IFG
    case 16: break;
  }
}


void InitButtonLeds(void)
{
  // Set up the button as interruptible 
  P1DIR &= ~BIT1;

  //  P1REN |= BIT1;
  P1REN = 0xFc;
  /*
  P1IES &= BIT1;
  P1IFG = 0;
  P1IE  |= BIT1; 
  */
  P1IES |=  BIT1;  // hi to lo 
  //P1IFG &= ~BIT1;
  P1IFG = 0;
  P1IE  |=  BIT1;


  //  P1OUT |= BIT1;
  P1OUT &= ~BIT1;

  // Initialize Port J
  PJOUT = 0x00;
  PJDIR = 0xFF; 

  // Set up LEDs 
   P1OUT &= ~BIT0;
   //P1OUT |= BIT0;
  P1DIR |= BIT0;
  //P3OUT &= ~BIT6;
  //P3DIR |= BIT6;
}


