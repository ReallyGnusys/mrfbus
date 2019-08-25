#!/usr/bin/env python
'''  Copyright (c) 2012-17 Owners of gnusys.com

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
from mrflog import mrflog


class MrfSensTimer(MrfSens):
    _in_flds_ = [ ('cname', unicode), ('val' , dict) ]

    _out_flds_ = [ ('on' , datetime.time ),
                   ('off' , datetime.time ),
                   ('active', bool) ]
    _stype_ = 'timer'

    def is_active(self , outdata = None):
        if outdata == None:
            outdata = self.output
        nw = datetime.datetime.now()
        nt = nw.time()
        if outdata['on'] == outdata['off']:
            return False
        if outdata['on'] < outdata['off']:
            return (outdata['on'] < nt) and (nt < outdata['off'])
        else:
            return (outdata['on'] < nt) or ( nt < outdata['off'])

    def expire_callback(self, on_or_off):
        mrflog.warn("%s %s expire_callback on_or_off was %s"%(self.__class__.__name__, self.label))

    def genout(self,indata):
        outdata = dict()
        #mrflog.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        if not indata.has_key('cname'):
            mrflog.error("%s genout no key cname in %s"%(self.__class__.__name__, repr(indata)))
            return

        cname = indata['cname']
        outdata[cname] = datetime.time(hour=indata['val']['hour'], minute=indata['val']['minute'], second=indata['val']['second'])

        #nw = datetime.datetime.now()
        #nt = datetime.time (nw.hour, nw.minute, nw.second)
        outdata['active'] = self.is_active(outdata)

        mrflog.warn("%s %s genout %s"%(self.__class__.__name__, self.label, repr(outdata)))
        mrflog.warn("%s %s subscribers = %s"%(self.__class__.__name__, self.label, repr(self.subscribers)))
        return outdata
