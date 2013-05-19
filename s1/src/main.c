/******************************************************************************
 xbus packet sniffer example
 Copyright 2012 Gnusys Ltd   -  http://www.gnusys.com 
******************************************************************************/


#include  <msp430.h>
#include <legacymsp430.h>

#include "RF_Toggle_LED_Demo.h"
#include "Temp.h"
#include "spi.h"
#include "stdio.h"
//#include "rtc.h"

//#include  <intrinsics.h>
#include "LCD1x9.h"
#include "uart.h"

#define  PACKET_LEN         (0x05)			// PACKET_LEN <= 61
#define  RSSI_IDX           (PACKET_LEN)    // Index of appended RSSI 
#define  CRC_LQI_IDX        (PACKET_LEN+1)  // Index of appended LQI, checksum
#define  CRC_OK             (BIT7)          // CRC_OK bit 
#define  PATABLE_VAL        (0x51)          // 0 dBm output 

extern RF_SETTINGS rfSettings;

unsigned char packetReceived;
unsigned char packetTransmit; 


#define DEVCODE 25
#define TEMPTYPE  1






volatile unsigned char buttonPressed = 0;
unsigned int i = 0; 


volatile char dummychar;

//float tempc;

int count;
int modtcCx4;
int mtcC;
int mtcR;

#if 1
#include "xbus.h"

XB_MSG_TEMP temp_msg;
XB_TX_STATUS txres;

void tx_success(XB_TX_STATUS res){
  
  txres = res;
}

void tx_fail(XB_TX_STATUS res){
  
  txres = res;
}



TIMEDATE timedate;


#endif


void dummyfunc(){
  if(dummychar)
    dummychar = 0;
  //    __no_operation(); 

}


int Temperature;

extern uint8  RSTAT;
int TIMEX;
unsigned char GetRF1ASTATB(void);


uint8 rf_stat;
void UpdateRFSTAT(){
  rf_stat = GetRF1ASTATB();
}
//unsigned char GetRF1ASTATB(void){
// return Strobe(RF_SNOP);    
//}

uint8 msg_buffer[64];

#define NIB2HEX(nib) (nib > 9) ? ( 'A' - 10 + nib) : ('0'+nib)

void char2hex(uint8 chr,char *buff){
  buff[0] = NIB2HEX(chr/16);
  buff[1] = NIB2HEX(chr%16);
  
}
void got_message(){
  P1OUT ^= 0x01; // toggle LED
  // uart_tx_string("Got something",64);
  
  //uart_tx_string(msg_buffer,64);
}
void get_xmessage(){
  int len;  
  len = _rx_queue_pop(msg_buffer);  
  uart_tx_data(msg_buffer,len);
  //uart_tx_data_nointr(msg_buffer,len);  
}
//int receiving,transmitting;
static TIMEDATE time1;

int main( void )
{  
  int i,j,on;
  WDTCTL = WDTPW + WDTHOLD; 
  //receiving = 0;
  //transmitting = 0;
  
  time1.sec = 0;
  time1.min = 0;
  time1.hour = 0;
  time1.day = 0;
  time1.mon = 0;
  time1.year = 0;


  SetVCore(2);     

  UCSCTL6 &= ~(XT1DRIVE0 | XT1DRIVE1);  // low power mode

  init_clock();
   
  rtc_init();
  rtc_set(&time1);

  xbus_init();
  //ResetRadioCore();
  //InitRadio();
  RSTAT=GetRF1ASTATB();
  InitButtonLeds();

  P3OUT = 0x00;
  P3DIR = 0x00;
  LCD1x9_Initialize();
 
   uart_init();
  xb_receive_on();


  TIMEX = 0;
 

  xb_receive_off();

 __bis_SR_register(GIE);
  
 //cdisplaytime();


  // get temperature
  //modtcCx4 = 0x77;

 while((RSTAT=GetRF1ASTATB()) & 0x80){
    __delay_cycles(100);
    TIMEX++;
  }

   spi_init();
  
  // modtcCx4 = modtc_celx4();
  //mtcC = modtcCx4/4;
  //mtcR =  25*(modtcCx4%4);

  //initTA1();

  count = 0;
  xb_receive_on();

  while(1){
    
    //    rtc_wake_on_min();
    // __bis_SR_register(LPM3_bits  + GIE);
    
  
    // wait for uart buffer
    // while (!uart_rx_rdy());

    while(!_rx_queue_data_avail())
      __delay_cycles(10);
  
    
    get_xmessage();
    got_message();
    //measuretemp();
    //xb_receive_on();
    //rtc_sleep_mins(1);
    //xb_receive_on();
    UpdateRFSTAT();
    //displaytime();

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
  P1DIR |= BIT0;
  //P3OUT &= ~BIT6;
  //P3DIR |= BIT6;
}


