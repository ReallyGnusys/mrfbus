#include "modtc.h"

// read olimex modtc thermocouple sensor
// returns reading in units of 0.25C  
int modtc_celx4(){
  int val;
  spi_select();
  spi_delay();
  val = spi_read();
  val <<= 8;
  val |= spi_read();
  spi_delay();
  spi_deselect();
  spi_delay();
  if(val & 0x4)  //no thermocouple
    return MODTC_NOTC;
  val >>= 3; // discard 3 lsbs
  return val; 
}
