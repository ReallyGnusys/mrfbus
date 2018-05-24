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

#ifndef __MRF_BUFF_INCLUDED__
#define __MRF_BUFF_INCLUDED__

#include "mrf_sys_structs.h"

#include <mrf_if.h>

#include "device.h"

uint8  mrf_alloc_if(I_F i_f);
void mrf_free(uint8* buff);
uint8 mrf_buff_num_free();
void _mrf_buff_free(uint8 i);

void mrf_buff_loaded_if(I_F owner, uint8 *buff);
int mrf_buff_loaded(uint8 bnum);

uint8 *_mrf_buff_ptr(uint8 bind);
MRF_BUFF_STATE *_mrf_buff_state(uint8 bnum);
I_F mrf_buff_owner(uint8 bnum);
void _mrf_buff_print();
const char * mrf_buff_state_name(uint8 bnum);

//extern static uint8 _mrf_buff[_MRF_BUFFS][_MRF_BUFFLEN];
#endif
