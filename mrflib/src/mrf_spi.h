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

#ifndef _MRF_SPI_INCLUDED_
#define _MRF_SPI_INCLUDED_

int mrf_spi_tx(uint8 tx_byte);
uint8 mrf_spi_rx();
int mrf_spi_flush_rx();
int mrf_spi_init();
int mrf_spi_data_avail();  // rx data
int mrf_spi_tx_data_avail();
int mrf_spi_tx_queue_items();
int mrf_spi_rx_queue_items();
IQUEUE *mrf_spi_tx_queue();
IQUEUE *mrf_spi_rx_queue();
#endif
