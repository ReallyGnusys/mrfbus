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
import re

class MrfLandWebletPeriodTest(MrflandWeblet):
    re_period = re.compile(r'(.*)_PERIOD')
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        self.sl = self.rm.senscaps['period']

        self.graph_periods = []
        self.pumps = {}
        self.pumpnames = {}
        self.pumplist = []
        for s in self.sl:
            self.add_var(s.label, s , field='active', graph=True)
            self.graph_periods.append(s.label)
            mob = self.re_period.match(s.label)
            if mob:
                pn = mob.group(1) + '_PUMP'
                if self.rm.sensors.has_key(pn):
                    pumpsens = self.rm.sensors[pn]
                    self.pumpnames[s.label] = pn
                    self.pumps[s.label] = pumpsens
                    self.add_var(pn, pumpsens , field='relay')
                    self.pumplist.append(pumpsens) # for table


    def var_changed(self,name,wsid):


        # check all pumps are set according to period

        for name in self.graph_periods:  # this is just list of period labels:

            pump = self.pumps[name]
            pn   = self.pumpnames[name]
            pump_state = self.var.__dict__[pn].val
            period_state = self.var.__dict__[name].val

            if pump_state != period_state:
                mrflog.warn("pump %s state = %d period %s state = %d - setting pump to %d"%(pn,pump_state,name,period_state,period_state))
                pump.set(period_state)


    def pane_html(self):
        s =  """
        <h2>%s</h2>"""%self.label
        s += """
        <hr> """

        s += self.rm.graph_inst({
            "active" : self.graph_periods
        })

        s += self.html_var_table(self.sl + self.pumplist)

        s += self.timer_ctrl_table()

        return s
