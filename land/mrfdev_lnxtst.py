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

from mrf_sens import MrfSens
from mrf_dev  import MrfDev
import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test
from math import sqrt
from mrflog import mrflog
from collections import deque

from mrf_sens_relay import MrfSensRelay, PktRelayState # FIXME should be in shared lib

class PktLnxMemStats(MrfStruct):
    _fields_ = [
        ("td",PktTimeDate),
        ("relay_state", c_uint16),
        ("sz",c_uint32),
        ("res",c_uint32),
        ("share",c_uint32),
        ("text",c_uint32),
        ("lib",c_uint32),
        ("data",c_uint32),
        ("dt",c_uint32)
        ]

mrf_app_cmd_test = 128
mrf_app_cmd_mstats = 129
mrf_cmd_get_relay   = 130
mrf_cmd_set_relay   = 131


LnxtstAppCmds = {
    mrf_app_cmd_test : {
        'name'  : "APP_TEST",
        'param' : None,
        'resp'  : PktTimeDate
    },
    mrf_app_cmd_mstats : {
        'name'  : "MEM_STATS",
        'param' : None,
        'resp'  : PktLnxMemStats
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
    }

}

class MrfSensMemory(MrfSens):
    _in_flds_ = [ ('date', PktTimeDate) ,
                  ('sz' , int),
                  ('res' , int),
                  ('share' , int),
                  ('text' , int),
                  ('lib' , int),
                  ('data' , int),
                  ('dt' , int) ]

    _out_flds_ = [ ('send_date' , datetime.datetime.now ),
                   ('recd_date' , datetime.datetime.now),
                   ('memory',int),  # FIXME - only field with name matching sensor working for graphs ATM
                   ('sz' , int),
                   ('res' , int),
                   ('share' , int),
                   ('text' , int),
                   ('lib' , int),
                   ('data' , int),
                   ('dt' , int) ]

    _history_ =  { 'fields' : ['memory'] }
    _stype_ = 'memory'

    def init(self):
        return

    def genout(self,indata):
        outdata = dict()
        #mrflog.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        outdata['send_date'] = indata['date'].to_datetime()
        outdata['recd_date'] = datetime.datetime.now()
        #mrflog.warn("%s setting output for out flds %s"%(self.__class__.__name__,repr(self.out_data_flds())))
        #mrflog.warn("%s _out_flds_  %s"%(self.__class__.__name__,repr(self._out_flds_)))

        for f in self.out_data_flds():
            #mrflog.warn("%s setting output for fld %s"%(self.__class__.__name__,f))
            if f == 'memory':
                outdata[f] = indata['res']
            else:
                outdata[f] = indata[f]
        #mrflog.warn("outdata : "+repr(outdata))
        return outdata



class DevLnxtst(MrfDev):

    _capspec = {
            'memory' : MrfSensMemory,
            'relay'  : MrfSensRelay
    }

    _cmdset = LnxtstAppCmds


    def __init__(self, rm, label, address, caplabels={}):
        mrflog.debug("%s __init__ entry , label %s address 0x%x"%(self.__class__.__name__,label,address))

        if caplabels == {}:
            caplabels = {'memory' : [label]}
            mrflog.debug("no caplabels - setting to %s"%repr(caplabels))
        return super(DevLnxtst,self).__init__(rm,label, address, caplabels)
    def app_packet(self, hdr, param , resp):
        mrflog.info("%s app_packet type %s"%(self.__class__.__name__, type(resp)))

        mrflog.info("LnxtstDev app_packet, hdr %s param %s resp %s %s"%(repr(hdr), repr(param),resp.__class__.__name__, repr(resp)))


        if param.type == mrf_app_cmd_mstats:
            inp = { }

            tdic = resp.dic()

            mrflog.debug("LnxtstDev 0x%x app_packet,  mrf_app_cmd_mstats %s \n orig %s"%(self.address,repr(tdic),repr(resp)))

            #mrflog.warn("caps[memory]  %s"%repr(self.caps['memory']))
            for attn in tdic:

                if attn == 'td':
                    inp['date'] = resp.td
                elif attn == 'relay_state':  # dirty hack to process relay state
                    mrflog.debug("addr 0x%x : mstats.relay_state 0x%x"%(self.address,resp.relay_state))
                    for ch in range(len(self.caps['relay'])):
                        #(resp.relay_state >> ch) & 0x01
                        rinp = { 'date' : resp.td,
                                 'relay' :  int((int(resp.relay_state) >> ch) & 0x01)
                        }
                        self.caps['relay'][ch].input(rinp)
                else:
                    inp[attn] = int(tdic[attn]) # FIXME should get type from field decs

            #mrflog.warn("inp %s"%(repr(inp)))
            self.caps['memory'][0].input(inp)

        elif  param.type == mrf_cmd_set_relay or param.type == mrf_cmd_get_relay:
            now = datetime.datetime.now()
            td =  PktTimeDate()
            td.set(now)
            inp = { 'date'  :  td,
                    'relay' :  resp.val
            }
            mrflog.debug("LnxtstDev 0x%x app_packet relay_cmd_0x%x %s \n orig %s"%(self.address,param.type,repr(inp),repr(resp)))

            self.caps['relay'][resp.chan].input(inp)
