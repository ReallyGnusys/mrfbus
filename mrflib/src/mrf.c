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

#include <mrf.h>
#include <stdio.h>

#define _QUOTE_SYM_(SYM)  #SYM
#define SYM_NAME(name) _QUOTE_SYM_(name)

#define _CONCAT_(A,B) # A ## B

//#define _DEVNAME_STR_  SYM_NAME(DEVTYPE)

#define _DEVNAME_STR_  SYM_NAME(_CONCAT_(DEVTYPE,MRFID))

const MRF_PKT_DEVICE_INFO device_info  = { SYM_NAME(DEVTYPE) , MRFID, MRFNET, _MRF_BUFFS,NUM_INTERFACES };
const MRF_PKT_SYS_INFO sys_info        = { (const uint8)MRF_NUM_SYS_CMDS, SYM_NAME(GITSH), GITMOD , SYM_NAME(MRFBLD) };


int _print_mrf_cmd(MRF_CMD_CODE cmd);

int mrf_init(){

  mrf_arch_boot();
  mrf_if_init();
  mrf_sys_init();
  mrf_app_init();
  mrf_arch_run();
  return 0; // unreachable
}


int mrf_time(char *buff){
  TIMEDATE td;
  mrf_rtc_get(&td);
  return sprintf(buff,"%02d:%02d:%02d %02d-%02d-%04d",td.hour,td.min,td.sec,td.day,td.mon,td.year+2000);
}


