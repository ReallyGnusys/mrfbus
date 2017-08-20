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


class MrfSens(object):    
    #def __init__(self, label, address,channel,log):
    def __init__(self, label, devupdate, address, channel):        
        self.label = label  ## fix me this should be tag - maybe should have label as well

        self.address = address  # useful for sensor/actuator to know it's address channel
        self.channel = channel
        self.devupdate = devupdate  # hmpfff... need way for actuators to send commands.
        self.inval = None
        self.skey = 0
        self.subscribers = dict()

        """ below are prototypes to type check
            and use OrderedDict instead of list descs at runtime
        """
        self._input = OrderedDict()
        for fld in self._in_flds_:
            self._input[fld[0]] = fld[1]

        self._output = OrderedDict()
        # and also generate initial output value
        self.output = OrderedDict()
        for fld in self._out_flds_:
            self._output[fld[0]] = fld[1]
            self.output[fld[0]] = fld[1]()

        # call optional subclass init

        if hasattr(self,'init'):
            self.init()
        
    def subscribe(self,callback):
        key = self.skey
        self.subscribers[key] = callback
        self.skey += 1
        return key

    def input(self, indata):
        # input sanity check for keys and types 
        #mrflog.info("new item for sens %s - %s"%(self.label,repr(indata)))
        for ditem in indata.keys():
            if not self._input.has_key(ditem):
                mrflog.error("%s input invalid indata , no key %s in %s"%(self.__class__.__name__, ditem, repr(indata)))
                return None
            if type(indata[ditem]) != type(self._input[ditem]()) :
                mrflog.error("%s input indata type mismatch for key %s  %s vs %s"%(self.__class__.__name__, ditem, type(indata[ditem]), type(self._input[ditem]()) ))
                return None

        odata = self.genout(indata, self.output)
        # output sanity check for keys and types 

        for ditem in odata.keys():
            if not self._output.has_key(ditem):
                mrflog.error("%s input invalid odata , no key %s in %s"%(self.__class__.__name__, ditem, repr(odata)))
                return None
            if type(odata[ditem]) != type(self._output[ditem]()) :
                mrflog.error("%s input output data type mismatch for key %s  %s vs %s"%(self.__class__.__name__, ditem, type(odata[ditem]), type(self._output[ditem]) ))
                return None

        mrflog.info("new item for sens %s - %s"%(self.label,repr(odata)))

        self.output = odata
        

        for s in self.subscribers.keys():
            self.subscribers[s](self.label,self.output)
