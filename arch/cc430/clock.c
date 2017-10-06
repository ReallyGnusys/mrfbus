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

//#define MCLK_8MHZ_DCO
//MCLK_XT2
#include  <msp430.h>
#include <legacymsp430.h>

// MCLK_8MHZ_DCO

void init_clock(void){
  int x;

#ifdef CLOCK_DEFAULT
  return;
#endif


  P5SEL = 3; // enable XT1 oscillator

#ifdef MCLK_XT2
  /* should be selecting MCLK ACLK SMCLK = XT2 */
  UCSCTL6 &= ~XT2OFF;                       // Enable XT2
  UCSCTL3 |= SELREF_2;                      // FLLref = REFO
                                            // Since LFXT1 is not used,
                                            // sourcing FLL with LFXT1 can cause
                                            // XT1OFFG flag to set

  // ACLK=REFO,SMCLK=DCO,MCLK=DCO
  UCSCTL4 = SELA__REFOCLK + SELS__DCOCLKDIV + SELM__DCOCLKDIV;
  //UCSCTL4 = SELA__DCOCLKDIV + SELS__DCOCLKDIV + SELM__DCOCLKDIV;
 
  // Loop until XT1,XT2 & DCO stabilizes
  do
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
                                            // Clear XT2,DCO fault flags
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
                                            
  UCSCTL4 |= SELA__REFOCLK + SELS__XT2CLK + SELM__XT2CLK;   // SMCLK=MCLK=XT2
   // UCSCTL4 |= SELA__XT2CLK + SELS__XT2CLK + SELM__XT2CLK;   // SMCLK=MCLK=XT2

#endif


#ifdef MCLK_16MHZ_DCO

  UCSCTL3 = SELREF__XT1CLK + FLLREFDIV_0;

 // UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
 // UCSCTL4 |= SELA_0;                        // Set ACLK = REFO
 UCSCTL4 |=  SELA__XT1CLK ;//+ SELM__XT1CLK + SELS__XT1CLK;
 __bis_SR_register(SCG0);                  // Disable the FLL control loop
 UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
 UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
 UCSCTL2 = FLLD_1 + 256;                   // Set DCO Multiplier for 8MHz
                                            // (N + 1) * FLLRef = Fdco
                                            // (249 + 1) * 32768 = 8MHz
                                            // Set FLL Div = fDCOCLK/2
 __bic_SR_register(SCG0);                  // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
  //  __delay_cycles(250000);
 for(x=0 ; x<255;x++)
    __delay_cycles(1000);
  // Loop until XT1,XT2 & DCO fault flag is cleared
 do
   {
     UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
     // Clear XT2,XT1,DCO fault flags
     SFRIFG1 &= ~OFIFG;                      // Clear fault flags
   }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag


 // UCSCTL4 = SELA__REFOCLK + SELS__DCOCLKDIV + SELM__DCOCLKDIV;
 UCSCTL4 = SELA__XT1CLK + SELS__DCOCLKDIV + SELM__DCOCLK;

#endif


  
#ifdef MCLK_8MHZ_DCO

  //UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO

  UCSCTL3 = SELREF__XT1CLK + FLLREFDIV_0;

 // UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
 // UCSCTL4 |= SELA_0;                        // Set ACLK = REFO
 UCSCTL4 |=  SELA__XT1CLK ;//+ SELM__XT1CLK + SELS__XT1CLK;
 __bis_SR_register(SCG0);                  // Disable the FLL control loop
 UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
 UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
 UCSCTL2 = FLLD_1 + 255;                   // Set DCO Multiplier for 8MHz
                                            // (N + 1) * FLLRef = Fdco
                                            // (249 + 1) * 32768 = 8MHz
                                            // Set FLL Div = fDCOCLK/2
 __bic_SR_register(SCG0);                  // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
  //  __delay_cycles(250000);
 for(x=0 ; x<255;x++)
    __delay_cycles(1000);
  // Loop until XT1,XT2 & DCO fault flag is cleared
 do
   {
     UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
     // Clear XT2,XT1,DCO fault flags
     SFRIFG1 &= ~OFIFG;                      // Clear fault flags
   }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag


 // UCSCTL4 = SELA__REFOCLK + SELS__DCOCLKDIV + SELM__DCOCLKDIV;
 UCSCTL4 = SELA__XT1CLK + SELS__DCOCLKDIV + SELM__DCOCLKDIV;

#endif
#ifdef MCLK_4MHZ_DCO

  //UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO

  UCSCTL3 = SELREF__XT1CLK + FLLREFDIV_0;

 // UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
 // UCSCTL4 |= SELA_0;                        // Set ACLK = REFO
 UCSCTL4 |=  SELA__XT1CLK ;//+ SELM__XT1CLK + SELS__XT1CLK;
 __bis_SR_register(SCG0);                  // Disable the FLL control loop
 UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
 UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
 UCSCTL2 = FLLD_1 + 127;                   // Set DCO Multiplier for 8MHz
                                            // (N + 1) * FLLRef = Fdco
                                            // (249 + 1) * 32768 = 8MHz
                                            // Set FLL Div = fDCOCLK/2
 __bic_SR_register(SCG0);                  // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
  //  __delay_cycles(250000);
 for(x=0 ; x<255;x++)
    __delay_cycles(1000);
  // Loop until XT1,XT2 & DCO fault flag is cleared
 do
   {
     UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
     // Clear XT2,XT1,DCO fault flags
     SFRIFG1 &= ~OFIFG;                      // Clear fault flags
   }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag


 // UCSCTL4 = SELA__REFOCLK + SELS__DCOCLKDIV + SELM__DCOCLKDIV;
 UCSCTL4 = SELA__XT1CLK + SELS__DCOCLKDIV + SELM__DCOCLKDIV;

#endif

#ifdef MCLK_8MHZ_DCO_SMCLK

 UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO
 //UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
 UCSCTL4 = SELA__REFOCLK + SELS__DCOCLKDIV + SELM__DCOCLKDIV;
  __bis_SR_register(SCG0);                  // Disable the FLL control loop
  UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
  UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
  UCSCTL2 = FLLD_1 + 249;                   // Set DCO Multiplier for 8MHz
                                            // (N + 1) * FLLRef = Fdco
                                            // (249 + 1) * 32768 = 8MHz
                                            // Set FLL Div = fDCOCLK/2
  __bic_SR_register(SCG0);                  // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
  //  __delay_cycles(250000);
 for(x=0 ; x<251;x++)
    __delay_cycles(1000);
  // Loop until XT1,XT2 & DCO fault flag is cleared
  do
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
                                            // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
#endif
}
