#include "cc430f5137.h"
#include "spi_pin_def.h"
#include "g430_types.h"
#define LOW(port,bit)  port&=~bit
#define HIGH(port,bit)  port|=bit

void spi_init();
static void inline  spi_select(){
  LOW(STEOUT,STEBIT);
}

static void inline spi_deselect(){
  HIGH(STEOUT,STEBIT);

}
static void inline spi_delay(){
  __delay_cycles(10000);
}

static void inline spi_clocklow(){
  LOW(SCKOUT,SCKBIT);
}
static void inline spi_clockhigh(){
  HIGH(SCKOUT,SCKBIT);
}

static void inline spi_sdolow(){
  LOW(SDOOUT,SDOBIT);
}
static void inline spi_sdohigh(){
  HIGH(SDOOUT,SDOBIT);
}

void spi_delay();
unsigned char spi_read();
void spi_write_7bits(uint8 val);

