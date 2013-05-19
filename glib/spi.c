
#include "spi.h"




#define SELECT spi_select()
#define DESELECT spi_deselect()


#define SPIDELAY spi_delay()





//#define CLKLOW    LOW(SCKOUT,SCKBIT)
//#define CLKHIGH    HIGH(SCKOUT,SCKBIT)

#define CLKLOW spi_clocklow();
#define CLKHIGH spi_clockhigh()
#define SDOLOW spi_sdolow();
#define SDOHIGH spi_sdohigh()

#define READSDA() (SDAIN & SDABIT) ? 1 : 0 
void spi_init(){
  LOW(SDADIR,SDABIT); //sda is input
  HIGH(SDAREN,SDABIT); //sda pull up enable
  HIGH(SDAOUT,SDABIT); //sda pull up enable
  HIGH(SCKDIR,SCKBIT);//sck is output
  HIGH(STEDIR,STEBIT);//ste is output
  HIGH(STEOUT,STEBIT);//set ste high
}


unsigned char spi_read(){
  int i;
  unsigned char val = 0; 
  SDOLOW;
  SPIDELAY;    
  for (i=7; i>=0; i--)
  {    
    CLKLOW;
    SPIDELAY;    
    if(READSDA())
      val +=  1 << i;
    CLKHIGH;
    SPIDELAY;
  } 
 return val;
}

void spi_write(uint8 val){
  int i;
  for (i=7; i>=0; i--)
  {    
    CLKLOW;
    SPIDELAY;
    if (val & 0x80)
      SDOHIGH;
    else
      SDOLOW;
    SPIDELAY;
    val = val << 1;
    CLKHIGH;
    SPIDELAY;
  } 
}
