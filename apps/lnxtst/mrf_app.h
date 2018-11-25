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

#ifndef __MRF_APP_INCLUDED__
#define __MRF_APP_INCLUDED__
/* default app , no new pkts defined 
 */

/* mrf_app_task_test
   returns current MRF_PKT_TIMEDATE 
*/

typedef struct  __attribute__ ((packed))   {
  uint32 sz;
  uint32 res;
  uint32 share;
  uint32 text;
  uint32 lib;
  uint32 data;
  uint32 dt;
  
} MRF_PKT_LNX_MEM_STATS;


MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_app_mem_stats(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);

#endif
