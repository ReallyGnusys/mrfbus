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
from mrfland_weblet import MrflandWeblet
from mrflog import mrflog


class MrfLandWebletPeriodTest(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        self.sl = self.rm.senscaps['period']
        self.graph_periods = []
        for s in self.sl:
            self.add_var(s.label, s , field='active', graph=True)
            self.graph_periods.append(s.label)



    def pane_html(self):
        s =  """
        <h2>%s</h2>"""%self.label
        s += """
        <hr> """

        s += self.rm.graph_inst({
            "active" : self.graph_periods
        })

        s += self.html_var_table(self.sl)

        s += self.timer_ctrl_table()

        return s
