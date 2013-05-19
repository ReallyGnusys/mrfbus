#include "glib.h"


// dumping ground of potentially reusable code


int hexchartonibble(uint8 hchar){
  if (( hchar >= '0') && ( hchar <= '9'))
    return (int) (hchar - '0');
  else if(( hchar >= 'A') && ( hchar <= 'F'))
    return (int)(10 + hchar - 'A');
  else if (hchar == 0)  // end of string
    return HCTN_EOS;  // ugly
  else
    return HCTN_ERR;      
}
