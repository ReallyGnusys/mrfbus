/**********************************************************************
*
*    Author:		Energy Micro AS
*    Modified for the MOD-LCD1x9 by shondll (shondll_yahoo.com)
*    Company:		Olimex Ltd.
*    Date:		07/02/2011
*    File Version:	1.00
*    Description:	LCD mapping and font
*
**********************************************************************/
#ifndef _LCD1x9_MAPPING_H
#define _LCD1x9_MAPPING_H

/**************************************************************************//**
 * @brief
 * Defines each text symbol's segment in terms of COM and BIT numbers,
 * in a way that we can enumerate each bit for each text segment in the
 * following bit pattern:
 * @verbatim
 *  -------0------
 *
 * |   \7  |8  /9 |
 * |5   \  |  /   |1
 *
 *  --6---  ---10--
 *
 * |    /  |  \11 |
 * |4  /13 |12 \  |2
 *
 *  -------3------  .(14)
 *  ------15------
 *
 * @endverbatim
 * E.g.: First text character bit pattern #3 (above) is
 *  Segment 1D for Display
 *  Location COM 3, BIT 0
 *****************************************************************************/
typedef struct
{
  BYTE com[16]; /**< LCD COM line (for multiplexing) */
  BYTE bit[16]; /**< LCD bit number */
} CHAR_TypeDef;

/**************************************************************************//**
 * @brief Defines prototype for all segments in display
 *****************************************************************************/
typedef struct
{
  CHAR_TypeDef			Text[9];
} MCU_DISPLAY;

/**************************************************************************//**
 * @brief Working instance of LCD display
 *****************************************************************************/
const MCU_DISPLAY LCD1x9 = {
  {
    { /* 1 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{  34 - 0,  32 - 0,  32 - 0,  34 - 0,  35 - 0,  35 - 0,  34 - 0,  35 - 0,  33 - 0,  33 - 0,  32 - 0,  32 - 0,  33 - 0,  34 - 0,  33 - 0,  35 - 0}  // bit    
    },
    { /* 2 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{  34 - 4,  32 - 4,  32 - 4,  34 - 4,  35 - 4,  35 - 4,  34 - 4,  35 - 4,  33 - 4,  33 - 4,  32 - 4,  32 - 4,  33 - 4,  34 - 4,  33 - 4,  35 - 4}  // bit    
    },
    { /* 3 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{  34 - 8,  32 - 8,  32 - 8,  34 - 8,  35 - 8,  35 - 8,  34 - 8,  35 - 8,  33 - 8,  33 - 8,  32 - 8,  32 - 8,  33 - 8,  34 - 8,  33 - 8,  35 - 8}  // bit    
    },
    { /* 4 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{ 34 - 12, 32 - 12, 32 - 12, 34 - 12, 35 - 12, 35 - 12, 34 - 12, 35 - 12, 33 - 12, 33 - 12, 32 - 12, 32 - 12, 33 - 12, 34 - 12, 33 - 12, 35 - 12}  // bit    
    },
    { /* 5 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{ 34 - 16, 32 - 16, 32 - 16, 34 - 16, 35 - 16, 35 - 16, 34 - 16, 35 - 16, 33 - 16, 33 - 16, 32 - 16, 32 - 16, 33 - 16, 34 - 16, 33 - 16, 35 - 16}  // bit    
    },
    { /* 6 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{ 34 - 20, 32 - 20, 32 - 20, 34 - 20, 35 - 20, 35 - 20, 34 - 20, 35 - 20, 33 - 20, 33 - 20, 32 - 20, 32 - 20, 33 - 20, 34 - 20, 33 - 20, 35 - 20}  // bit    
    },
    { /* 7 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{ 34 - 24, 32 - 24, 32 - 24, 34 - 24, 35 - 24, 35 - 24, 34 - 24, 35 - 24, 33 - 24, 33 - 24, 32 - 24, 32 - 24, 33 - 24, 34 - 24, 33 - 24, 35 - 24}  // bit    
    },
    { /* 8 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{ 34 - 28, 32 - 28, 32 - 28, 34 - 28, 35 - 28, 35 - 28, 34 - 28, 35 - 28, 33 - 28, 33 - 28, 32 - 28, 32 - 28, 33 - 28, 34 - 28, 33 - 28, 35 - 28}  // bit    
    },
    { /* 9 */
	{       3,       3,       0,       0,       2,       1,       1,       3,       1,       3,       1,       2,       2,       2,       0,       0}, // com
	{ 34 - 32, 32 - 32, 32 - 32, 34 - 32, 35 - 32, 35 - 32, 34 - 32, 35 - 32, 33 - 32, 33 - 32, 32 - 32, 32 - 32, 33 - 32, 34 - 32, 33 - 32, 35 - 32}  // bit    
    }
  }
};

/**************************************************************************//**
 * @brief
 * Defines higlighted segments for the alphabet, starting from "blank" (SPACE)
 * Uses bit pattern as defined for text segments above.
 * E.g. a capital O, would have bits 0 1 2 3 4 5 => 0x003f defined
 *****************************************************************************/
const WORD LCDAlphabet[] = {
  0x0000, /* space */
  0x1100, /* ! */
  0x0280, /* " */
  0x0000, /* # */
  0x0000, /* $ */
  0x0000, /* % */
  0x0000, /* & */
  0x0000, /* � */
  0x0039, /* ( */
  0x000f, /* ) */
  0x0463, /* * ->> modified to a degree symbol */ 
  0x1540, /* + */
  0x0000, /* , */
  0x0440, /* - */
  0x1000, /* . */
  0x2200, /* / */

  0x003f, /* 0 */
  0x0006, /* 1 */
  0x045b, /* 2 */
  0x044f, /* 3 */
  0x0466, /* 4 */
  0x046d, /* 5 */
  0x047d, /* 6 */
  0x0007, /* 7 */
  0x047f, /* 8 */
  0x046f, /* 9 */

  0x0000, /* : */
  0x0000, /* ; */
  0x0a00, /* < */
  0x0000, /* = */
  0x2080, /* > */
  0x0000, /* ? */
  0xffff, /* @ */

  0x0477, /* A */
  0x0a79, /* B */
  0x0039, /* C */
  0x20b0, /* D */
  0x0079, /* E */
  0x0071, /* F */
  0x047d, /* G */
  0x0476, /* H */
//  0x0006, /* I */
  0x0030, /* I edit */
  0x000e, /* J */
  0x0a70, /* K */
  0x0038, /* L */
  0x02b6, /* M */
  0x08b6, /* N */
  0x003f, /* O */
  0x0473, /* P */
  0x083f, /* Q */
  0x0c73, /* R */
  0x046d, /* S */
  0x1101, /* T */
  0x003e, /* U */
  0x2230, /* V */
  0x2836, /* W */
  0x2a80, /* X */
  0x046e, /* Y */
  0x2209, /* Z */

  0x0039, /* [ */
  0x0880, /* backslash */
  0x000f, /* ] */
  0x0001, /* ^ */
  0x0008, /* _ */
  0x0100, /* ` */

  0x1058, /* a */
  0x047c, /* b */
  0x0058, /* c */
  0x045e, /* d */
  0x2058, /* e */
  0x0471, /* f */
  0x0c0c, /* g */
  0x0474, /* h */
  0x0004, /* i */
  0x000e, /* j */
  0x0c70, /* k */
  0x0038, /* l */
  0x1454, /* m */
  0x0454, /* n */
  0x045c, /* o */
  0x0473, /* p */
  0x0467, /* q */
  0x0450, /* r */
  0x0c08, /* s */
  0x0078, /* t */
  0x001c, /* u */
  0x2010, /* v */
  0x2814, /* w */
  0x2a80, /* x */
  0x080c, /* y */
  0x2048, /* z */

  0x0000,
};

#endif // _LCD1x9_MAPPING_H
