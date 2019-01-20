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
import math
from mrfland import DateTimeFormat
import time

class MrfSens(object):
    _HISTORY_SECONDS_ = 60*60*24   # keep a day of history by default .. one minute averages
    #def __init__(self, label, address,channel,log):
    def __init__(self, label, devupdate, address, channel):
        self.label = label  ## fix me this should be tag - maybe should have label as well

        self.address = address  # useful for sensor/actuator to know it's address channel
        self.channel = channel
        self.devupdate = devupdate  # hmpfff... need way for actuators to send commands.
        self.outcount = 0
        self.dropcount = 0
        self.skey = 0
        self.subscribers = dict()
        self.mskey = 0
        self.minute_subscribers = dict()

        """ keep running average of all sensors """
        self.last_reading_time = time.time()  ## last time reading arrived
        self.last_value = {}
        self.last_avg_time = self.last_reading_time
        self.avg_tots = {}

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
            if fld[0].find("date") == -1:
                self.last_value[fld[0]] = fld[1]()
                self.avg_tots[fld[0]] = 0.0

        # crude generic history
        """
        if hasattr(self,'_history_') :
            import datetime
            self.history = { 'ts' : []}  # buffer for last minutes readings
            self.history_dt = None #datetime.datetime.now()  # we're going to average readings at end of each minute
            #self.history_dt = self.history_dt.replace(second = 0,microsecond = 0)
            self.averages = { 'ts' : []} # each minute averages updated ... hmpff
            self.avg_types = {}
            for fld in self._history_['fields']:
                self.history[fld] = []
                self.averages[fld] = []
                self.avg_types[fld] = type(self.output[fld])
        """
        # call optional subclass init

        if hasattr(self,'init'):
            self.init()

    def out_data_flds(self):
        """ out_flds except dates """
        flds = []
        for f in self._out_flds_:
            if f[0].find("_date") == -1:
                flds.append(f[0])
        return flds

    def subscribe(self,callback):  # subscribe for all changes
        key = self.skey
        self.subscribers[key] = callback
        self.skey += 1
        return key

    def subscribe_minutes(self,callback):  # subscribe for minutely averages
        key = self.mskey
        self.minute_subscribers[key] = callback
        self.mskey += 1
        mrflog.warn("subscribe_minutes for sensor %s"%(self.label))
        return key

    def update_avg_totals(self):
        """ accumulate time*reading for average calculation """
        new_time = time.time()
        dsecs = new_time - self.last_reading_time
        self.last_reading_time = new_time
        for fld in self._out_flds_:
            if fld[0].find("_date") == -1:
                self.avg_tots[fld[0]] += dsecs * self.last_value[fld[0]]
                self.last_value[fld[0]] = self.output[fld[0]]

    def gen_average(self):
        """ called by regmanager each minute """
        self.update_avg_totals()
        periodsecs = self.last_reading_time - self.last_avg_time
        self.last_avg_time = self.last_reading_time
        avg = OrderedDict()
        for fld in self._out_flds_:
            if fld[0].find("_date") == -1:
                favg = fld[1](self.avg_tots[fld[0]] / periodsecs)
                avg[fld[0]] = favg
                self.avg_tots[fld[0]] = 0.0
        mrflog.debug("%s 0x%x avg %s "%(self.__class__.__name__,self.address,repr(avg)))
        return avg


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

        odata = self.genout(indata)


        if odata == None:
            mrflog.warn ("%s no output data generated for  %s"%(self.__class__.__name__, self.label ))
            self.dropcount +=1
            return None
        else:
            self.outcount += 1

        # output sanity check for keys and types

        for ditem in odata.keys():
            if not self._output.has_key(ditem):
                mrflog.error("%s input invalid odata , no key %s in %s"%(self.__class__.__name__, ditem, repr(odata)))
                return None
            if type(odata[ditem]) != type(self._output[ditem]()) :
                mrflog.error("%s input output data type mismatch for key %s  %s vs %s"%(self.__class__.__name__, ditem, type(odata[ditem]), type(self._output[ditem]) ))
                return None

        mrflog.debug("new output for sens %s - %s"%(self.label,repr(odata)))
        if False and self.__class__.__name__ == 'MrfSensMemory' and self.address == 0x02:
            mrflog.warn("new output for sens %s - %s  num subscribers %d"%(self.label,repr(odata),len(self.subscribers)))
        else:
            mrflog.debug("new output for sens %s - %s  num subscribers %d"%(self.label,repr(odata),len(self.subscribers)))

        self.output = odata

        # new history

        self.update_avg_totals()

        # run subscriber callbacks with new output
        for s in self.subscribers.keys():
            self.subscribers[s](self.label,self.output)
