#include "mrf_types.h"

//#include "mrf_buff.h"
#include "mrf_if.h"
#include "mrf_debug.h"
#include "mrf_sys.h"
#include "mrf_cmd_def.h"
#include "mrf_buff.h"
#include "mrf_cmds.h"

/* each buffer can take a full mrfbus packet - 128 bytes */

static uint8 _mrf_buff[_MRF_BUFFS][_MRF_BUFFLEN];

static MRF_BUFF_STATE _mrf_buffst[_MRF_BUFFS];

//extern const MRF_CMD const mrf_cmds[];
void _mrf_buff_free(uint8 i){
  if (i < _MRF_BUFFS){
    _mrf_buffst[i].state = FREE;
    _mrf_buffst[i].owner = NUM_INTERFACES;    
  }
}


// this is where OS processes buffer

void mrf_buff_loaded_if(I_F owner, uint8 *buff,uint8 em){
  int i;
  //printf("\nmrf_buff_loaded_if owner %d buff %p\n",owner,buff);
  for ( i = 0 ; i < _MRF_BUFFS ; i++){
    //printf("i %d buff i %p bsowner %d\n",i,_mrf_buff[i],_mrf_buffst[i].owner);
    if((buff == &(_mrf_buff[i][0])) && (_mrf_buffst[i].owner == owner)){       
      _mrf_buffst[i].state = LOADED;
 
      _mrf_process_packet(owner,i);	
      _mrf_buff_free(i);
      return;
    }
}
  // if we're here it is an emergency buffer
  
  
}


void mrf_free(uint8* buff){
  int i;
  for ( i = 0 ; i < _MRF_BUFFS ; i++)
    if(buff == &(_mrf_buff[i][0])){
      _mrf_buff_free(i);
      return;
    }      
}

void _mrf_buff_clear_status(){

}

void mrf_buff_init(){
  uint8 i;
  for ( i = 0 ; i < _MRF_BUFFS ; i++)
    _mrf_buff_free(i);
  _mrf_buff_clear_status();
}
MRF_BUFF_STATE *_mrf_buff_state(uint8 bnum){
  if  ( bnum <  _MRF_BUFFS )
    return &(_mrf_buffst[bnum]);
  else
    return NULL;
}




uint8 *mrf_alloc_if(I_F i_f){
  int i;
  for ( i = 0 ; i < _MRF_BUFFS ; i++){
    if ( _mrf_buffst[i].state == FREE)
      {
	_mrf_buffst[i].state = LOADING;
	_mrf_buffst[i].owner = i_f;	
	return &(_mrf_buff[i][0]);
      }
  }
  return NULL;
}

uint8 *_mrf_buff_ptr(uint8 bind){
  return (uint8 *)_mrf_buff[bind];

}

