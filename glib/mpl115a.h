#ifndef __MPL115A_INCLUDED__
#define __MPL115A_INCLUDED__

#include "cc430f5137.h"
#include "g430_types.h"
#include "spi.h"


int mpl115a_init();
int mpl115a_start_convert();
int mpl115a_read_results();
int mpl115a_read_coeffs();


int mpl115a_copy_coeffs(uint8 *buffer);
int mpl115a_copy_results(uint8 *buffer);

void mpl115a_conv_delay();
void mpl115a_wake_delay();

#endif
