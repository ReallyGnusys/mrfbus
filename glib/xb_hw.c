#include "xbus.h"
#include "xb_sys.h"

extern  XB_PKT_STATS  _xb_stats;

void xb_hw_idle(){
  Strobe(RF_SIDLE);
  Strobe(RF_SFRX);// flush rx fifo
}

int xb_hw_rd_rx_fifo(uint8 *buff,RX_PKT_INFO *pkt_info){
  int len,i,j;
  uint8 linktoks[2];
  len = ReadSingleReg( RXBYTES ); 

  if ( len < 3 )  // at least 2 are LQI and RSSI
    return -1;
  if ( len == 0 )
    return -1;

  while (!(RF1AIFCTL1 & RFINSTRIFG));    // wait for RFINSTRIFG
  RF1AINSTR1B = (RF_RXFIFORD | RF_REGRD);          

  for (i = 0; i < (len - 2); i++)
    {
      while (!(RFDOUTIFG&RF1AIFCTL1));        // wait for RFDOUTIFG
      buff[i] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
                                              // Also initiates auo-read for next DOUT byte
    }
  
  while (!(RFDOUTIFG&RF1AIFCTL1));        // wait for RFDOUTIFG
  linktoks[0] = RF1ADOUT1B;    
   
  
  while (!(RFDOUTIFG&RF1AIFCTL1));        // wait for RFDOUTIFG
  linktoks[1] = RF1ADOUT0B;             // Store the last DOUT from Radio Core  

  
  pkt_info->csumok = linktoks[1] & 0x80;
  pkt_info->linktoks[0] = linktoks[0];  // RSSI
  pkt_info->linktoks[1] = linktoks[1] & 0x7f; // LQI
  
  return 0;
  /*
        // Stop here to see contents of _xb_rx_buff
  __no_operation();
  RFRXRSSI = *(((uint8 *)(&_xb_rx_buff)) + rx_buff_len-2);	
  RFRXCSUM = *(((uint8 *)(&_xb_rx_buff)) + rx_buff_len-1);
  */
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
	xb_rx_intr();
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
  //  __bic_SR_register_on_exit(LPM3_bits);     
  //  xb_receive_on(); // desperate measures
  // Strobe( RF_SRX );                      

}

