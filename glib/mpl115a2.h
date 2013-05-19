#ifndef __MPL115A_INCLUDED__
#define __MPL115A_INCLUDED__

#include "cc430f5137.h"
#include "g430_types.h"


int mpl115a_init();
void mpl115a_convert();
int mpl115a_copy_coeffs(uint8 *buffer);
int mpl115a_copy_results(uint8 *buffer);
void mpl115a_sleep();
void mpl115a_wake();


#endif
