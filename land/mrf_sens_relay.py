
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

import ctypes
from mrf_structs import *
from mrf_sens import MrfSens
import datetime
from mrflog import mrflog


class MrfSensRelay(MrfSens):
    _in_flds_ = [ ('date', PktTimeDate) ,
                  ('relay' , int) ]

    _out_flds_ = [ ('send_date' , datetime.datetime.now ),
                   ('recd_date' , datetime.datetime.now),
                   ('relay' , int )
    ]
    _history_ =  { 'fields' : ['relay']
                 }
    _stype_ = 'relay'
    def init(self):
        self.req_val     = 0
        #self.clear()  # this doesn't work at sens init, as network is not up... but called again by weblet relays run_init
    def clear(self):
        self.set(0)  # turn off all relays on server start

    def genout(self,indata):
        outdata = dict()
        #mrflog.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        outdata['send_date'] = indata['date'].to_datetime()
        outdata['recd_date'] = datetime.datetime.now()
        outdata['relay']  = int(indata['relay'])
        return outdata

    def _cmd(self,on_off):
        self.req_val = on_off
        cdata = {}

        cdata['chan'] = self.channel
        cdata['val'] = int(on_off)
        param = PktRelayState()
        param.dic_set(cdata)
        mrflog.debug("%s sending SET_RELAY with param %s"%
                    (self.__class__.__name__ , repr(param)))
        self.devupdate('SET_RELAY',param)

    def set(self, on_off):
        self.req_val = on_off
        if (self.req_val != self.output['relay']):
            mrflog.warn("%s %s changing relay state to %d  output %d"%
                        (self.__class__.__name__,self.label,self.req_val,self.output['relay']))
            self._cmd(on_off)
        else:
            mrflog.warn("%s %s NOT changing relay state to %d  output %d"%
                        (self.__class__.__name__,self.label,self.req_val,self.output['relay']))
