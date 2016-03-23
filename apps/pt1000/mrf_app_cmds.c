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

#include "mrf_sys.h"

//#include "mrf_app_cmds.h"

const MRF_CMD mrf_app_cmds[MRF_NUM_APP_CMDS] = {
  [ mrf_app_cmd_test      ] = {"APP_TEST"    , 0                           , 0                          , sizeof(MRF_PKT_TIMEDATE)   ,  NULL          , mrf_app_task_test   },
  [ mrf_app_cmd_spi_read  ] = {"SPI_READ"   , 0                            , sizeof(MRF_PKT_UINT8)      ,sizeof(MRF_PKT_UINT8)       ,  NULL          , mrf_app_spi_read },
  [ mrf_app_cmd_spi_write ] = {"SPI_WRITE"  , 0                            , sizeof(MRF_PKT_UINT8_2)    ,0                           ,  NULL          , mrf_app_spi_write   }

};


const uint8 mrf_num_app_cmds = (uint8)MRF_NUM_APP_CMDS;

const MRF_PKT_APP_INFO app_info        = {"pt1000", MRF_NUM_APP_CMDS};
