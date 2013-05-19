#include "mpl115a.h"

// routines to support freescale mpl115a barometer IC

// device memory map

#define PADC_MSB  0x00
#define PADC_LSB  0x01
#define TADC_MSB  0x02
#define TADC_LSB  0x03
#define A0_MSB    0x04
#define A0_LSB    0x05
#define B1_MSB    0x06
#define B1_LSB    0x07
#define B2_MSB    0x08
#define B2_LSB    0x09
#define C12_MSB   0x0A
#define C12_LSB   0x0B

#define CONVERT   0x12

// coefficient bytes
#define COEFF_BYTES 0x04
#define NUM_COEFF_BYTES 8

#define A0_MSB    0x04
#define A0_LSB    0x05
#define B1_MSB    0x06
#define B1_LSB    0x07
#define B2_MSB    0x08
#define B2_LSB    0x09
#define C12_MSB   0x0A
#define C12_LSB   0x0B

// shutdown pin def

#define SHDOUT P3OUT
#define SHDDIR P3DIR
#define SHDBIT  BIT3
#define SHDREN P3REN

#define MPL_READ 0x80
#define MPL_WRITE 0

uint8 mpl_coeffs[NUM_COEFF_BYTES];

static uint8 mpl_results[4]; 

#define READ     0x80
#define WRITE    0x0

int mpl115a_copy_coeffs(uint8 *buffer){
  int i;
  for ( i = 0 ; i < NUM_COEFF_BYTES ; i++){
    buffer[i] = mpl_coeffs[i];
  }
}
int mpl115a_copy_results(uint8 *buffer){
  int i;
  for ( i = 0 ; i < 4 ; i++){
    buffer[i] = mpl_results[i];
  }
}


int mpl115a_read_coeffs(){
  int i;
  spi_select();
  spi_delay();
  for ( i = 0 ; i < NUM_COEFF_BYTES ; i++){
    spi_write(MPL_READ | (COEFF_BYTES + i) * 2);
    mpl_coeffs[i] = spi_read();

  }
  spi_read(); // extra read - see datasheet
  spi_deselect();
  return 0;
}
void mpl115a_convert(){
  int i;
  spi_select();
  spi_delay();
  spi_write(CONVERT*2);
  spi_write(0); // extra write - see datasheet
  spi_delay();  
  spi_deselect();

  // need 3 ms here
  for (i = 0 ; i < 4 ; i++)
    spi_delay();  


  for (i = 0 ; i < 4 ; i++)
    {
    spi_write((PADC_MSB + i)*2);
    mpl_results[i] = spi_read();
    }
  spi_write(0); // extra transaction - see datasheet

  
  /*
  spi_delay();  
  spi_delay();  
  */
  
}
void mpl115a_wake(){
  int i;
  HIGH(SHDOUT,SHDBIT);//set ste high
 
  // wait
  for ( i = 0 ; i < 100 ; i++)
    spi_delay();


}

// 
int mpl115a_init(){
  int val,i;
  mpl115a_wake();
 
  mpl115a_read_coeffs();

  return 0;
}
