#include "i2c.h"

// routines to access SHT21 humidity sensor

#define SHT21_READ  0x81
#define SHT21_WRITE  0x80


#define TRIG_TEMP   0xF3
#define TRIG_RH     0xF5

static sht21_errors;
void sht21_error(){
  sht21_errors++;
}

void sht21_delay(){
  __delay_cycles(20000);
}

int sht21_start_meas(uint8 cmd){
  i2c_start();
  if(i2c_write(SHT21_WRITE)){
    sht21_error();
    i2c_stop();
    return -1;
  }
  if(i2c_write(cmd)) {
    sht21_error();
    i2c_stop();
    return -2;
  }

  i2c_stop();
  return 0;
}
int sht21_start_temp(){
  return sht21_start_meas(TRIG_TEMP);
}

int sht21_start_rh(){
  return sht21_start_meas(TRIG_RH);
}


void sht21_debug(uint16 val){
  __delay_cycles(1);
}

static int sht21_readcnt;

int sht21_read_measurement(uint16 *readval){
  // non-blocking
  // returns 0 if reading was available and written to readval
  uint8 b1,b2,b3;
  uint16 val;
  int ack = 1;
  //sht21_delay();
  sht21_readcnt = 0;
  while (ack == 1){
    i2c_start();
    ack = i2c_write(SHT21_READ);
    if(ack){
      sht21_readcnt++;
      i2c_stop();
      sht21_delay();
      if (sht21_readcnt > 20){
	sht21_error();
	return -1;
      }
    }
  }
  b1 = i2c_read();
  b2 = i2c_read();
  b3 = i2c_read();
  val = (uint16) (b1 << 8)  + (uint16)b2;
  // checksum in b3
  *readval = val;
  sht21_debug(val);
  i2c_stop();
  return 0;
  }
  
