#!/usr/bin/env python
'''  Copyright (c) 2012-17 Gnusys Ltd

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

from mrf_structs import *



class MrfDevCap(object):
    """ 
    capability of a device - e.g. temp sensing array , actuator array - array is assumed - index translates to channel throughout
    """
    def __init__(self, label, address, stype, slabels, log):
        self.label = label
        self.address = address
        self.log = log
        self._channels = []

        for chan in xrange(len(slabels)):
            self._channels.append(stype(slabels[chan],address,chan,log))

        self.num_chans = len(self._channels)  # slightly redundant
            


class MrfDev(object):
    """  
    base class : encapsulates physical mrfbus device which might have multiple
    sensors/ actuators

    cmdset and dspec effectively define MrfBus physical device running a specific app

    """
    def __init__(self, label, address, caplabels ,log):
        self.address = address
        self.label = label
        self.sys = {} # aims to contain all sys info responses from device_info upwards
        self.log = log
        self.skey = 0
        self.lastmsgid = -1
        self.subscribers = dict()

        self.caps = {}

        for clab in self._capspec.keys():
            if not caplabels.has_key(clab):
                self.log.error("%s spec labels error  no key %s in %s"%(self.__class__.__name__, clab, repr(caplabels)))
                return
            self.caps[clab] = []
            chan = 0
            for slab in caplabels[clab]:
                self.caps[clab].append(self._capspec[clab](slab, self.address, chan, self.log))
                chan += 1
            
        
    def subscribe(self,callback):
        key = self.skey
        self.subscribers[key] = callback
        self.skey += 1
        return key
               
                
    def packet(self,hdr,rsp):  # server interface - all packets from this device are sent here
        self.log.warn("MrfDev packet for label %s addr %s"%(self.label,self.address))
        if hdr.usrc != self.address :
            self.log.error("MrfDev %s addr %s got wrong fyi"%(self.label,self.address))
            return None, None

        ## try and catch duplicate msgids - retransmissions , pending debug.. hmpff
        if hdr.msgid == self.lastmsgid:
            self.log.error("MrfDev %s addr %s duplicate msgid %d"%(self.label,self.address,hdr.msgid))
            return None , None
        self.lastmsgid = hdr.msgid
        
        param = MrfSysCmds[hdr.type]['param']()
        #print "have param type %s"%type(param)
        param_data = bytes(rsp)[len(hdr):len(hdr)+len(param)]
        param.load(param_data)
        #print "resp should be %s"%repr(param)
        respdat = bytes(rsp)[len(hdr)+len(param):]

        #self.log.info(" have response or struct object %s"%repr(param))

        resp = mrf_decode_buff(param.type,respdat)
        if resp:  # it's a sys command response - always keep most up to date copies
            # FIXME should do some validation of the decode here
            self.sys[param.type] = resp
                           
        else:
            resp = mrf_decode_buff(param.type, respdat, cmdset=self._cmdset)
        if not resp:
            self.log.error("%s failed to decode packet , hdr was %s"%(self.__class__.__name__,repr(hdr)))
            return
        self.app_packet(hdr,param,resp)  # this must be defined in derived class

        return param, resp
        
        
        
            


class MrfSens(object):    
    def __init__(self, label, address,channel,log):
        self.label = label  ## fix me this should be tag - maybe should have label as well
        self.address = address        
        self.channel = channel
        self.log = log
        self.inval = None
        self.skey = 0
        self.subscribers = dict()

        self._input = OrderedDict()
        for fld in self._in_flds_:
            self._input[fld[0]] = fld[1]

        self._output = OrderedDict()
        for fld in self._out_flds_:
            self._output[fld[0]] = fld[1]
            
    
        
    def subscribe(self,callback):
        key = self.skey
        self.subscribers[key] = callback
        self.skey += 1
        return key

    def input(self, indata):
        # input sanity check for keys and types 
        #self.log.info("new item for sens %s - %s"%(self.label,repr(indata)))
        for ditem in indata.keys():
            if not self._input.has_key(ditem):
                self.log.error("%s input invalid indata , no key %s in %s"%(self.__class__.__name__, ditem, repr(indata)))
                return None
            if type(indata[ditem]) != type(self._input[ditem]()) :
                self.log.error("%s input indata type mismatch for key %s  %s vs %s"%(self.__class__.__name__, ditem, type(indata[ditem]), type(self._input[ditem]()) ))
                return None

        odata = self.genout(indata,{})
        # output sanity check for keys and types 

        for ditem in odata.keys():
            if not self._output.has_key(ditem):
                self.log.error("%s input invalid odata , no key %s in %s"%(self.__class__.__name__, ditem, repr(odata)))
                return None
            if type(odata[ditem]) != type(self._output[ditem]()) :
                self.log.error("%s input output data type mismatch for key %s  %s vs %s"%(self.__class__.__name__, ditem, type(odata[ditem]), type(self._output[ditem]) ))
                return None

        self.log.info("new item for sens %s - %s"%(self.label,repr(odata)))

        self.output = odata
        

        for s in self.subscribers.keys():
            self.subscribers[s](self.label,self.output)
