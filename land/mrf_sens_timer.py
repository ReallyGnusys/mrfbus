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
from mrf_sens import MrfSens, MrfDev
import datetime
import ctypes
from mrf_structs import *



class MrfSensTimer(MrfSens):
    _in_flds_ = [ ('cname', unicode), ('val' , dict) ]
    
    _out_flds_ = [ ('on' , datetime.time ),
                   ('off' , datetime.time ),
                   ('active', bool) ]

    def is_active(self , outdata = None):
        if outdata == None:
            outdata = self.outdata
        nw = datetime.datetime.now()
        nt = datetime.time (nw.hour,nw.minute,nw.second)
        if outdata['on'] <= outdata['off']:            
            return outdata['on'] < nt < outdata['off']
        else:
            return (outdata['on'] < nt) or ( nt < outdata['off'])
    
    def genout(self,indata, outdata):
        #self.log.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        if not indata.has_key('cname'):
            self.log.error("%s genout no key cname in %s"%(self.__class__.__name__, repr(indata)))
            return

        
        cname = indata['cname']
        outdata[cname] = datetime.time(hour=indata['val']['hour'],minute=indata['val']['minute'], second=indata['val']['second'])

        nw = datetime.datetime.now()
        nt = datetime.time (nw.hour,nw.minute,nw.second)
        outdata['active'] = self.is_active(outdata)

        self.log.warn("%s %s genout %s"%(self.__class__.__name__, self.label, repr(outdata)))
        self.log.warn("%s %s subsribers = %s"%(self.__class__.__name__, self.label, repr(self.subscribers)))
        return outdata
        

