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


        # crude generic history
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

        self.output = odata

        # update history if specified

        if hasattr(self, "history"):
            if indata.has_key('date'):
                now = indata['date'].to_datetime()  # it had better be PktTimeDate!
                ournow = datetime.datetime.now()

                if (ournow - now) > datetime.timedelta(minutes = 2) or   (now - ournow) > datetime.timedelta(minutes = 2):
                    mrflog.warn("sensor input date is not set ( got %s from %s)... setting current"%(repr(now),repr(indata['date'])))
                    now = ournow


            else:
                now = datetime.datetime.now()
            if self.history_dt == None:
                self.history_dt = now
                self.history_dt = self.history_dt.replace(second = 0,microsecond = 0)

            if (now > self.history_dt) and (now.minute != self.history_dt.minute):  # roll up last minutes readings
                mrflog.info("averaging minute for sensor %s history_dt %s now %s"%
                            (self.label,repr(self.history_dt),repr(now)))
                if (now - self.history_dt) >= datetime.timedelta(minutes = 2):
                    #FIXME! should pad from last know value here, then build average for minute
                    mrflog.error("%s %s history not updated for %s"%(self.__class__.__name__,
                                                                     self.label,
                                                                     repr(now.minute - self.history_dt.minute)))
                    mrflog.error("now %s  self.history_dt %s"%(now,self.history_dt))
                    mrflog.error("ts = %s"%repr(self.history['ts']))

                avm = {}
                for hfld in self._history_['fields']:
                    if len(self.history['ts']):
                        tot = self.avg_types[hfld](0)
                        for val in self.history[hfld]:
                            tot += val
                        tot = float(tot)
                        avg1 = tot/float(len(self.history['ts']))
                        avg2 = math.floor(avg1*100)/100

                        avg = self.avg_types[hfld](avg2)
                        mrflog.info("averaging sensor %s  %s  %.2f tot %.2f avg1  %.2f avg2  %.2f "%
                                    (self.label,hfld,avg,tot,avg1,avg2))
                    else:
                        avg = self.avg_types[hfld](0)
                    self.averages[hfld].append(avg)
                    avm[hfld] = avg
                    self.history[hfld] = []
                    mrflog.info("history[%s] len is %d"%(hfld,len(self.history[hfld])))

                self.history['ts'] = []
                avm['ts'] = self.history_dt.strftime(DateTimeFormat)
                self.averages['ts'].append(avm['ts'])

                mrflog.info("history len is %d averages len %d"%(len(self.history['ts']),len(self.averages['ts'])))
                mrflog.info("%s"%repr(self.averages))
                for s in self.minute_subscribers.keys():
                    self.minute_subscribers[s](self.label,avm)

                nowm = now
                nowm = nowm.replace(second=0,microsecond=0)



                self.history_dt += datetime.timedelta(minutes = 1)
                if False and self.label == 'LOUNGE_AMBIENT':
                    mrflog.warn("minute_subscribers %s updated with %s hdt is now %s nowm %s"%(self.label,repr(avm),repr(self.history_dt),repr(nowm)))

                while self.history_dt < nowm:
                     avm['ts']  = self.history_dt.strftime("%Y-%m-%dT%H:%M:%S")
                     for hfld in self._history_['fields']:
                         self.averages[hfld].append(avm[hfld])
                     self.averages['ts'].append(avm['ts'])
                     mrflog.warn("%s padding sensor average %s"%(self.label,repr(avm)))
                     for s in self.minute_subscribers.keys():
                         self.minute_subscribers[s](self.label,avm)
                     self.history_dt += datetime.timedelta(minutes = 1)

                #self.history_dt = self.history_dt.replace(second = 0,microsecond = 0)

                # trim old history from front of array
                binit = True
                while binit:
                    firstts = self.averages['ts'][0]
                    ftd = datetime.datetime.strptime(firstts,'%Y-%m-%dT%H:%M:%S')
                    tdel = now - ftd
                    if tdel.total_seconds() > self._HISTORY_SECONDS_:
                        mrflog.info("%s discarding average ts %s"%(self.label,firstts))
                        del self.averages['ts'][0]
                        for hfld in  self._history_['fields']:
                            del self.averages[hfld][0]
                    else:
                        binit = False



            self.history['ts'].append(now.strftime("%Y-%m-%dT%H:%M:%S"))
            for hfld in self._history_['fields']:
                self.history[hfld].append(self.output[hfld])

        for s in self.subscribers.keys():
            self.subscribers[s](self.label,self.output)
