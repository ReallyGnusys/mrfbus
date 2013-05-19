#include "mpl115a.h"

// routines to support freescale mpl115a barometer IC

#define MPL115_WRITE  0xC0
#define MPL115_READ  0xC1

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


// reset pin def

#define RSTOUT P2OUT
#define RSTDIR P2DIR
#define RSTBIT  BIT3
#define RSTREN P2REN


// shutdown pin def

#define SHDOUT P2OUT
#define SHDDIR P2DIR
#define SHDBIT  BIT2
#define SHDREN P2REN
#define SHDDS P2DS


#define MPL_READ 0x80
#define MPL_WRITE 0

uint8 mpl_coeffs[NUM_COEFF_BYTES];

static uint8 mpl_results[4]; 

#define READ     0x80
#define WRITE    0x0

static int  mpl115_errors;
void mpl115_error(){
  mpl115_errors++;
}
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

int mpl115a2_cmd_read(uint8 addr,uint8 len, uint8 *buff){
  uint8 i;
  //i2c_stop();
  i2c_start();
  if(i2c_write(MPL115_WRITE)){
    mpl115_error();
    i2c_stop();
    return -21;
  }
  if(i2c_write(addr)) {
    mpl115_error();
    i2c_stop();
    return -22;
  } 
  i2c_repeatedstart();

  if(i2c_write(MPL115_READ)){
    mpl115_error();
    i2c_stop();
    return -23;
  }
  //  i2c_stop();
  for ( i = 0 ; i < len-1 ; i++){
    buff[i] = i2c_read();
  }
  buff[i] = i2c_read_nack();
 
  
  i2c_stop();
  return 0;
}
int mpl115a_read_coeffs(){

  return mpl115a2_cmd_read(A0_MSB,NUM_COEFF_BYTES,mpl_coeffs);

}

int mpl115a_read_results(){

  return mpl115a2_cmd_read(PADC_MSB,4,mpl_results);

}
int mpl115a_start_convert(){
  i2c_start();
  if(i2c_write(MPL115_WRITE)){
    mpl115_error();
    i2c_stop();
    return -2;
  }
  if(i2c_write(CONVERT)) {
    mpl115_error();
    i2c_stop();
    return -3;
  } 

  if(i2c_write(0)) {
    mpl115_error();
    i2c_stop();
    return -4;
  } 
  i2c_stop();
  return 0;
  

  
}



void mpl115a_wake(){
  HIGH(SHDOUT,SHDBIT);//set ste high
 
}
void mpl115a_sleep(){
  LOW(SHDOUT,SHDBIT);//set ste high
 
}
void mpl115a_reset(){
  LOW(RSTOUT,RSTBIT);//set ste high
 
}
void mpl115a_active(){
  HIGH(RSTOUT,RSTBIT);//set ste high
 
}

void mpl115_delay(int del){

  int i;
  for ( i = 0 ; i < del ; i++ )
    __delay_cycles(1000);
}

void mpl115a_wake_delay(){
  mpl115_delay(100);
}

void mpl115a_conv_delay(){
  mpl115_delay(25);
}
// 
void _mpl115a_pin_init(){
  LOW(SHDOUT,SHDBIT);//
  LOW(RSTOUT,RSTBIT);//

  HIGH(SHDOUT,SHDBIT); // try high shutdown


  HIGH(SHDDIR,SHDBIT);//set shutdown pin to output
  HIGH(RSTDIR,RSTBIT);//set shutdown pin to output


  
  HIGH(SHDDS,SHDBIT); // try high ds

}
int mpl115a_init(){
  mpl115_errors = 0;
  _mpl115a_pin_init();
  mpl115a_reset();
  mpl115_delay(10);
  mpl115a_active();

  mpl115a_wake();
  mpl115a_wake_delay();
  mpl115a_read_coeffs();

  return 0;
}
