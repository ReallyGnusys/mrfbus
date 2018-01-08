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

class MrfLandWebletTemps(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))

        if not self.rm.senscaps.has_key('temp'):
            mrflog.error("%s post_init failed to find sensor category temp in rm"%self.__class__.__name__)
            return
        self.sl = self.rm.senscaps['temp']
        
        self.graph_temps = []

        for s in self.sl:
            if s.label.find("_AMBIENT") != -1:
                self.add_var(s.label, s , field='temp', graph=True)
                self.graph_temps.append(s.label)
            else:
                self.add_var(s.label, s , field='temp')


                
    def pane_html(self):
        if self.var.__dict__.has_key('LOUNGE_AMBIENT') and self.var.__dict__.has_key('OUTSIDE_AMBIENT'):
            s =  """
        <h2>Lounge """+self.var.LOUNGE_AMBIENT.html+" &#176;C   Outside """+self.var.OUTSIDE_AMBIENT.html+" &#176;C</h2>"

        else:
            s = ""
            
        s += """
        <hr> """

        s += self.rm.graph_inst({
            "temp" : self.graph_temps
        })
            
        s += self.html_var_table(self.sl)
           
        return s
