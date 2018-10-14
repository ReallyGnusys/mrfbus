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
from mrflog import mrflog



class MrfDev(object):
    """
    base class : encapsulates physical mrfbus device which might have multiple
    sensors/ actuators

    cmdset and dspec effectively define MrfBus physical device running a specific app

    """
    def __init__(self, rm, label, address, caplabels={}):
        mrflog.warn("%s __init__ entry , label %s address 0x%x"%(self.__class__.__name__,label,address))
        self.address = address
        self.label = label
        self.rm = rm
        self.sys = {} # aims to contain all sys info responses from device_info upwards
        self.skey = 0
        self.lastmsgid = -1
        self.lasthdr = None
        self.subscribers = dict()

        self.caps = {}

        # build lookup of command names
        self.cmdnames = {}

        for ccode in self._cmdset.keys():
            self.cmdnames[self._cmdset[ccode]['name']] = ccode

        # construct sensors and actuators from capability spec
        for clab in self._capspec.keys():
            if not caplabels.has_key(clab):
                mrflog.error("%s spec labels error  no key %s in %s"%(self.__class__.__name__, clab, repr(caplabels)))
                return
            self.caps[clab] = []
            chan = 0
            for slab in caplabels[clab]:
                mrflog.warn("%s elaborating sensor %s  %s"%(self.__class__.__name__, clab, slab))
                #self.caps[clab].append(self._capspec[clab](slab, self.address, chan, mrflog))
                self.caps[clab].append(self._capspec[clab](slab,self.devupdate,self.address,chan))
                self.rm.senslink(slab, self.address ,chan)
                chan += 1
            mrflog.warn(self.caps[clab])

        if hasattr(self, 'init'):  # run subclass init if defined
            self.init()
        self.rm.device_register(self)

    def devupdate(self, cmd, data = {}):
        mrflog.warn("%s devupdate dest %s cmd %s data %s"%(self.__class__.__name__, self.address,cmd , repr(data)))
        self.rm.devupdaten(self.label, self.address, cmd, data)

    def subscribe(self,callback):
        key = self.skey
        self.subscribers[key] = callback
        self.skey += 1
        return key


    def packet(self,hdr,rsp):  # server interface - all packets from this device are sent here
        mrflog.info("MrfDev packet for label %s addr %s"%(self.label,self.address))
        if hdr.usrc != self.address :
            mrflog.error("MrfDev %s addr %s got wrong packet"%(self.label,self.address))
            return None, None

        ## try and catch duplicate msgids - retransmissions , pending debug.. hmpff
        if hdr.msgid == self.lastmsgid:
            mrflog.error("MrfDev %s addr %s duplicate msgid %d - discarding"%(self.label,self.address,hdr.msgid))
            mrflog.error(repr(hdr))
            mrflog.error("lasthdr:")
            mrflog.error(repr(self.lasthdr))

            return None , None
        self.lastmsgid = hdr.msgid
        self.lasthdr = hdr


        param = MrfSysCmds[hdr.type]['param']()
        #print "have param type %s"%type(param)
        param_data = bytes(rsp)[len(hdr):len(hdr)+len(param)]
        param.load(param_data)
        #print "resp should be %s"%repr(param)
        respdat = bytes(rsp)[len(hdr)+len(param):]



        mrflog.info(" have response or struct object %s"%repr(param))

        resp = mrf_decode_buff(param.type,respdat)
        if resp:  # it's a sys command response - always keep most up to date copies
            # FIXME should do some validation of the decode here
            di = dict(resp.dic())
            self.sys[param.type] = di
            mrflog.warn("have sys command %s  type %s  resp %s"%(repr(param.type), type(di), repr(di)))
            for s in self.subscribers.keys():  # for now only update subscribers for sys command responses
                self.subscribers[s](self.label,self.sys)

        else:
            resp = mrf_decode_buff(param.type, respdat, cmdset=self._cmdset)
        if not resp:
            mrflog.error("%s failed to decode packet , hdr was %s"%(self.__class__.__name__,repr(hdr)))
            return

        mrflog.info(" calling app packet with resp %s"%repr(resp))
        self.app_packet(hdr,param,resp)  # this must be defined in derived class


        return param, resp
