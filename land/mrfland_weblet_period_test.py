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

    _tagperiods_ =  [ {'name' : 'RAD_PUMP', 'pulse' : True , 'num' : 3},
                      {'name' : 'HX_PUMP' , 'pulse' : True , 'num' : 3},
                      {'name' : 'HEAT'    , 'pulse' : True , 'num' : 3},
                      {'name' : 'LIGHT'   , 'pulse' : True , 'num' : 3}]

    re_period = re.compile(r'(.*)_PERIOD')
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))

        self.graph_periods = []
        self.pumps = {}
        self.pumpnames = {}
        self.pumplist = []
        self.graph_pumps = []
        for tper in  self.tagperiodvar:
            mrflog.warn("got tper :"+tper)

            mrflog.warn(" = "+repr(self.tagperiodvar[tper]))
            pname = self.cdata['targ'] + '_' + tper
            mrflog.warn("  pname "+ pname)
            pumpsens = self.rm.sensors[pname]
            self.pumpnames[tper] = pname
            self.pumps[tper] = pumpsens

            self.add_var(pname, pumpsens , field='relay', graph=True)
            self.graph_pumps.append(pname)
            self.graph_periods.append(self.tagperiodvar[tper])


    def var_changed(self,name,wsid):


        # check all pumps are set according to period
        mrflog.warn("var_changed "+name)
        for name in  self.tagperiodvar:

            pvar = self.tagperiodvar[name]
            pump = self.pumps[name]
            pn   = self.pumpnames[name]
            pump_state = self.var.__dict__[pn].val
            period_state = self.var.__dict__[pvar].val

            if pump_state != period_state:
                mrflog.warn("pump %s state = %d period %s state = %d - setting pump to %d"%(pn,pump_state,name,period_state,period_state))
                pump.set(period_state)


    def pane_html(self):
        s =  """
        <h2>%s</h2>"""%self.label
        s += """
        <hr> """

        s += self.rm.graph_inst({
            "relay" : self.graph_pumps,
            "active" : self.graph_periods
        })

        s += self.html_var_table(self.graph_pumps)
        s += self.html_var_table(self.graph_periods)

        s += self.timer_ctrl_table()

        return s
