#include "xbus.h"
#include "xb_sys.h"
#include "xb_if.h"
#include "xb_buff.h"


#define xb_alloc()  xb_alloc_if(RF1)
#define xb_buff_loaded(buff,em)  xb_buff_loaded_if(RF1,buff,em)


static XB_IF_STATS  _xb_stats;


void xb_hw_idle(){
  Strobe(RF_SIDLE);
  Strobe(RF_SFRX);// flush rx fifo
}

void _xb_receive_enable(void){
  RF1AIES |= BIT9;                          // Falling edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt 
  
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  // _xb_state = XB_ST_RX;
  // Strobe( RF_SFRX  );    
  Strobe( RF_SIDLE );
  Strobe(RF_SFRX);
  Strobe( RF_SRX );      
}


// emergency buff used to record packet header if main alloc fails 
EBUFF ebuff;

int _xb_hw_rd_rx_fifo(){
  uint8 i,len;
  uint8 *buff;
  uint8 em = false,lenerr = false;

  len = ReadSingleReg( RXBYTES ); 

  if ( len < sizeof(XB_PKT_HDR) + 2) { // at least 2 are LQI and RSSI
    _xb_stats.under++;
    lenerr = true;
    
  }
 
  else if  (len > _XB_BUFFLEN -1){ // packet too long
     _xb_stats.over++;
    lenerr = true;
  }
  else{
    buff = xb_alloc();

    if ( buff == NULL){  // emergency buffer used to collect header + toks only
      em = true;
      ebuff.len = len;
      buff = (uint8 *)&ebuff;
    }else {
      buff[0] = len;
    }
  }
  
  while (!(RF1AIFCTL1 & RFINSTRIFG));    // wait for RFINSTRIFG
  RF1AINSTR1B = (RF_RXFIFORD | RF_REGRD);          

  for (i = 0; i < len; i++)
    {
      while (!(RFDOUTIFG&RF1AIFCTL1));        // wait for RFDOUTIFG
      if (lenerr)
	ebuff.hdr[0] = RF1ADOUT1B;  // clear fifo regardless of len err	
      else if ( em == false)
	buff[i+1] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
      else if ( i < sizeof(XB_PKT_HDR))  // if em == true  save hdr and link toks to embuff 
	ebuff.hdr[i] = RF1ADOUT1B;
      else if ( i >= (len - 2))
	ebuff.tok[i - (len -2)] = RF1ADOUT1B;	  
                                              // Also initiates auto-read for next DOUT byte
    }

  if (lenerr){ // re-enable reception
    _xb_receive_enable();
    return;
  }

   
  xb_buff_loaded(buff,em);
  
  


  

  return 0;
 
}
int  _xb_hw_wr_tx_fifo(int len , uint8 *buff){
  int i;
  if ((len < 1 ) || ( len > 255 ))
    return -1;
  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // clear interrupt flag
  RF1AIE |= BIT9;                           // enable TX end-of-packet interrupt  
  
  //Strobe( RF_STX );                       // Strobe STX   
  /* total bodging - what is this for? */

  Strobe( RF_SIDLE );
  Strobe( RF_SFTX  );  
  Strobe( RF_SNOP );                         // Strobe STX   
  
  while (!(RF1AIFCTL1 & RFINSTRIFG));        // wait for tx ready
  
  RF1AINSTRW = ((RF_TXFIFOWR | RF_REGWR)<<8 ) + (uint8)len; // write reg addr + len
  for (i = 0; i < len; i++){
    while (!(RFDINIFG & RF1AIFCTL1));       
    RF1ADINB = buff[i];                   

  }
  i = RF1ADOUTB;                            // reset RFDOUTIFG flag status byte  

  Strobe( RF_STX );                         // Strobe STX   
  return 0; 
}

void inline xb_hw_disable_tx_eop(void){
RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt
}

static unsigned int RF1AIVREG;

interrupt(CC1101_VECTOR) CC1101_ISR(void)
{

  XB_STATE xbst = _xb_curr_state();
  int xbreceiving = _xb_receiving();
  int xbtransmitting = _xb_transmitting();
  int rx_ournet = 0;
  int rx_crcok = 0;
  //RSTAT = GetRF1ASTATB();
  
  // switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt 
  RF1AIVREG = RF1AIV;
  //  switch(RF1AIV,32)        // Prioritizing Radio Core Interrupt 
  switch(RF1AIVREG)        // Prioritizing Radio Core Interrupt 
  {
    case  0: break;                         // No RF core interrupt pending                                            
    case  2: break;                         // RFIFG0 
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10: break;                         // RFIFG4
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6          
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9
      if(xbreceiving)			    // RX end of packet
      {
	_xb_hw_rd_rx_fifo();
      }
      else if(xbtransmitting)		    // TX end of packet
      {
	xb_tx_intr();

      }

      // else while(1); 			    // trap - sometimes gets in here - 
      break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }  
 
}

