#ifndef _MRF_ARCH_INCLUDED_
#define _MRF_ARCH_INCLUDED_

#define SOCKET_DIR "/tmp/mrf_bus/"

int mrf_arch_app_callback(int fd, MRF_APP_CALLBACK callback);
void _mrf_print_hex_buff(uint8 *buff,uint16 len);

#endif
