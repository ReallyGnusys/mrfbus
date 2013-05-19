#include "g430_types.h"
#include  <msp430.h>
#include <legacymsp430.h>

#include "cc430x513x.h"

#define LOW(port,bit)  port&=~bit
#define HIGH(port,bit)  port|=bit

#define HCTN_EOS -2
#define HCTN_ERR -1


int hexchartonibble(uint8 hchar);
