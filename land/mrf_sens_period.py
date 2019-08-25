
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
from mrf_structs import *

class MrfSensPeriod(MrfSens):
    _in_flds_ = [ ('date', PktTimeDate) ,
                  ('active' , bool) ]  # hmspff

    _out_flds_ = [ ('send_date' , datetime.datetime.now ),
                   ('recd_date' , datetime.datetime.now),
                   ('active' , bool )
    ]
    _history_ =  { 'fields' : ['active']
                 }
    _stype_ = 'relay'
    def init(self):
        self.clear()
    def clear(self):
        self.active = False

    def genout(self,indata):
        outdata = dict()
        #mrflog.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        outdata['send_date'] = indata['date'].to_datetime()
        outdata['recd_date'] = datetime.datetime.now()
        outdata['active']  = bool(indata['active'])
        return outdata
