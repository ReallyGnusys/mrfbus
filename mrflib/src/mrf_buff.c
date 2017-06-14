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

#include "mrf_types.h"

//#include "mrf_buff.h"
#include "mrf_if.h"
#include "mrf_debug.h"
#include "mrf_sys.h"
#include "mrf_sys_cmds.h"
#include "mrf_buff.h"

/* each buffer can take a full mrfbus packet - 128 bytes */

static uint8 _mrf_buff[_MRF_BUFFS][_MRF_BUFFLEN];

static MRF_BUFF_STATE _mrf_buffst[_MRF_BUFFS];

const uint8 _mrf_buffs = _MRF_BUFFS;
//extern const MRF_CMD const mrf_cmds[];
void _mrf_buff_free(uint8 i){
  if (i < _MRF_BUFFS){
    _mrf_buffst[i].state = FREE;
    _mrf_buffst[i].owner = NUM_INTERFACES;   
    mrf_debug("_mrf_buff_free : free buff %d  %d/%d buffs free\n",i,mrf_buff_num_free(),_MRF_BUFFS);

    return;
  }
  mrf_debug("_mrf_buff_free error buff num was %d\n",i);
}


// this is where OS processes buffer
int mrf_buff_loaded(uint8 bnum){
  const MRF_IF *ifp;
  if (bnum <  _MRF_BUFFS){
    mrf_debug("mrf_buff_loaded bnum entry  %u owner %u\n",bnum, _mrf_buffst[bnum].owner);
    ifp = mrf_if_ptr( _mrf_buffst[bnum].owner);
    mrf_debug("ifp = %p\n",ifp);
    ifp->status->stats.rx_pkts += 1;
    _mrf_buffst[bnum].state = LOADED;
    //_mrf_buff_print();
    _mrf_process_buff(bnum);
    mrf_debug("%s","mrf_buff_loaded exit\n");
    //_mrf_buff_print();

    return 0;
  } else {
    return -1;
  }
}


void mrf_free(uint8* buff){
  int i;
  for ( i = 0 ; i < _MRF_BUFFS ; i++)
    if(buff == &(_mrf_buff[i][0])){
      _mrf_buff_free(i);
      mrf_debug("mrf_free : free buff %d  %d/%d buffs free\n",i,mrf_buff_num_free(),_MRF_BUFFS);
      return;
    }
  mrf_debug("%s","ERROR : buff was not found or freed\n\n");
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

const char * bstnames[] = {"FREE","LOADING","LOADED","TXQUEUE","TX","APPIN"}; // ugly duplication of enum tags

const char illegal_state_warning[] = "ILLEGAL_STATE ERROR";

const char * mrf_buff_state_name(uint8 bnum){
  MRF_BUFF_STATE *bst = _mrf_buff_state(bnum);
  if (bst == NULL)
    return illegal_state_warning;
  else
    return bstnames[bst->state];
}





void _mrf_buff_print(){
  int i;
  mrf_debug("_mrf_buff_print warning is %s\n",illegal_state_warning);
  mrf_debug("free?  %s\n",mrf_buff_state_name(0));
  for ( i = 0 ; i < _MRF_BUFFS ; i++){
    MRF_BUFF_STATE *bst = _mrf_buff_state(i);
    mrf_debug("buff %d state %d (%s) owner %d\n",i,bst->state,mrf_buff_state_name(i),_mrf_buffst[i].owner);    
  }
}

uint8 mrf_buff_num_free(){
  uint8 i,free = 0;
  // _mrf_buff_print();
  for ( i = 0 ; i < _MRF_BUFFS ; i++)
    if ( _mrf_buffst[i].state == FREE)
      free++;
  return free;
}

uint8 mrf_alloc_if(I_F i_f){
  uint8 i;
  mrf_debug("mrf_alloc_if entry i_f %d\n",i_f);
  //_mrf_buff_print();
  for ( i = 0 ; i < _MRF_BUFFS ; i++){
    if ( _mrf_buffst[i].state == FREE)
      {
	_mrf_buffst[i].state = LOADING;
	_mrf_buffst[i].owner = i_f;	
        mrf_debug("mrf_alloc_if : alloc buff %d to i_f %d\n",i,i_f);
        return i;
      }
  }
  mrf_debug("mrf_alloc_if : ERROR failed to alloc buffer for i/f %d!!!\n",i_f);
 
  return _MRF_BUFFS;
}

I_F mrf_buff_owner(uint8 bnum){
  if (bnum < _MRF_BUFFS){
    return _mrf_buffst[bnum].owner;
  } else {
    return NUM_INTERFACES;
  }  
} 

uint8 *mrf_alloc_if_tbd(I_F i_f){
  int i;
  _mrf_buff_print();
  for ( i = 0 ; i < _MRF_BUFFS ; i++){
    if ( _mrf_buffst[i].state == FREE)
      {
	_mrf_buffst[i].state = LOADING;
	_mrf_buffst[i].owner = i_f;	
        mrf_debug("mrf_alloc_if_tbd : alloc buff %d to i_f %d\n",i,i_f);
	return &(_mrf_buff[i][0]);
      }
  }
  return NULL;
}

uint8 *_mrf_buff_ptr(uint8 bind){
  if (bind < _MRF_BUFFS)
    return (uint8 *)_mrf_buff[bind];
  else
    return NULL;

}

