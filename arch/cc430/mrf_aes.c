
#include <msp430.h>
//#include "cc430x513x.h"
#include "cc430f5137.h"

#include <stdint.h>

#include "mrf_kdata.h"

/* load key or IV to AES register
 * ac - AES registrer address, key - key to load
 */

#define AES_CBC_MODE  (1 << 5)
#define AESCMEN        (1 << 15)
#define AESAXDIN      (AESACTL0_ +  0x00C)

#define AESAXIN_      (AESACTL0_ +  0x00E)

#define AESACTL1_             0x09C2    /* AES accelerator control register 0 */
#define AESACTL1             (*((uint16_t *)AESACTL1_))    /* AES accelerator control register 0 */


//sfrb(AESACTL1_L , AESACTL1_);
//sfrb(AESACTL1_H , AESACTL1_+1);
//sfrw(AESACTL1, AESACTL1_);

int dma_int_cnt;
int aes_int_cnt;
static volatile int _aes_busy;
extern const uint8_t  *_mrf_key;
uint8_t _mrf_dec_key[8];

int _aes_keygen_err(){
  return 0;
}


void aes_init(){
  //uint16_t *keyreg = (uint16_t *)(&AESAKEY);
  dma_int_cnt = 0;
  aes_int_cnt = 0;
  *(uint16_t *)AESACTL0_ = AESSWRST;                          // reset AES, AESCMEN=0, AESOPx=00


  // generate decryption key
  for (int i=0;i<8;i++)
    _mrf_dec_key[i] = 0;

  *(uint16_t *)AESACTL0_ = 2;                     // AESCMEN=1, AESCMx=01

  for (int i=0;i<8;i++)
    *(uint16_t *)AESAKEY_ = _mrf_key[i];





  for (int i=0;(i<150) &&((*(uint16_t *)AESACTL0_ & AESRDYIFG) == 0) ;i++);

  if (*(uint16_t *)AESACTL0_ & AESRDYIFG){

    for (int i=0;i<8;i++) {
      _mrf_dec_key[i] = *((uint16_t *)(AESADOUT));
    }

  }
  else{
    _aes_keygen_err();

  }


  /*

  AESACTL0 = 0;                     // AESCMEN=1, AESCMx=01


  for (int i=0;i<8;i++)
    *keyreg = ((uint16_t *)_mrf_key)[i];
    */
   _aes_busy = 0;
}

uint16_t outbuff[8], outbuff2[8];

uint16_t ctl0_in, ctl0_out;

static  uint16_t *_aes_output;

volatile uint16_t _dbgv1;

uint16_t dgb303(uint16_t actl){
  _dbgv1 = actl;
  return _dbgv1;
}
uint16_t dgb304(uint16_t actl){
  _dbgv1 = actl;
  return _dbgv1;
}

volatile int _aes_to_cnt;

int _aes_dec;
int aes_timeout(int i){
  _aes_to_cnt = i;
  return _aes_to_cnt;
}

volatile uint16_t _dbgcnt;
uint16_t wait_not_busy(){
  int i;
  for (i = 0 ; (i < 200) && (_aes_busy == 1) ; i++);

  if (_aes_busy == 1){
    aes_timeout(i);
    return -1;
  }

  return 0;
}

void load_enc_key(){
  for (int i=0;i<8;i++)
    *(uint16_t *)AESAKEY_ = _mrf_key[i];

}

void aes_encrypt(uint16_t  *in,uint16_t  *out, uint16_t *iv, int len){

  wait_not_busy();
  _aes_dec = 0;
  _aes_busy = 1;
  ctl0_in  = *(uint16_t *)AESACTL0_;
  *(uint16_t *)AESACTL0_ =  2;
  *(uint16_t *)AESACTL0_ = 0 + AESRDYIE;                     // AESCMEN=1, AESCMx=01


  load_enc_key();
  //AESASTAT |= AESKEYWR;
  _aes_output = out;
  __bis_SR_register(GIE);  // GIE should always be set for foreground process

  for (int i=0;i<8;i++) {
    //outbuff[i] = 0;
    out[i] = 0;
    // *((uint16_t *)(AESAXIN_)) = iv[i];

  }
  for (int i=0;i<8;i++) {
    *((uint16_t *)(AESADIN_)) = in[i];
  }

  ctl0_out  = AESACTL0;
  dgb303(ctl0_out);
}


void aes_encrypt2(uint16_t  *in,uint16_t  *out, uint16_t *iv, int len){
  wait_not_busy();

  _aes_dec = 0;

  _aes_busy = 1;

  AESACTL0 =  AESRDYIE; // decrypt using same key
  AESASTAT |= AESKEYWR;
  ctl0_in  = AESACTL0;
  _aes_output = out;
  __bis_SR_register(GIE);  // GIE should always be set for foreground process

  for (int i=0;i<8;i++) {
    //outbuff2[i] = 0;
    out[i] = 0;
    *((uint16_t *)(AESAXIN_)) = 0;


  }
  for (int i=0;i<8;i++) {
    *((uint16_t *)(AESADIN_)) = in[i];
  }

  ctl0_out  = AESACTL0;

}


void load_dec_key(){
  for (int i=0;i<8;i++)
    *(uint16_t *)AESAKEY_ = _mrf_dec_key[i];

}

void aes_decrypt(uint16_t  *in,uint16_t  *out, uint16_t *iv, int len){

  wait_not_busy();

  _aes_busy = 1;
  _aes_dec = 1;

  *(uint16_t *)AESACTL0_ =  0;
  *(uint16_t *)AESACTL0_ =  AESRDYIE | 0x03;

  //for (int i=0;i<8;i++)
  //  *keyreg = ((uint16_t *)_mrf_dec_key)[i];

  load_dec_key();

  //for (int i=0;i<8;i++)
  //  *(uint16_t *)AESAKEY_ = _mrf_dec_key[i];


  //AESASTAT |= AESKEYWR;
  ctl0_in  = *(uint16_t *)AESACTL0_;
  _aes_output = out;
  __bis_SR_register(GIE);  // GIE should always be set for foreground process

  for (int i=0;i<8;i++) {
    //outbuff[i] = 0;
    out[i] = 0;
    // *((uint16_t *)(AESAXIN_)) = iv[i];

  }
  for (int i=0;i<8;i++) {
    *((uint16_t *)(AESADIN_)) = in[i];
  }

  ctl0_out  = *(uint16_t *)AESACTL0_;
  dgb304(ctl0_out);

}

volatile int tbd_int_1;

int got_aes_dec_out(){
  tbd_int_1 = 0;
  return 0;
}


void aes_get_output(){

  for (int i=0;i<8;i++) {
    //outbuff[i] = *((uint16_t *)(AESADOUT));
    _aes_output[i] = *((uint16_t *)(AESADOUT));
  }
  _aes_busy = 0;
  if (_aes_dec == 1){
    got_aes_dec_out();
  }
}



/*
  DMACTL0=DMA0TSEL_11 | DMA1TSEL_12;                  // Set DMA 0-1 triggers

  DMA0CTL=DMADT_0 | DMALEVEL | DMASRCINCR_3 | DMADSTINCR_0;       // configure DMA 0
  // DMA 0 copy output to outbuff for now
  *((uint32_t *)(&DMA0SA)) = (uint32_t)&AESADOUT;
  *((uint32_t *)(&DMA0DA)) = (uint32_t)(out);

  DMA0SZ=8;                         // Size packet in word
  DMA0CTL|= DMAEN;                            // Enable DMA 0

  // DMA 1 COPY INPUT to AESAXDIN
  DMA1CTL=DMADT_0 | DMALEVEL;                         // configure DMA 1
  *((uint32_t *)(&DMA1SA)) = (uint32_t)in;
  *((uint32_t *)(&DMA1DA)) = (uint32_t)(AESAXDIN);
  DMA1SZ=8;                         // Size packet in word
  DMA1CTL|= DMAEN;                            // Enable DMA 0

  *((uint16_t *)AESACTL1_)=1;                           // start AES set AESBLKCNT size packet/128bit(8 byte)

  while(!(DMA0CTL & DMAIFG));                         // wait end of AES encrypt on DMA 0

}
*/

/* Crypt/Decrypt AES Cipher Block Chaining (CBC) Mode
 * in - input buffer, out - output buffer, key - security key, iv - Initialization Vector
 * mode=0 - encrypt in -> out
 * mode=1 - decrypt in -> out
 * tested 8MHz enc - 0.0075ms, dec - 0.008ms
 */

#if 0
void aesCBC(unsigned char mode, unsigned char *in, unsigned char *out, unsigned char *key, unsigned char *iv) {
    AESACTL0=AESSWRST;                          // reset AES, AESCMEN=0, AESOPx=00
    //AESACTL0=AESKL__128;                            // set KEY length
    // ------------------------------ AES CBC generate key in decrypt mode -------
    if(mode) {
        AESACTL0|=AESOP1;                           // AESOPx=10
        loadKey(&AESAKEY,key);                          // Load  AES key
        AESACTL0|=AESOP0;                           // AESOPx=11
    }
    AESACTL0|=AESCM0 | AESCMEN;                     // AESCMEN=1, AESCMx=01
    // ------------------------------ AES CBC generate key in crypt mode ---------
    if(mode) {
        AESASTAT|=AESKEYWR;
    } else {
        loadKey(&AESAKEY,key);                          // Load AES key
        loadKey(&AESAXIN,iv);                           // Load IV
    }
    DMACTL0=DMA0TSEL_11 | DMA1TSEL_12;                      // Set DMA 0-1 triggers
    if(mode) DMACTL1=DMA2TSEL_13;                           // Set DMA 2 triggers
    DMA0CTL=DMADT_0 | DMALEVEL;                         // configure DMA 0
    if(mode) {
        DMA0CTL|=DMASRCINCR_3 | DMADSTINCR_0;                       // configure DMA 0
        __data20_write_long((unsigned long)&DMA0SA, (unsigned long)iv);         // Source address
        __data20_write_long((unsigned long)&DMA0DA, (unsigned long)&AESAXIN);       // Destination address
    } else {
        DMA0CTL|=DMASRCINCR_0 | DMADSTINCR_3;                       // configure DMA 0
        __data20_write_long((unsigned long)&DMA0SA, (unsigned long)&AESADOUT);      // Source address
        __data20_write_long((unsigned long)&DMA0DA, (unsigned long)out);        // Destination address
    }
    DMA0SZ=mode?8:PACKET_LEN>>1;                          // Size packet in word or 8 word
    DMA0CTL|= DMAEN;                                // Enable DMA 0
    DMA1CTL=DMADT_0 | DMALEVEL;                         // configure DMA 1
    if(mode) {
        DMA1CTL|=DMASRCINCR_0 | DMADSTINCR_3;                       // configure DMA 1
        __data20_write_long((unsigned long)&DMA1SA, (unsigned long)&AESADOUT);      // Source address
        __data20_write_long((unsigned long)&DMA1DA, (unsigned long)out);        // Destination address
    } else {
        DMA1CTL|=DMASRCINCR_3 | DMADSTINCR_0;                       // configure DMA 1
        __data20_write_long((unsigned long)&DMA1SA, (unsigned long)in);         // Source address
        __data20_write_long((unsigned long)&DMA1DA, (unsigned long)&AESAXDIN);      // Destination address
    }
    DMA1SZ=PACKET_LEN>>1;                             // Size packet in word
    DMA1CTL|= DMAEN;                                // Enable DMA 1
    if(mode) {
        DMA2CTL=DMADT_0 | DMALEVEL | DMASRCINCR_3 | DMADSTINCR_0;           // configure DMA 2 for decrypt
        __data20_write_long((unsigned long)&DMA2SA, (unsigned long)in);         // Source address
        __data20_write_long((unsigned long)&DMA2DA, (unsigned long)&AESADIN);       // Destination address
        DMA2SZ=PACKET_LEN>>1;                             // Size packet in word
        DMA2CTL|= DMAEN;                                // Enable DMA 2
    }
    AESACTL1=PACKET_LEN>>4;                           // start AES set AESBLKCNT size packet/128bit(8 byte)
    if(mode) {
        while(!(DMA0CTL & DMAIFG));                         // wait end of AES decrypt first block
        DMA0CTL=DMADT_0 | DMALEVEL | DMASRCINCR_3 | DMADSTINCR_0;           // configure DMA 0
        __data20_write_long((unsigned long)&DMA0SA, (unsigned long)in);         // Source address
        __data20_write_long((unsigned long)&DMA0DA, (unsigned long)&AESAXIN);       // Destination address
        DMA0SZ=PACKET_LEN/2-8;                              // Size in word - Packet-8 word
        DMA0CTL|=DMAEN;                                 // Enable DMA 0
        while(!(DMA1CTL & DMAIFG));                         // wait end of AES decrypt on DMA 1
    } else {
        while(!(DMA0CTL & DMAIFG));                         // wait end of AES encrypt on DMA 0
    }
}//----- end aesCBC

#endif

#pragma vector=DMA_VECTOR
__interrupt void DMA_ISR(void)

//interrupt (DMA_VECTOR) DMA_ISR()
{
  dma_int_cnt++;
  switch(DMAIV)
  {
  case 0:break;
  case 2:break;   // DMA0IFG - Chan 0
  case 4:break;   // DMA1IFG - Chan 1
  case 6:break;   // DMA2IFG - Chan 2
  default : break;

  }
}


#pragma vector=AES_VECTOR
__interrupt void AES_ISR(void)

//interrupt (DMA_VECTOR) DMA_ISR()
{
  aes_int_cnt++;

  aes_get_output();

}
