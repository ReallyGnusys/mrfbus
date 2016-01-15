#ifndef __MRF_BUFF_INCLUDED__
#define __MRF_BUFF_INCLUDED__

#include "mrf_sys_structs.h"

#include <mrf_if.h>

#include "device.h"

uint8  mrf_alloc_if(I_F i_f);
void mrf_free(uint8* buff);
uint8 mrf_buff_num_free();
void mrf_buff_loaded_if(I_F owner, uint8 *buff);

uint8 *_mrf_buff_ptr(uint8 bind);
MRF_BUFF_STATE *_mrf_buff_state(uint8 bnum);
I_F mrf_buff_owner(uint8 bnum);
void _mrf_buff_print();

//extern static uint8 _mrf_buff[_MRF_BUFFS][_MRF_BUFFLEN];
#endif
