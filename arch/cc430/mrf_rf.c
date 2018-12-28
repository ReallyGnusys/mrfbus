/******************************************************************************
*
* Copyright (c) 2012-16 Gnusys Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************************/

#include "mrf_sys.h"
#include "mrf_if.h"
#include "mrf_buff.h"
#include  <msp430.h>
#include <legacymsp430.h>
#include "_mrf_rf1.h"
#include "mrf_route.h"
#include "mrf_data.h"   // for MRF_CHANN_OFFSET
//#define mrf_buff_loaded(buff)  mrf_buff_loaded_if(RF0,buff)

//#define mrf_alloc() mrf_alloc_if(RF0)
int rf_if_send_func(I_F i_f, uint8 *buff);
int mrf_rf_init(I_F i_f);
int mrf_rf_clear(I_F i_f);

extern const MRF_IF_TYPE mrf_rf_cc_if = {
 tx_del : 4,
 ack_del: 2,
 funcs : { send : rf_if_send_func,
           init : mrf_rf_init,
           clear : mrf_rf_clear,
           buff : NULL}
};

static struct rf_stats_t {
  uint32 ints;
  uint32 rdint;
  uint32 wrint;
  uint32 ccaint;
  uint32 nodefint;
  uint32 rdbytes;
  uint32 rdnothing;
  uint32 rdlenerr;
  uint32 rx_strobe_err;
  uint32 tx_strobe_err;
  uint32 oddint;
  uint32 channel_not_clear;
  uint32 channel_was_clear;
  uint32 clear_called;
  int transmitting;
  uint16 lastnodef;
  uint8 laststat;
  uint8 rxstat;
  uint8 txstat;
  uint8 pktstatus;

} rf_stats;

int _Dbg_sresp(){
  return 2;
}

int _Dbg_dinfo(){
  return 3;
}


int  _xb_hw_wr_tx_fifo(int len , uint8 *buff);

int rf_if_send_func(I_F i_f, uint8 *buff){
  //const MRF_IF *mif = mrf_if_ptr(i_f);
  if (buff[4] == 2) // resp
    _Dbg_sresp();
  if (buff[4] == 3) // dev info
    _Dbg_dinfo();


  _xb_hw_wr_tx_fifo((int)buff[0] , buff);

  return 0;
}

extern RF_SETTINGS rfSettings;

static void _mrf_radio_active_lpm3(){
  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled
  PMMCTL0_H = 0xA5;

  // for light sleep , allow RF and peripherals to keep clocks requested during LPM3
#ifdef SLEEP_deep
  PMMCTL0_L &= ~PMMHPMRE_L;
#else
  PMMCTL0_L |= PMMHPMRE_L;
#endif
  PMMCTL0_H = 0x00;


}

uint8 mcsm1(){
  return  ReadSingleReg(MCSM1);
}
static void _mrf_init_radio()
{
  uint8 mcsm1;
  WriteRfSettings(&rfSettings);
  WriteSinglePATable(PATABLE_VAL);

  WriteSingleReg(CHANNR,RF_CHANNEL_NUM+MRF_CHANN_OFFSET); // set CCA_MODE=0x1 (RSSI below threshold)

  mcsm1 = ReadSingleReg(MCSM1);
  WriteSingleReg(MCSM1,(mcsm1 & ~0x30) | 0x10); // set CCA_MODE=0x1 (RSSI below threshold)
  Strobe( RF_SIDLE );
}


int _rf_rx_dbg(uint8_t i){
  rf_stats.rx_strobe_err++;
  return 22;
}


int _rf_tx_dbg(uint8_t i){
  rf_stats.tx_strobe_err++;
  return 22;
}

int _mrf_receive_reenable(){
  uint8_t stb,i;
  Strobe(RF_SFRX);
  stb = Strobe( RF_SRX );
  i = 0;
  while ((stb != 0x1f) && ( i < 50)){
    stb = Strobe( RF_SRX );
    i++;
  }

  if (stb != 0x1f)
    _rf_rx_dbg(i);
  /*
  RF1AIES |= BIT9;                          // neg edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt
  */
  RF1AIE  &= ~BIT9;                          // Disable the interrupt
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIES |= BIT9;                          // Pos edge of RFIFG10
  RF1AIE  |= BIT9;                          // Enable the interrupt

  return 0;


}

int _mrf_receive_enable(void){
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  // _xb_state = XB_ST_RX;
  // Strobe( RF_SFRX  );
  Strobe( RF_SIDLE );

  return _mrf_receive_reenable();

}

int _mrf_initial_receive_enable(void){
  uint8_t stb,i;

  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  // _xb_state = XB_ST_RX;
  // Strobe( RF_SFRX  );
  Strobe( RF_SIDLE );
  Strobe(RF_SFRX);
  stb = Strobe( RF_SRX );
  i = 0;
  while ((stb != 0x1f) && ( i < 250)){
    stb = Strobe( RF_SRX );
    i++;
  }

  if (stb != 0x1f)
    _rf_rx_dbg(i);
  /*
  RF1AIES |= BIT9;                          // Pos edge of RFIFG10
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt
  */
  RF1AIES |= BIT9;                          // Pos edge of RFIFG10  - neg edge???
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt

  return 0;

}

int _clear_cca_fg(){
  RF1AIFG &= ~BITC;                         // Clear a pending interrupt
  return RF1AIFG;
}

uint16_t rf1ain_cca;
int _channel_is_clear(){
  rf1ain_cca = RF1AIN;
  if ((rf1ain_cca & BITC) == 0 )
    return 0;
  else
    return 1;
}

int mrf_rf_clear(I_F i_f){  // i_f ignored - only RF0 on cc arch
  rf_stats.clear_called++;
  RF1AIE  &= ~BITC;                          // Disable the interrupt
  RF1AIFG &= ~BITC;                         // Clear a pending interrupt
  RF1AIES |= BITC;                          // Neg edge of RFIFG12
  RF1AIE  |= BITC;                          // Enable the interrupt
  return _channel_is_clear();

}

int mrf_rf_init(I_F i_f){

  rf_stats.ints = 0;
  rf_stats.rdint = 0;
  rf_stats.wrint = 0;
  rf_stats.ccaint = 0;
  rf_stats.nodefint = 0;
  rf_stats.rdbytes = 0;
  rf_stats.rdnothing = 0;
  rf_stats.rdlenerr = 0;
  rf_stats.lastnodef = 0;
  rf_stats.laststat = 0;
  rf_stats.rx_strobe_err = 0;
  rf_stats.tx_strobe_err = 0;
  rf_stats.channel_not_clear = 0;
  rf_stats.channel_was_clear = 0;
  rf_stats.oddint = 0;
  rf_stats.clear_called = 0;
  _mrf_radio_active_lpm3();

  ResetRadioCore();
  _mrf_init_radio();

  WriteSingleReg(IOCFG0,0x1e);  // RSSI_VALID
  WriteSingleReg(IOCFG1,0x6 | 0x40);  // posedge SYNC word send , negedge TX_FIFO underflow

  //RF1AIES |= BIT1;  //GD01/IFG1 want negedge

  _mrf_initial_receive_enable();  // take longer to get PLL locked first time
  return 0;
}



uint16 rf_if_clt0(){
  return RF1AIFCTL0;

}

uint16 rf_if_clt1(){
  return RF1AIFCTL1;

}


uint16 rf_iflg(){
  return RF1AIFG;

}
uint16 rf_iflgin(){
  return RF1AIN;

}
uint16 rf_iedge(){
  return RF1AIES;

}

uint16 rf_ien(){
  return RF1AIE;

}

uint16 rf_if_err(){
  return RF1AIFERR;

}

uint16 rf_if_iflg(){
  return RF1AIFIFG ;

}
uint16 rf_if_ien(){
  return   RF1AIFIE;

}


int _Dbg11(){
  return 11;
}


int _Dbg12(){
  return 12;
}
int _Dbg15(){
  return 15;
}
int _Dbg17(){
  return 17;
}

volatile static uint8 _dumpbyte;

uint8 rssi(){
  return ReadSingleReg(RSSI);
}


uint8 rx_status(){
  return Strobe(RF_SNOP | RF_RXSTAT);
}
uint8 tx_status(){
  return Strobe(RF_SNOP | RF_TXSTAT);
}

int _xb_hw_rd_rx_fifo(I_F i_f){
  //int i;
  uint8 i, len,bnum;
  uint8 *buff;
  uint8 lenerr = FALSE;
  uint8 rfstatus;
  uint8 pktstatus;
  uint16 ctl1_val;
  uint16 err_val;
  const MRF_IF *mif;
  mif = mrf_if_ptr(i_f);

  rfstatus = Strobe(RF_SNOP | RF_RXSTAT);  // nop read rx status
  ctl1_val = RF1AIFCTL1;
  err_val = RF1AIFERR;

  pktstatus = ReadSingleReg(PKTSTATUS);


  len = ReadSingleReg( RXBYTES );
  _Dbg17();

  /* nonsense
  if (len == 0) {
    len = ReadSingleReg( RXBYTES );
  }
  */
  if (len == 0) {
    rf_stats.rdnothing++;
    _Dbg15();
    _mrf_receive_reenable();
    return -1;
  }
  rf_stats.rdbytes++;

  _Dbg11();

  if ( len < sizeof(MRF_PKT_HDR) + 2) { // at least 2 are LQI and RSSI
    lenerr = TRUE;
    rf_stats.rdlenerr++;
  }
  else if  (len > _MRF_BUFFLEN + 2){ // packet too long
    rf_stats.rdlenerr++;
    lenerr = TRUE;
  }
  else{
    //bnum = mrf_alloc();
    bnum =  mrf_alloc_if(RF0);
    if ( bnum == _MRF_BUFFS){  // emergency buffer used to collect header + toks only
      mif->status->stats.alloc_err++;
      lenerr = TRUE;

    }else {
      buff = _mrf_buff_ptr(bnum);
      buff[0] = len;
    }
  }

  while (!(RF1AIFCTL1 & RFINSTRIFG));    // wait for RFINSTRIFG   // FIXME check this doesn't block - it shouldn't - should be able to remove this line
  RF1AINSTR1B = (RF_RXFIFORD | RF_REGRD);

  if ( lenerr == FALSE){

    buff[0] = RF1ADOUT1B;
    buff[0] = len -2;
    for (i = 1; i < (len-2); i++){     // append rssi and lqi
      if ( lenerr == FALSE)
        buff[i] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
    }
    if ( i < 60){  // don't risk buff overflow by appending RSSI and LQI for long packets
      buff[i] = RF1ADOUT1B;  // rssi
      i++;
      buff[i] = RF1ADOUT1B;  // lqi
    }

  } else{

    for (i = 1; i < (len-2); i++){
       _dumpbyte = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
    }
  }

  if (lenerr){ // re-enable reception
    _mrf_receive_reenable();
    return -1;
  }
  _mrf_receive_reenable();  // need to be rx if not tx

  MRF_PKT_HDR *pkt = (MRF_PKT_HDR *)buff;
  if (pkt->type == mrf_cmd_device_info)
    _Dbg12();


  mrf_buff_loaded(bnum);
  return 0;

}

uint8_t sb_state(uint8_t sv){
  return 23;
}

int  _xb_hw_wr_tx_fifo(int len , uint8 *buff){
  int i;
  uint8_t sv;
  if ((len < 1 ) || ( len > 255 ))
    return -1;

  RF1AIES |= BIT9;       // posedge
  RF1AIFG &= ~BIT9;                         // clear interrupt flag
  RF1AIE |= BIT9;                           // enable TX end-of-packet interrupt

  //Strobe( RF_STX );                       // Strobe STX
  /* total bodging - what is this for? */
  Strobe( RF_SIDLE );
  Strobe( RF_SFTX  );
  sv = Strobe( RF_SNOP );
  sb_state(sv);

  sv = Strobe( RF_STX );                         // Strobe STX
  i = 0;

   while ((sv != 0x2f) && ( i < 50)){
    sv = Strobe( RF_STX );
    i++;
  }

  if (sv != 0x2f)
    _rf_tx_dbg(i);


  // Strobe STX
  while (!(RF1AIFCTL1 & RFINSTRIFG));        // wait for tx ready
  RF1AINSTRW = ((RF_TXFIFOWR | RF_REGWR)<<8 ) + (uint8)(len-1); // write reg addr + len
  for (i = 1; i < len; i++){
    while (!(RFDINIFG & RF1AIFCTL1));
    RF1ADINB = buff[i];
  }
  i = RF1ADOUTB;                            // reset RFDOUTIFG flag status byte
  sb_state(sv);


  return 0;
}

void inline xb_hw_disable_tx_eop(void){
RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt
}

void mrf_rf_idle(I_F i_f){
  const MRF_IF *ifp = mrf_if_ptr(i_f);
  //ifp->status->state = MRF_ST_IDLE;
  Strobe( RF_SIDLE );
  Strobe( RF_SWOR );

  //ResetRadioCore();  // test if needs re-init
}

int _mrf_rf_tx_intr(I_F i_f){
  const MRF_IF *ifp = mrf_if_ptr(i_f);
  IF_STATE *if_state = &(ifp->status->state);
  xb_hw_disable_tx_eop();
	// _dbg100(xbst);
        //  (*if_state) = XB_ST_IDLE;
        // P3OUT &= ~BIT6;
  mrf_if_tx_done(i_f);
  _mrf_receive_enable();
  /*
  // Turn off LED after Transmit
  if ((*if_state) ==  MRF_ST_TX){
    _mrf_receive_enable();
    // we're now waiting for SACK
    ifp->status->state = MRF_ST_WAITSACK;
  }
  else
  if((*if_state) ==  MRF_ST_ACK){

#ifdef SLEEP_deep
      mrf_rf_idle(i_f);
#else
      _mrf_receive_enable();
#endif
      /*
   if (mrf_if_can_sleep(i_f))
      mrf_rf_idle(i_f);
    else{
      _mrf_receive_enable();
    }

  }else { // some cockup
    ifp->status->stats.st_err ++;
  }
*/
  return 0;
}

uint8 rf1stb;

uint8 GetRF1ASTATB(){
  uint8 rv = RF1ASTATB;
  return rv;

}

uint16_t rfa1in;
uint16_t rfa1flg;
uint16_t rfa1ie;
uint16_t rfa1ies;

int cca_return_l1(){
  return 0;
}
int _mrf_rf_cca(){
  rfa1in = RF1AIN;
  rfa1flg = RF1AIFG;
  rfa1ie = RF1AIE;
  rfa1ies = RF1AIES;

  if (rfa1in & BITC){
    mrf_if_ptr(RF0)->status->channel_clear = 1;
    rf_stats.channel_was_clear++;
  }
  else {
    mrf_if_ptr(RF0)->status->channel_clear = 0;
    rf_stats.channel_not_clear++;
  }
  // disable int once triggered

  RF1AIE &= ~BITC;

  return cca_return_l1();
  //return 0;
}

unsigned int _dbg_iv(unsigned int iv){
  return iv;
}
static unsigned int RF1AIVREG;

interrupt(CC1101_VECTOR) CC1101_ISR(void)
{

  rf_stats.transmitting = mrf_if_transmitting(RF0);

  rf_stats.ints++;
  rf1stb = GetRF1ASTATB();
  rf_stats.laststat = rf1stb;

  rf_stats.rxstat = Strobe(0x3D + 0x80);
  rf_stats.txstat = Strobe(0x3D);
  rf_stats.pktstatus = ReadSingleReg(PKTSTATUS);
  // switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt
  RF1AIVREG = RF1AIV;
  _dbg_iv(RF1AIVREG);
  //  switch(RF1AIV,32)        // Prioritizing Radio Core Interrupt
  switch(RF1AIVREG)        // Prioritizing Radio Core Interrupt
  {
#if 0
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

#endif

  case   RF1AIV_RFIFG9:
    if (rf_stats.transmitting){
      rf_stats.wrint++;
      _mrf_rf_tx_intr(RF0);
    }
    else {
      rf_stats.rdint++;
      _xb_hw_rd_rx_fifo(RF0);
    }
    if (mrf_wake_on_exit())
      __bic_SR_register_on_exit(LPM3_bits);

    break;



  case RF1AIV_RFIFG0:
    if (!rf_stats.transmitting) {

      rf_stats.rdint++;
      _xb_hw_rd_rx_fifo(RF0);
    }

    /*
    else if (xbtransmitting){
      rf_stats.wrint++;
      _mrf_rf_tx_intr(RF0);
      }
    */
     else
       rf_stats.oddint++;

    if (mrf_wake_on_exit())
      __bic_SR_register_on_exit(LPM3_bits);

      // else while(1); 			    // trap - sometimes gets in here -
    break;
  case RF1AIV_RFIFG12:  // NB set for negedge, i.e. when a signal is detected
    rf_stats.ccaint++;
    _mrf_rf_cca();
    break;


#if 0
    case RF1AIV_RFIFG10:   break;                 // RFIFG10  - RX packet with checksum OK
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
#endif
  default : rf_stats.nodefint++;
            rf_stats.lastnodef = RF1AIVREG;

  }

}
