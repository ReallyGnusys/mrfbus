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
from mrfdev_lnxtst import *

from mrf_sens import MrfSens
from mrf_dev  import MrfDev
from mrfland_weblet import MrflandWeblet, MrflandObjectTable
from mrflog import mrflog
import re
import pdb

class MrfLandWebletMem(MrflandWeblet):
    def init(self):
        mrflog.warn("%s init"%(self.__class__.__name__))
        if not self.rm.senscaps.has_key('memory'):
            mrflog.error("%s post_init failed to find sensor category memory in rm"%self.__class__.__name__)
            return

        self.sl = self.rm.senscaps['memory']
        self.graph_mems = []

        mrflog.warn("len sl %d"%len(self.sl))
        for s in self.sl:
            mrflog.warn("got label "+s.label)
            self.add_var(s.label, s , field='sz', graph=True)
            self.graph_mems.append(s.label)
        #else:
        #    self.add_var(s.label, s , field='sz')

    def pane_html(self):
        s = ""

        s += """
        <hr> """

        s += self.rm.graph_inst({
            "memory" : self.graph_mems
        })

        s += self.html_var_table(self.sl)

        return s
