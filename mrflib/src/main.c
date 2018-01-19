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

const uint8 _mrfid = MRFID;

#define _DEVNAME_STR_  SYM_NAME(_CONCAT_(DEVTYPE,MRFID))

const MRF_PKT_DEVICE_INFO device_info  = { SYM_NAME(DEVTYPE) , MRFID, MRFNET, _MRF_BUFFS,NUM_INTERFACES };
const MRF_PKT_SYS_INFO sys_info        = {  SYM_NAME(GITSH), SYM_NAME(MRFBLD), (const uint8)MRF_NUM_SYS_CMDS,GITMOD };
const MRF_PKT_APP_INFO app_info        = {SYM_NAME(MRF_APP), MRF_NUM_APP_CMDS};


int main(void){
  mrf_init();
  return 0;
}

