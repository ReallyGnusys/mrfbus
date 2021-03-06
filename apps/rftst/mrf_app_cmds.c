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

extern const MRF_CMD mrf_app_cmds[MRF_NUM_APP_CMDS] = {
  {"APP_TEST"  , 0 , 0                        , sizeof(MRF_PKT_TIMEDATE)  ,  NULL  , mrf_app_task_test  },
  {"LED_ON" , 0  ,  sizeof(MRF_PKT_RELAY_STATE) , 0   ,  NULL  , mrf_app_led_on   },

  {"LED_OFF" , 0  ,  sizeof(MRF_PKT_RELAY_STATE) , 0   ,  NULL  , mrf_app_led_off   },

  {"GET_RELAY" , 0  , sizeof(MRF_PKT_RELAY_STATE) , sizeof(MRF_PKT_RELAY_STATE)   ,  NULL  , mrf_app_get_relay   },
  {"SET_RELAY" , 0  , sizeof(MRF_PKT_RELAY_STATE) , sizeof(MRF_PKT_RELAY_STATE)   ,  NULL  , mrf_app_set_relay   },
  {"READ_STATE", 0  , 0    , sizeof(MRF_PKT_RFMODTC_STATE)  ,  NULL  , mrf_app_read_state   }
  
};


const uint8 mrf_num_app_cmds = (uint8)MRF_NUM_APP_CMDS;
