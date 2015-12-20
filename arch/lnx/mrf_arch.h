#ifndef _MRF_ARCH_INCLUDED_
#define _MRF_ARCH_INCLUDED_


#define IQUEUE_DEPTH 8

#include <stdlib.h>
#define handle_error(msg)  \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)


void trim_trailing_space(uint8 *buff);

#endif
