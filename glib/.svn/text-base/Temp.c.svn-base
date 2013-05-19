#include "cc430x513x.h"
#include <legacymsp430.h>

volatile long temp;
volatile int IntDegC;

int tempC()
{
  long templ;

  temp = 0;

  /* Initialize the shared reference module */ 
  REFCTL0 |= REFMSTR + REFVSEL_0 + REFON;    // Enable internal 1.5V reference
  
  /* Initialize ADC12_A */ 
  ADC12CTL0 = ADC12SHT0_8 + ADC12ON;		// Set sample time 
  ADC12CTL1 = ADC12SHP;                     // Enable sample timer
  ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC input ch A10 => temp sense 
  ADC12IE = 0x001;                          // ADC_IFG upon conv result-ADCMEMO
  
  __delay_cycles(128);                       // 35us delay to allow Ref to settle
                                            // based on default DCO frequency.
                                            // See Datasheet for typical settle
                                            // time.
  

  ADC12CTL0 |= ADC12ENC;
  
  ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start

  while(temp == 0){
        __no_operation();
  }
  templ = ((temp - 2438) * 410) / 4096;

  IntDegC = templ;
  return (int)templ;
}

interrupt(ADC12_VECTOR) ADC12ISR(void)
{
  switch(ADC12IV)
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
    temp = ADC12MEM0;                       // Move results, IFG is cleared
    // __bic_SR_register_on_exit(LPM4_bits);   // Exit active CPU
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
