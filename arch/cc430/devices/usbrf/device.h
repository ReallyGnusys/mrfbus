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

#ifndef __DEVICE_INCLUDED__
#define __DEVICE_INCLUDED__


// device type usbrf - one usb and one rf interface

typedef  enum { UART0,
     	        RF0,
                NUM_INTERFACES} I_F;

// 8 buffs for allocation by sys
#define _MRF_BUFFS 8

#define IQUEUE_DEPTH 16

#endif
