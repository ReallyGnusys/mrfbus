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
   mrf_app_cmd_spi_read = 1,
   mrf_app_cmd_spi_write = 2,
   mrf_app_cmd_spi_debug = 3,
   mrf_app_cmd_spi_data  = 4,
   MRF_NUM_APP_CMDS = 5
 } MRF_APP_CMD_CODE;

#endif
