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
#ifndef __MQUEUE_INCLUDED__
#define __MQUEUE_INCLUDED__

#include "mrf_types.h"
//#include "device.h"  // for MQUEUE_DEPTH

//typedef struct volatile {
#define MQUEUE_DEPTH 4

template <class T> 

class MQueue

{ 
 public:
  MQueue();
  int full();
  int items();

  int data_avail();
  int push(const T& data);
  const T* pop();
  int flush();
  const T& head();


 private:
  volatile uint8 qip;
  volatile uint8 qop;
  volatile uint8 nitems;
  volatile uint8 push_errors;
  volatile uint8 pop_errors;
  volatile uint32 buffer[MQUEUE_DEPTH];
  void _q_elem_set(const T& val);
};


template <class T> 

MQueue<T>::MQueue(){
  qip = 0;
  qop = 0;
  nitems = 0;  // FIXME kind of pointless ..
  push_errors = 0;
  pop_errors = 0;
}

template <class T> 

int MQueue<T>::items(){
  int _nitems;

  if (qip >= qop)
    _nitems = qip - qop;
  else
    _nitems = (MQUEUE_DEPTH*2 + qip) -  qop;
  return _nitems;
}

template <class T> 

int MQueue<T>::flush(){
  int flushed = items();
  
  nitems = 0;  //FIXME shouldn't need this field - use function above 
  qop = 0;
  qip = 0;
  
  //return (items == (MQUEUE_DEPTH -1 ));
  return flushed;
}
template <class T> 

int MQueue<T>::full(){
  return ( items() >= MQUEUE_DEPTH);  
  //return ((qip+1) % (MQUEUE_DEPTH*2)) == qop;
}
template <class T> 

int MQueue<T>::data_avail(){
  return (qip != qop);
}


// sets elem for entry qip

template <class T> 

void MQueue<T>::_q_elem_set(const T& val){
  buffer[qip % MQUEUE_DEPTH] = val;
}


template <class T> 

const T& MQueue<T>::head(){
  return buffer[qop % MQUEUE_DEPTH];
}

template <class T> 

int MQueue<T>::push(const T& data){
  if (full()){
    push_errors++;
    return -1;
  }
  _q_elem_set(data);  
  qip = (qip + 1) % (MQUEUE_DEPTH * 2);
  nitems++;
  return 0;
}

template <class T> 

const T *MQueue<T>::pop(){
  if (data_avail() == 0){
    pop_errors++;
    return (const T *)NULL;
  }
  const T& data = head();
  qop = (qop + 1) % (MQUEUE_DEPTH * 2);
  nitems--;
  return (const T *)data;
}



#endif
