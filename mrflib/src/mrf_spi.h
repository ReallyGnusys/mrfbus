#ifndef _MRF_SPI_INCLUDED_
#define _MRF_SPI_INCLUDED_

int mrf_spi_tx(uint8 tx_byte);
uint8 mrf_spi_rx();
int mrf_spi_flush_rx();
int mrf_spi_init();
int mrf_spi_data_avail();
#endif
