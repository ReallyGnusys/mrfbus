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


class PktLnxMemStats(MrfStruct):
    _fields_ = [
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
    }
}

class MrfSensMemory(MrfSens):
    _in_flds_ = [ ('date', PktTimeDate) ,
                  ('kBytes' , int),
                  
                  ('res' , int),
                  ('share' , int),
                  ('text' , int),
                  ('lib' , int),
                  ('data' , int),
                  ('dt' , int) ]
    
    _out_flds_ = [ ('send_date' , datetime.datetime.now ),
                   ('recd_date' , datetime.datetime.now),
                   ('sz' , int),
                   ('res' , int),
                   ('share' , int),
                   ('text' , int),
                   ('lib' , int),
                   ('data' , int),
                   ('dt' , int) ]
    
    _history_ =  { 'fields' : ['sz','res'] } 
    _stype_ = 'memory'
    
    def init(self):
        return
        
    def genout(self,indata):
        outdata = dict()
        #mrflog.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        outdata['send_date'] = indata['date'].to_datetime()
        outdata['recd_date'] = datetime.datetime.now()

        for f in self.out_data_flds():
            outdata[f] = indata[f]
                    
        return outdata


        
class DevLnxtst(MrfDev):

    _capspec = {
            'memory' : MrfSensMemory
    }
    
    _cmdset = LnxtstAppCmds

    _sens_names = [ 'sz',
                    'res',
                    'share',
                    'text',
                    'lib',
                    'data',
                    'dt' ]
    
    def __init__(self, rm, label, address, caplabels={}):
        mrflog.warn("%s __init__ entry , label %s address 0x%x"%(self.__class__.__name__,label,address))
        
        if caplabels == {}:
            slabs = []
            for sl in self._sens_names:
                slabs.append(label+"_"+sl)
            caplabels = {'memory' : slabs}
            mrflog.warn("no caplabels - setting to %s"%repr(caplabels))            
        return super(DevLnxtst,self).__init__(rm,label, address, caplabels)
    def app_packet(self, hdr, param , resp):
        mrflog.info("%s app_packet type %s"%(self.__class__.__name__, type(resp)))
        
        mrflog.info("LnxtstDev app_packet, hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))


