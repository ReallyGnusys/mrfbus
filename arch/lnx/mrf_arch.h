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

#ifndef _MRF_ARCH_INCLUDED_
#define _MRF_ARCH_INCLUDED_

#define SOCKET_DIR "/tmp/mrf_bus/"

int mrf_arch_app_callback(MRF_APP_CALLBACK callback);
int mrf_arch_servfd();


void _mrf_print_hex_buff(uint8 *buff,uint16 len);
int is_hex_digit(uint8 dig);
uint8 int_to_hex_digit(int in);
long long mrf_timestamp();
  
#endif
