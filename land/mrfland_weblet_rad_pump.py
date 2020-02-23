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
from mrf_dev  import MrfDev
from mrf_sens_relay import MrfSensRelay
from mrfdev_pt1000  import MrfSensPt1000
from mrfland_weblet import MrflandWeblet, MrflandObjectTable, MrfWebletSensorVar, mrfctrl_butt_html
from mrflog import mrflog
import re
import datetime

class MrfLandWebletRadPump(MrflandWeblet):

    _config_ = [
                 ('max_return'  ,  45.0  , { 'min_val' : 30.0,  'max_val' :  60.0, 'step' : 1.0}),
                 ('min_store'   ,  50.0  , { 'min_val' : 30.0,  'max_val' :  70.0, 'step' : 1.0}),
                 ('hysterisis',    5.0   , { 'min_val' :  0.25,  'max_val' :  10.0, 'step' : 0.25})
    ]
    _tagperiods_  = [{'name':'EN','pulse' :True , 'num' : 3}]

    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))

        # begin sanity checks

        ## expect MrfSensPt1000 types

        if MrfSensPt1000 not in self.rm.senstypes:
            mrflog.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return

        ## expect MrfSensRelay types

        if MrfSensRelay not in self.rm.senstypes:
            mrflog.error("%s post_init failed to find sensor type MrfSensRelay in rm"%self.__class__.__name__)
            return


        ## expect config data fields as follows

        if 'pump' not in self.cdata:
            mrflog.error("%s , no pump in data"%self.__class__.__name__)
            return


        if 'flowsens' not in self.cdata:
            mrflog.error("%s , no flowsens in data"%self.__class__.__name__)
            return

        if 'storesens' not in self.cdata:
            mrflog.error("%s , no storesens in data"%self.__class__.__name__)
            return


        if 'retsens' not in self.cdata:
            mrflog.error("%s , no retsens in data"%self.__class__.__name__)
            return

        if 'rad' not in self.cdata:
            mrflog.error("%s , no rad in data"%self.__class__.__name__)
            return


        # end sanity checks

        # find period sensor
        sn = self.tag + "_EN" + "_PERIOD"

        #sn = self.cdata['rad'] + "_PERIOD"
        self.periodsens = self.rm.sens_search(sn)
        self.add_var('active',self.periodsens, field='active')

        # find store sensor

        self.storesens = self.rm.sens_search(self.cdata['storesens'])
        self.add_var('store_temp',self.storesens, field='temp', graph=True)

        # find temp sensors

        self.flowsens = self.rm.sens_search(self.cdata['flowsens'])
        self.add_var('flow_temp',self.flowsens, field='temp', graph=True)
        self.retsens = self.rm.sens_search(self.cdata['retsens'])
        self.add_var('return_temp',self.retsens, field='temp', graph=True)


        # find relay

        self.pump = self.rm.sens_search(self.cdata['pump'])
        self.add_var('pump',self.pump, field='relay', graph=True)



        ## declare state var

        if self.var.return_temp.val < self.var.max_return.val:
            self.add_var('state','UP', save=False)  # values UP DOWN , i.e. hysterisis ref select
        else:
            self.add_var('state','DOWN', save=False)  # values UP DOWN , i.e. hysterisis ref select

        ## normal vars to keep state of algo

        self.return_state = 'UP'
        self.store_state  = 'UP'


    def run_init(self):
        mrflog.warn("%s run_init"%(self.__class__.__name__))
        # start timer







    def var_changed(self,name,wsid):
        #mrflog.warn("%s var_changed %s "%(self.__class__.__name__, name))

        pump_curr = self.var.pump.val

        ## eval hysterisis states

        if self.return_state == 'UP'and self.var.return_temp.val > self.var.max_return.val:
                self.return_state = 'DOWN'
        elif self.return_state == 'DOWN' and  self.var.return_temp.val < (self.var.max_return.val - self.var.hysterisis.val):
            self.return_state = 'UP'


        if self.store_state == 'UP' and self.var.store_temp.val < self.var.min_store.val:
                self.store_state == 'DOWN'
        elif self.store_state == 'DOWN' and  self.var.store_temp.val >  (self.var.min_store.val + self.var.hysterisis.val):
            self.store_state == 'UP'


        if self.store_state == 'UP' and self.return_state == 'UP':  # for debug aid
            self.var.state.set("UP")
        else:
            self.var.state.set("DOWN")

        # determine pump setting

        if self.var.active.val and self.var.state.val == 'UP': # only turn pump on when period active and hyst_up
            pump_next = True
        else:
            pump_next = False

        # set relay if not already correct
        if pump_next != pump_curr:
            mrflog.warn("setting pump to %s"%repr(pump_next))
            self.pump.set(pump_next)
        #else:
        #    mrflog.warn("leaving pump %s"%repr(pump_next))




    def pane_html(self):
        s =  """
        <h2>"""+self.label+" </h2>"  #   "+self.var.tank_top.html+" &#176;C</h2>"

        s += self.rm.graph_inst({
            "temp" : [self.flowsens.label, self.retsens.label, self.storesens.label],
            "relay": [self.pump.label]
        })


        s += self.timer_ctrl_table()



        s += """
        <hr>
        <h3>Status</h3>"""

        s += self.html_var_table(
            [
                self.var.active.name,
                self.var.store_temp.name,
                self.var.flow_temp.name,
                self.var.return_temp.name,
                self.var.pump.name,
                self.var.state.name

            ]
        )

        s += """
        <hr>
        <h3>Config</h3>"""

        s += self.html_var_ctrl_table(
            [
                self.var. min_store.name,
                self.var.max_return.name,
                self.var.hysterisis.name
            ]
        )

        return s
