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

#ifndef __MRF_APP_CMDS_INCLUDED__
#define __MRF_APP_CMDS_INCLUDED__


typedef enum mrf_app_cmd_code
 {
   mrf_app_cmd_test = 0,
   mrf_app_cmd_led_on  = 1,
   mrf_app_cmd_led_off  = 2,
   mrf_app_cmd_get_relay  = 3,
   mrf_app_cmd_set_relay  = 4,
   mrf_app_cmd_read_state  = 5,   
   MRF_NUM_APP_CMDS = 6
 } MRF_APP_CMD_CODE;


#endif
