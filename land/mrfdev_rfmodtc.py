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

from .mrf_sens import MrfSens
from .mrf_dev  import MrfDev
import datetime
import ctypes
from .mrf_structs import *
from .core_tests import mrf_cmd_app_test
from math import sqrt
from .mrflog import mrflog
from collections import deque

from .mrf_sens_relay import MrfSensRelay

#from mrfdev_pt1000 import PktRelayState, MrfSensPtRelay
MAX_RTDS = 1

class PktRfmodtcState(MrfStruct):
    _fields_ = [
        ("td",PktTimeDate),
        ("relay_cmd", c_uint8),
        ("relay_state", c_uint8),
        ("tempX100", c_uint32*MAX_RTDS)
        ]


mrf_app_cmd_test = 128
mrf_cmd_led_on   = 129
mrf_cmd_led_off   = 130
mrf_cmd_get_relay   = 131
mrf_cmd_set_relay   = 132
mrf_cmd_read_state   = 133


RfmodtcAppCmds = {
    mrf_cmd_app_test : {
        'name'  : "APP_TEST",
        'param' : None,
        'resp'  : PktTimeDate
    },
    mrf_cmd_led_on : {
        'name'  : "LED_ON",
        'param' : None,
        'resp'  : PktRelayState
    },
    mrf_cmd_get_relay : {
        'name'  : "LED_OFF",
        'param' : None,
        'resp'  : PktRelayState
    },
    mrf_cmd_get_relay : {
        'name'  : "GET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },
    mrf_cmd_set_relay : {
        'name'  : "SET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },

    mrf_cmd_read_state : {
        'name'  : "READ_STATE",
        'param' : None,
        'resp'  : PktRfmodtcState
    },


}



class MrfSensTempX100(MrfSens):
    _in_flds_ = [ ('date', PktTimeDate) ,
                  ('tempX100' , int) ]  # hmpff

    _out_flds_ = [ ('send_date' , datetime.datetime.now ),
                   ('recd_date' , datetime.datetime.now),
                   ('temp'      , float) ]

    _history_ =  { 'fields' : ['temp']
                 }

    _stype_    =  'temp'


    def filter(self, ntaps):
        self.ftaps = deque()

        for i in range(ntaps):
            self.ftaps.append(0)

    def filter_out(self,newval): # cheap and nasty filter option
        self.ftaps.popleft()
        self.ftaps.append(newval)
        tot = 0
        for v in self.ftaps:
            tot += v
        mo = 1.0 * tot / len(self.ftaps)
        return int(mo)

    def genout(self,indata):

        outdata = dict()
        #mrflog.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        outdata['send_date'] = indata['date'].to_datetime()
        outdata['recd_date'] = datetime.datetime.now()
        if not hasattr(self,'ftaps'):
            outdata['temp']  = indata['tempX100']/100.0
        else:
            outdata['temp']  = self.filter_out(int(indata['tempX100']))/100.0


        if (outdata['temp'] < -40.0)  or (outdata['temp'] > 140.0) :
            mrflog.error("%s  %s invalid temp %.2f, discarding"%( self.__class__.__name__, self.label,  outdata['temp']))
            return None

        if hasattr(self,'last_temp')  and (abs(self.last_temp - outdata['temp'])) > 40.0:  # no way should shift 20C in one reading
            mrflog.error("%s  %s  temp swing beyond limit got  %.2f  last was %.2f , discarding"%( self.__class__.__name__, self.label,  outdata['temp'],self.last_temp))
            return None
        #mrflog.info("%s gend output type %s data %s"%(self.__class__.__name__, type(outdata), outdata))
        self.last_temp = outdata['temp']
        return outdata





class DevRfmodtc(MrfDev):

    _capspec = {
        'temp' : MrfSensTempX100,
        'relay' : MrfSensRelay}
    _cmdset = RfmodtcAppCmds

    def init(self):
        self.filt_ind = 0
    def app_packet(self, hdr, param , resp):
        mrflog.info("%s app_packet type %s"%(self.__class__.__name__, type(resp)))

        mrflog.info("DevRfmodtc app_packet, hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))

        if param.type == mrf_cmd_read_state:

            for ch in range(len(resp.tempX100)):
                #mrflog.debug("chan %s milliohms %d type %s"%(ch, resp.milliohms[ch], type(resp.milliohms[ch])))
                inp = { 'date' : resp.td,
                        'tempX100' : resp.tempX100[ch]
                }
                self.caps['temp'][ch].input(inp)


            for ch in range(len(self.caps['relay'])):
                (resp.relay_state >> ch) & 0x01
                inp = { 'date' : resp.td,
                        'relay' :  (resp.relay_state >> ch) & 0x01
                }
                self.caps['relay'][ch].input(inp)


        elif  param.type == mrf_cmd_set_relay or param.type == mrf_cmd_get_relay:
            now = datetime.datetime.now()
            td =  PktTimeDate()
            td.set(now)
            inp = { 'date' : td,
                    'relay' :  resp.val
            }
            self.caps['relay'][resp.chan].input(inp)
