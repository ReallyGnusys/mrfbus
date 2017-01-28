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

#ifndef __MRF_APP_INCLUDED__
#define __MRF_APP_INCLUDED__

// define APP specific packets

typedef struct  __attribute__ ((packed))   {
  uint16 spi_rx_int_cnt;
  uint16 spi_tx_int_cnt; 
  uint16 spi_rx_bytes;
  uint16 spi_tx_bytes;
  uint16 spi_rxov_err;
  uint16 spi_rx_queue_level;
  uint16 spi_tx_queue_level;
  uint8  spi_rx_queue_data_avail;
  uint8  spi_tx_queue_data_avail;
  uint8  rxq_qip;
  uint8  rxq_qop;
  uint8  rxq_items;
  uint8  rxq_push_errors;
  uint8  rxq_pop_errors;
  uint8  txq_qip;
  uint8  txq_qop;
  uint8  txq_items;
  uint8  txq_push_errors;
  uint8  txq_pop_errors;
  uint8  ucb0_ifg;
  uint8  ucb0_ie;
  uint8  ucb0_cntrl0;
  uint8  ucb0_cntrl1;
  uint8  ucb0_stat;

  //uint16 pad2;


} MRF_PKT_SPI_DEBUG;


/* mrf_app_task_test
   returns current MRF_PKT_TIMEDATE 
*/
int mrf_spi_init_cc()  __attribute__ ((constructor));
MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_app_spi_read(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_app_spi_write(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_app_spi_debug(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);

#endif
