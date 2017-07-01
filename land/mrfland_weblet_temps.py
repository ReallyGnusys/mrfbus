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
from mrfdev_pt1000 import *
from mrf_sens import MrfSens, MrfDev
from mrfland_weblet import MrflandWeblet, MrflandObjectTable


class MrfLandWebletTemps(MrflandWeblet):
    def post_init(self):
        self.log.info("%s post_init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPt1000):
            self.log.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return
        self.sl = self.rm.senstypes[MrfSensPt1000]

        self.log.info("num MrfSensPt1000 found was %d"%len(self.sl))
        self.slabs = []
        self.sens = OrderedDict()
        for s in self.sl:
            self.slabs.append(s.label)
            self.sens[s.label] = s
        self.log.info("MrfSensPt1000 : %s"%repr(self.slabs))

        for s in self.sens.keys():
            self.sens[s].subscribe(self.sens_callback)
    def sens_callback(self, label, data ):
        self.log.info("TempWeblet : sens_callback  %s  data %s"%(label,repr(data)))
        self.rm.webupdate(self.mktag('temp', label), data)
                          
        
    
    def pane_html(self):
        """ just want to display pt1000sens output stucture"""
        s =  """
        <h2>Temps</h2>"""
        if len(self.sl):
            self.log.warn("labels are %s "%repr(self.slabs))
            s += MrflandObjectTable("temps","temp",self.sl[0]._output,self.slabs)
        return s

