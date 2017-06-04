
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


GCC=msp430-gcc
LD=msp430-ld
OD=msp430-objdump -dt
SIZE=msp430-size
AOBJS=${MOBJ}/mrf_rf.o  ${MOBJ}/_mrf_rf1.o ${MOBJ}/clock.o ${MOBJ}/hal_pmm.o ${MOBJ}/RfRegSettings.o \
	${MOBJ}/mrf_uart_cc.o  ${MOBJ}/rtc_arch.o

#ifdef MRF_SPI_B
#      ${MOBJ}/mrf_spi_cc.o
#endif


ACCFLAGS= -mmcu=cc430f5137  -DMHZ_868 

# -g
