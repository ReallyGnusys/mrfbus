#include "cc430x513x.h"
#include <legacymsp430.h>
#include "g430_types.h"
#include "xbus.h"
#include "xb_usr_structs.h"

#define ADC_CHANNELS 2

#define TEMP_CHANN 0
#define VCC_CHANN  1
static uint16 adc_results[ADC_CHANNELS];  // vcc div 2 and temp

static uint16 adc_channel;
static uint16 channel_send[ADC_CHANNELS]; 
uint8 _adc_msg[6];



static int16 set_channel(uint16 channel, int sendmsg)
{
  if(channel >=  ADC_CHANNELS)
    return -1;
  adc_channel = channel;
  adc_results[adc_channel] = 0;

  channel_send[adc_channel] = sendmsg;
  if ( channel == 0 )
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC input ch A10 => temp sense 
  else
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_11;  // ADC input ch A11 => (Vcc-Vss)/2 

  
  return 0;
}



uint16 adc_temp(int sendmsg){

  /* Initialize REF module */
  // Enable 1.5V shared reference, disable temperature sensor to save power
  REFCTL0 = REFMSTR+REFVSEL_0+REFON; 

  ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
  ADC12CTL1 = ADC12SHP;                     // Use sampling timer

  set_channel(TEMP_CHANN,sendmsg);
  
  //  ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC input ch A10 => temp sense 

  __delay_cycles(75*8);                       // 75 us delay @ 8MHz /~1MHz

  ADC12IE = 0x01;                           // Enable interrupt
  ADC12CTL0 |= ADC12ENC;
  //P2SEL |= BIT0;                            // P2.0 ADC option select
  //P1DIR |= BIT0;                            // P1.0 output


    ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion  

    
    /*   
  
    __bis_SR_register(LPM0_bits + GIE);     // LPM0, ADC12_ISR will force exit
    __no_operation();                       // For debugger
    ADC12IE = 0x0;                           // disable interrupt
  ADC12CTL0 = 0;         // Sampling time, ADCOFF
  REFCTL0 = REFTCOFF; 
    */
    return 0 ; //adc_results[adc_channel];

}


uint16 adc_supply(int sendmsg){

  /* Initialize REF module */
  // Enable 2.5V shared reference, disable temperature sensor to save power
  REFCTL0 |= REFMSTR+REFVSEL_2+REFON+REFTCOFF; 

  ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
  ADC12CTL1 = ADC12SHP;                     // Use sampling timer

  set_channel(VCC_CHANN,sendmsg);
  
  //  ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC input ch A10 => temp sense 

  __delay_cycles(75*8);                       // 75 us delay @ 8MHz /~1MHz

  ADC12IE = 0x01;                           // Enable interrupt
  ADC12CTL0 |= ADC12ENC;
  //P2SEL |= BIT0;                            // P2.0 ADC option select
  //P1DIR |= BIT0;                            // P1.0 output


    ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion    

    /* 
    __bis_SR_register(LPM0_bits + GIE);     // LPM0, ADC12_ISR will force exit
    __no_operation();                       // For debugger
    ADC12IE = 0x0;                           // disable interrupt
    ADC12CTL0 = 0;         // Sampling time, ADCOFF
    REFCTL0 = REFTCOFF; 
    */

    return 0 ;// adc_results[adc_channel];

}



uint16 adc_temp_result(){
  return adc_results[TEMP_CHANN];
}
uint16 adc_vcc_result(){
  return adc_results[VCC_CHANN];
}
uint16 adc_measure_supply(){
  adc_supply(0);
  while (adc_vcc_result() == 0)
    __delay_cycles(100);
  return adc_vcc_result();

}
interrupt(ADC12_VECTOR) ADC12ISR(void)
{
  uint16 *uptr = (uint16 *)&_adc_msg[2];

  switch(ADC12IV)
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0

    ADC12IE = 0x0;                           // disable interrupt
    ADC12CTL0 = 0;         // Sampling time, ADCOFF
    REFCTL0 = REFTCOFF; 
    

    adc_results[adc_channel] = ADC12MEM0;

    
    break;
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}
