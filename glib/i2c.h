#include "cc430f5137.h"
#include "i2c_pin_def.h"
#include "g430_types.h"

#define LOW(port,bit)  port&=~bit
#define HIGH(port,bit)  port|=bit

void i2c_init();

void   i2c_sdalow();
void  i2c_sdahigh();
void  i2c_delay();
void  i2c_clocklow();
void  i2c_clockhigh();


void i2c_delay();
uint8 i2c_read();
uint8 i2c_read_nack();
void i2c_stop();
void i2c_start();
void i2c_repeatedstart();

