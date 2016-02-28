#ifndef __MRF_APP_INCLUDED__
#define __MRF_APP_INCLUDED__
/* default app , no new pkts defined 
 */

/* mrf_app_task_test
   returns current MRF_PKT_TIMEDATE 
*/
int mrf_spi_init_cc()  __attribute__ ((constructor));
MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_app_spi_read(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);
MRF_CMD_RES mrf_app_spi_write(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp);

#endif
