#ifndef __MRF_PINMACROS_INCLUDED__
#define __MRF_PINMACROS_INCLUDED__

#define _CONCAT2(_A,_B) _A ## _B
#define _CONCAT(_A,_B)  _CONCAT2(_A,_B)
//#define PORT(signame) _##signame##_PORT
//#define PORT(signame) _CONCAT(signame,_PORT)
#define _PORT_MACRO(signame) _ ## signame ## _PORT
#define _BIT_MACRO(signame)  _ ## signame ## _BIT

#define BITNAME(signame) _CONCAT(BIT,_BIT_MACRO(signame))


#ifdef OUT
 #define _OUT_DEF_SAVE OUT
 #undef OUT  // need to override OUT definition in cc430f5137.h "
#else
 #undef _OUT_DEF_SAVE
#endif


//#define OUT OUT
// msp430 I/O control regs
#define OUTREG(signame) _CONCAT(_PORT_MACRO(signame),OUT)
#define INREG(signame) _CONCAT(_PORT_MACRO(signame),IN)
#define RENREG(signame) _CONCAT(_PORT_MACRO(signame),REN)
#define SELREG(signame) _CONCAT(_PORT_MACRO(signame),SEL)
#define DIRREG(signame) _CONCAT(_PORT_MACRO(signame),DIR)


#define OUTPUTPIN(signame)  DIRREG(signame) |= BITNAME(signame)
#define PINLOW(signame)   OUTREG(signame) &= ~ BITNAME(signame)
#define PINHIGH(signame)  OUTREG(signame) |= BITNAME(signame)




#endif
