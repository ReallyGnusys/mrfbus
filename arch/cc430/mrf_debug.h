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

#include <stdarg.h>
extern const uint8 _mrfid;

//#define mrf_debug(format, ...) mrf_cc_debug( format, ## __VA_ARGS__ )

#define mrf_debug(format, ...)

//int mrf_cc_debug( const char *fmt, ...);


#endif
