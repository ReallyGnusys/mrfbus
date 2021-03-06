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

#ifndef __MRF_SYS_CMDS_INCLUDED__
#define __MRF_SYS_CMDS_INCLUDED__
#include "mrf_sys.h"

typedef enum mrf_cmd_code
 {
   mrf_cmd_ack = 0,
   mrf_cmd_retry = 1,
   mrf_cmd_resp = 2,
   mrf_cmd_device_info = 3,
   mrf_cmd_device_status = 4,
   mrf_cmd_sys_info = 5,
   mrf_cmd_if_stats = 6,
   mrf_cmd_get_time = 7,
   mrf_cmd_set_time = 8,
   mrf_cmd_buff_state = 9,
   mrf_cmd_cmd_info = 10,
   mrf_cmd_app_info = 11,
   mrf_cmd_app_cmd_info = 12,
   mrf_cmd_test_1 = 13, 
   mrf_cmd_usr_struct = 14,
   mrf_cmd_usr_resp = 15,
   mrf_cmd_reset = 16,
   mrf_cmd_ping = 17,
   mrf_cmd_ndr = 18,   
   MRF_NUM_SYS_CMDS = 19
 } MRF_CMD_CODE;

typedef enum {
  MRF_CMD_RES_RETRY,
  MRF_CMD_RES_OK,
  MRF_CMD_RES_IGNORE,
  MRF_CMD_RES_WARN,
  MRF_CMD_RES_ERROR
} MRF_CMD_RES;


typedef MRF_CMD_RES (*MRF_CMD_FUNC)(MRF_CMD_CODE cmd, uint8 bnum , const MRF_IF *ifp);

typedef struct {
  const uint8 str[16];
  const uint8 cflags;
  const uint8 req_size;
  const uint8 rsp_size;
  const void *data;
  const MRF_CMD_FUNC func;
} MRF_CMD;

const MRF_CMD *mrf_cmd_ptr(uint8 type);

#endif
