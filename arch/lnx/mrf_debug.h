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

#ifndef __MRF_DEBUG_INCLUDED__
#define __MRF_DEBUG_INCLUDED__
/*

#define mrf_debug  printf
*/
#include <stdio.h>
//#define DEBUG_LEVEL 1

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1
#endif

long long mrf_timestamp();
//extern const uint8 _mrfid;

//#define mrf_debug(x)  _mrf_debug("%s",x)

#define mrf_debug(level,fmt, ...)                                       \
  do { if (level<=DEBUG_LEVEL) fprintf(stdout, "%lld ID:%02x %s:%d:(): " fmt, mrf_timestamp(),MRFID,__FILE__, \
                                __LINE__, __VA_ARGS__); } while (0)
      //         do { if (MRF_DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

// do { if (MRF_DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)
//  do { if (MRF_DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__,   \
//                                __LINE__, __func__, __VA_ARGS__); } while (0)



#endif
