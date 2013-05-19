/******************************************************************************
* CC430 RF Code Example - TX and RX (fixed packet length =< FIFO size)
*
* Simple RF Link to Toggle Receiver's LED by pressing Transmitter's Button    
* Warning: This RF code example is setup to operate at either 868 or 915 MHz, 
* which might be out of allowable range of operation in certain countries.
* The frequency of operation is selectable as an active build configuration
* in the project menu. 
* 
* Please refer to the appropriate legal sources before performing tests with 
* this code example. 
* 
* This code example can be loaded to 2 CC430 devices. Each device will transmit 
* a small packet, less than the FIFO size, upon a button pressed. Each device will also toggle its LED 
* upon receiving the packet. 
* 
* The RF packet engine settings specify fixed-length-mode with CRC check 
* enabled. The RX packet also appends 2 status bytes regarding CRC check, RSSI 
* and LQI info. For specific register settings please refer to the comments for 
* each register in RfRegSettings.c, the CC430x513x User's Guide, and/or 
* SmartRF Studio.
* 
* M. Morales/D. Dang
* Texas Instruments Inc.
* June 2010
* Built with IAR v4.21 and CCS v4.1
******************************************************************************/


#include  <msp430.h>
#include <legacymsp430.h>

#include "RF_Toggle_LED_Demo.h"
#include "Temp.h"
#include "spi.h"
#include "stdio.h"

//#include  <intrinsics.h>
#include "LCD1x9.h"


#define  PACKET_LEN         (0x05)			// PACKET_LEN <= 61
#define  RSSI_IDX           (PACKET_LEN)    // Index of appended RSSI 
#define  CRC_LQI_IDX        (PACKET_LEN+1)  // Index of appended LQI, checksum
#define  CRC_OK             (BIT7)          // CRC_OK bit 
#define  PATABLE_VAL        (0x51)          // 0 dBm output 

extern RF_SETTINGS rfSettings;

unsigned char packetReceived;
unsigned char packetTransmit; 

unsigned char RxBuffer[PACKET_LEN+2];
unsigned char RxBufferLength = 0;

#define DEVCODE 25
#define TEMPTYPE  1



typedef union packet{
  struct packet_str{
    unsigned char src;
    unsigned char type;
    unsigned char data[PACKET_LEN-2];
  } pstruct;
  unsigned char rawdata[PACKET_LEN];  
}PKT_UN;

PKT_UN pkt1;

unsigned char PacketOut[PACKET_LEN];

const unsigned char TxBuffer[PACKET_LEN]= {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
volatile unsigned char buttonPressed = 0;
unsigned int i = 0; 

volatile unsigned char transmitting = 0; 
volatile unsigned char receiving = 0; 

volatile char dummychar;

//float tempc;

char message[20];
int count;
int modtcCx4;
int mtcC;
int mtcR;

#include "xbus.h"



void transmit_temp(unsigned char whole, unsigned char part){
  PacketOut[0] = DEVCODE;
  PacketOut[1] = TEMPTYPE;
  PacketOut[2] = whole;
  PacketOut[3] = part;
  PacketOut[4] = 0xff;
  Transmit(PacketOut, PACKET_LEN);

  /*
  pkt1.pstruct.src = DEVCODE;
  pkt1.pstruct.type = TEMPTYPE;
  pkt1.pstruct.data[0] = whole;
  pkt1.pstruct.data[1] = part;
  pkt1.pstruct.data[2] = 0xff;
  Transmit(pkt1.rawdata, sizeof(pkt1));
  */


  //  Transmit( (unsigned char*)TxBuffer, sizeof TxBuffer);         
  transmitting = 1;
  P1IFG = 0; 

  P1IE  |=  BIT1;
  _EINT(); 
}
unsigned char whole,part;

void measuretemp(){
  // spiinit();

  //spiinit();

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
  
  transmit_temp(whole,part);
  // tempc = (float)modtcCx4 * 0.25;
  count++;
  // sprintf(message,"%2d %2d.%2d",count,mtcC,mtcR);
  sprintf(message,"%2d.%02d C",modtcCx4/4,25*(modtcCx4%4));
  //sprintf(message,"%4.3f C",tempc);
  // LCD1x9_Initialize2();

  LCD1x9_Write(message);
  
  //spiinit();
}

void dummyfunc(){
  if(dummychar)
    dummychar = 0;
  //    __no_operation(); 

}

static unsigned int TACOUNT,TAEXIT;

#define TADIV 4
void initTA1(){
  TACOUNT = 0;
  TAEXIT = 0;
  TA1CCTL0 = CCIE + ID_3;// CCR0 interrupt enabled, input div 8
  TA1EX0 = TAIDEX_7 ; // div 8
  TA1CCR0 = 65000;
  //TA1CTL = TASSEL_2 + MC_2 + TACLR;         // SMCLK, contmode, clear TAR




  TA1EX0 = TAIDEX_2 ; // div 2

  TA1CCR0 = 100;
  TA1CTL = TASSEL_1 + MC_2 + TACLR;         // ACLK, contmode, clear TAR

}

int Temperature;

int main( void )
{  
  int i,j,on;
  WDTCTL = WDTPW + WDTHOLD; 

  sprintf(message,"");

  SetVCore(2);                            
  
  ResetRadioCore();     
  InitRadio();
  InitButtonLeds();

  P3OUT = 0x00;
  P3DIR = 0x00;
  LCD1x9_Initialize();

  ReceiveOn(); 
  receiving = 1; 
  // receiving = 0;
  transmitting = 0;
  _EINT(); 

  
  // get temperature
  //modtcCx4 = 0x77;
  spiinit();
  
  // modtcCx4 = modtc_celx4();
  //mtcC = modtcCx4/4;
  //mtcR =  25*(modtcCx4%4);

  initTA1();

  count = 0;
  while(1){

  __bis_SR_register(LPM3_bits  + GIE);

  //   for(i=0; i < 50; i++)
  //    __delay_cycles(10000)   ;
    measuretemp();
  }
  Temperature = tempC();
  
  dummyfunc();

 while (1)
  { 



    //  __bis_SR_register( LPM3_bits + GIE );   
    __no_operation(); 

    if (buttonPressed)                      // Process a button press->transmit 
    {
      //     P3OUT |= BIT6;                        // Pulse LED during Transmit                          
      buttonPressed = 0; 
      P1IFG = 0; 
      //P1IFG &= ~BIT1;

      ReceiveOff();
      receiving = 0; 
      Transmit( (unsigned char*)TxBuffer, sizeof TxBuffer);         
      transmitting = 1;
      P1IFG = 0; 

      P1IE  |=  BIT1;
      _EINT(); 

                              // Re-enable button press  
    }
    else if(!transmitting)
    {

      // if (!receiving){
      ReceiveOn();      
      receiving = 1; 
      // }
    }


  }

  return 0;
}


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


static unsigned int RF1AIVREG;
static unsigned char RFRXCSUM;

interrupt(CC1101_VECTOR) CC1101_ISR(void)
{
  // switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt 
  RF1AIVREG = RF1AIV;
  //  switch(RF1AIV,32)        // Prioritizing Radio Core Interrupt 
  switch(RF1AIVREG)        // Prioritizing Radio Core Interrupt 
  {
    case  0: break;                         // No RF core interrupt pending                                            
    case  2: break;                         // RFIFG0 
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10: break;                         // RFIFG4
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6          
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9
      if(receiving)			    // RX end of packet
      {
        // Read the length byte from the FIFO       
        RxBufferLength = ReadSingleReg( RXBYTES );               
        ReadBurstReg(RF_RXFIFORD, RxBuffer, RxBufferLength); 
        
        // Stop here to see contents of RxBuffer
        __no_operation(); 		   
        RFRXCSUM = RxBuffer[CRC_LQI_IDX];
        // Check the CRC results
         if(RxBuffer[CRC_LQI_IDX] & CRC_OK)  
          P1OUT ^= BIT0;                    // Toggle LED1      
      }
      else if(transmitting)		    // TX end of packet
      {
        RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt
        // P3OUT &= ~BIT6;                     // Turn off LED after Transmit               
        transmitting = 0; 
      }
      else while(1); 			    // trap 
      break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }  
  //  __bic_SR_register_on_exit(LPM3_bits);     
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

void InitRadio(void)
{
  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled 
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L; 
  PMMCTL0_H = 0x00; 
  
  WriteRfSettings(&rfSettings);
  
  WriteSinglePATable(PATABLE_VAL);
}

void Transmit(unsigned char *buffer, unsigned char length)
{
  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt
  
  WriteBurstReg(RF_TXFIFOWR, buffer, length);     
  
  Strobe( RF_STX );                         // Strobe STX   
}


void ReceiveOn(void)
{  
  RF1AIES |= BIT9;                          // Falling edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt 
  
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  Strobe( RF_SRX );                      
}

void ReceiveOff(void)
{
  RF1AIE &= ~BIT9;                          // Disable RX interrupts
  RF1AIFG &= ~BIT9;                         // Clear pending IFG

  // It is possible that ReceiveOff is called while radio is receiving a packet.
  // Therefore, it is necessary to flush the RX FIFO after issuing IDLE strobe 
  // such that the RXFIFO is empty prior to receiving a packet.
  Strobe( RF_SIDLE );
  Strobe( RF_SFRX  );                       
}


