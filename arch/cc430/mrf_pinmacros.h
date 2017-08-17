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

#define MAPREG(signame) _CONCAT(_CONCAT(_PORT_MACRO(signame),MAP),_BIT_MACRO(signame))


#define INPUTVAL(signame) ((INREG(signame) & BITNAME(signame)) != 0)

#define OUTPUTPIN(signame)  DIRREG(signame) |= BITNAME(signame) ; SELREG(signame) &= (~BITNAME(signame))
#define INPUTPIN(signame)  DIRREG(signame) &= (~BITNAME(signame)) ; SELREG(signame) &= (~BITNAME(signame))
#define PINLOW(signame)   OUTREG(signame) &= (~BITNAME(signame))
#define PINHIGH(signame)  OUTREG(signame) |= BITNAME(signame)
#define NOPULLUP(signame) RENREG(signame) &= (~BITNAME(signame))

#define ANALOGUEPIN(signame)  SELREG(signame) |= BITNAME(signame) ; MAPREG(signame) = 31

#define ADCPIN(signame) INPUTPIN(signame) ; NOPULLUP(signame) ; ANALOGUEPIN(signame)

#endif
