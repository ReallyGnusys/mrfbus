###############################################################################
# Copyright (C) 2012-2016 Gnusys Ltd
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################


GCC=g++
LD=g++
#GCC=gcc
#LD=gcc

OD=objdump -dt
SIZE=size
AOBJS=${MOBJ}/mrf_pipe_lnx.o ${MOBJ}/mrf_uart_lnx.o
ALDFLAGS=-lrt
