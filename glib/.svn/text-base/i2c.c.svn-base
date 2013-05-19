
#include "i2c.h"




#define SELECT i2c_select()
#define DESELECT i2c_deselect()


#define I2CDELAY i2c_delay()


//#define CLKLOW    LOW(I2CI2CSDAOUT,I2CI2CSDABIT)
//#define CLKHIGH    HIGH(I2CI2CSDAOUT,I2CI2CSDABIT)
#define SDALOW i2c_sdalow()
#define SDAHIGH i2c_sdahigh()



#define CLKLOW i2c_clocklow();
#define CLKHIGH i2c_clockhigh()

#define READSDA() (I2CSDAIN & I2CSDABIT) ? 1 : 0 


 void   i2c_sdalow(){
  HIGH(I2CSDADIR,I2CSDABIT);
}

 void  i2c_sdahigh(){
  LOW(I2CSDADIR,I2CSDABIT);

}
 void  i2c_delay(){
  __delay_cycles(100);
}

 void  i2c_clocklow(){
  HIGH(I2CSCKDIR,I2CSCKBIT);
}
 void  i2c_clockhigh(){
  LOW(I2CSCKDIR,I2CSCKBIT);
}





// clock should be low on normal entry
 void i2c_stop(){
  I2CDELAY;
  CLKHIGH;
  I2CDELAY;
  SDAHIGH;
  I2CDELAY;

}
 void i2c_start(){  
  SDAHIGH;
  I2CDELAY;  
  SDALOW;
  I2CDELAY;  
  CLKLOW;
  I2CDELAY;

}
void i2c_repeatedstart(){  
  I2CDELAY;
  CLKHIGH;
  I2CDELAY;
  SDALOW;
  I2CDELAY;  
  CLKLOW;
  I2CDELAY;

}



void i2c_init(){
  LOW(I2CSDASEL,I2CSDABIT); //sda pin to io
  LOW(I2CSDADIR,I2CSDABIT); //sda is input
  LOW(I2CSDAREN,I2CSDABIT); //sda pull up disable
  LOW(I2CSDAOUT,I2CSDABIT); //sda output is set low

  LOW(I2CSCKSEL,I2CSCKBIT); //sck pin to io
  LOW(I2CSCKDIR,I2CSCKBIT); //sck is input
  LOW(I2CSCKREN,I2CSCKBIT); //sck pull up disable
  LOW(I2CSCKOUT,I2CSCKBIT); //sck output is set low
 
  //CLKLOW;
  //i2c_stop();
    
  // 
  
}

void i2c_write_dbg(){
  __delay_cycles(1);
}
void i2c_write_dbg2(){
  __delay_cycles(1);
}
int i2c_write(uint8 val){
  int i;
  int ack;
  uint8 byte = val;
  CLKLOW;
  I2CDELAY; 
  for (i = 0 ; i < 8 ; i++)
  {    
    CLKLOW;
    I2CDELAY;  
    if(byte & 0x80)
      SDAHIGH;
    else
      SDALOW;
    i2c_write_dbg();  
    I2CDELAY;
    CLKHIGH;
    I2CDELAY;
    byte = byte << 1;
  
  } 
  CLKLOW;
  I2CDELAY;
  SDAHIGH;
  I2CDELAY;
  CLKHIGH;
  I2CDELAY;
  ack = READSDA();
  i2c_write_dbg2();  
  CLKLOW;
  I2CDELAY;
  return ack;


}

void i2c_rd_dgb(){
  __delay_cycles(1);
}
void i2c_rd_dgb1(){
  __delay_cycles(1);
}

unsigned char i2c_read(){
  int i;
  unsigned char val = 0; 
  SDAHIGH;
  for (i=7; i>=0; i--)
  {    
    CLKLOW;
    i2c_rd_dgb1();

    I2CDELAY;    
    CLKHIGH;    
    I2CDELAY;
    if(READSDA())
      val +=  1 << i;
    i2c_rd_dgb();
    I2CDELAY;
  } 
  CLKLOW;
  I2CDELAY;
  SDALOW;
  I2CDELAY;
  CLKHIGH;
  I2CDELAY;
  CLKLOW;
  SDAHIGH;
  I2CDELAY;
  return val;
}

unsigned char i2c_read_nack(){
  int i;
  unsigned char val = 0; 
  SDAHIGH;
  for (i=7; i>=0; i--)
  {    
    CLKLOW;
    i2c_rd_dgb1();

    I2CDELAY;    
    CLKHIGH;    
    I2CDELAY;
    if(READSDA())
      val +=  1 << i;
    i2c_rd_dgb();
    I2CDELAY;
  } 
  CLKLOW;
  I2CDELAY;
  SDAHIGH;
  I2CDELAY;
  CLKHIGH;
  I2CDELAY;
  CLKLOW;
  SDAHIGH;
  I2CDELAY;
  return val;
}
