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

#ifndef _MRF_INCLUDED_
#define _MRF_INCLUDED_

#include <mrf_sys.h>

//#include <mrf_types.h>
//#include <mrf_if.h>
#include <mrf_arch.h>
#include <mrf_debug.h>

//#include <mrf_sys_structs.h>



int mrf_init(void);

int mrf_main_loop(void);

int mrf_rtc_get(TIMEDATE *td);
int mrf_rtc_set(TIMEDATE *td);

int mrf_tick_enable();
int mrf_tick_disable();

int mrf_time(char *buff);

int mrf_app_init() __attribute__ ((constructor));
#endif
