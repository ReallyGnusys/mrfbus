#ifndef __SHT21_INCLUDED__
#define __SHT21_INCLUDED__
#include "i2c.h"
int sht21_start_temp();
int sht21_start_rh();
int sht21_read_measurement(uint16 *readval);
#endif
