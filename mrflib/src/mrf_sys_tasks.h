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

#ifndef __MRF_SYS_TASKS_INCL__
#define __MRF_SYS_TASKS_INCL__
#include <mrf_sys.h>

MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_retry(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_device_status(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_if_status(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_set_time(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_buff_state(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_cmd_info(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_app_cmd_info(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_test_1(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
//MRF_CMD_RES mrf_task_test_2(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_usr_struct(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_reset(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);
MRF_CMD_RES mrf_task_ping(MRF_CMD_CODE cmd,uint8 bnum, const MRF_IF *ifp);

#endif
