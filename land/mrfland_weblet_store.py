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
from mrf_sens import MrfSens
from mrf_dev  import MrfDev
from mrfland_weblet import MrflandWeblet, MrflandObjectTable
from mrflog import mrflog
import re

class MrfLandWebletStore(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPt1000):
            mrflog.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return
        ts = self.rm.senstypes[MrfSensPt1000]

        mrflog.info("num MrfSensPt1000 found was %d"%len(ts))
        self.slabs = []
        self.sens = OrderedDict()
        if not self.cdata.has_key('acc_tag'):
            mrflog.error("%s , no acc_tag in data")
            return
        if not self.cdata.has_key('acc_litres'):
            mrflog.error("%s , no acc_litres in data")

            return
        self.litres = self.cdata['acc_litres']


        self.top_ts = self.rm.sens_search_vector_max(MrfSensPt1000, self.cdata['acc_tag'])
        self.add_var('tank_top',self.top_ts, field='temp', graph=True)
        self.return_sens = self.rm.sens_search(self.cdata['acc_tag'] + "_RET")        
        self.add_var('return_temp',self.return_sens, field='temp', graph=True)


        self.ts = self.rm.sens_search_vector(MrfSensPt1000, self.cdata['acc_tag'])
        for l in self.ts:  # 
            s = self.ts[l]
            self.add_var(s.label,s, field='temp', graph=True)
  
        mrflog.warn("Store has temp sensors at following levels %s"%repr(self.ts.keys()))

        

    def pane_html(self):
        """ just want to display pt1000sens output stucture"""
        
        s =  """
        <h2>"""+self.label+"    "+self.var.tank_top.html+" &#176;C</h2>"
        sensors = []
        for level in self.ts:
            sensors.append(self.ts[level].label)

            
        sensors.append(self.return_sens.label)
         
        s += self.rm.graph_inst({
            "temp" : sensors
        })


        skeys = self.var.__dict__.keys()
        skeys.sort()

        s += """
        <hr>
        <h3>Temps</h3>"""
        s += self.html_var_table(skeys)
            

        

        return s
