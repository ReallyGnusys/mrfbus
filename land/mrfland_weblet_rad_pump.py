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
from mrfland_weblet import MrflandWeblet, MrflandObjectTable, MrfWebletSensorVar, mrfctrl_butt_html
from mrflog import mrflog
import re
import datetime

class MrfLandWebletRadPump(MrflandWeblet):

    _config_ = [
                 ('max_return'  ,  45.0  , { 'min_val' : 30.0,  'max_val' :  60.0, 'step' : 1.0}),
                 ('hysterisis',    5.0   , { 'min_val' :  0.1,  'max_val' :  10.0, 'step' : 0.25}),
                 ('pulse_time',    5   ,   { 'min_val' :  1,  'max_val' :  10, 'step' : 1})
    ]
    
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))

        # begin sanity checks

        ## expect MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPt1000):
            mrflog.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return    

        ## expect MrfSensPtRelay types

        if not self.rm.senstypes.has_key(MrfSensPtRelay):
            mrflog.error("%s post_init failed to find sensor type MrfSensPtRelay in rm"%self.__class__.__name__)
            return


        ## expect config data fields as follows
        
        if not self.cdata.has_key('pump'):
            mrflog.error("%s , no pump in data"%self.__class__.__name__)
            return
        

        if not self.cdata.has_key('flowsens'):
            mrflog.error("%s , no flowsens in data"%self.__class__.__name__)
            return
        
        if not self.cdata.has_key('retsens'):
            mrflog.error("%s , no retsens in data"%self.__class__.__name__)
            return

        # end sanity checks
        
        

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
            self.add_var('state','UP')  # values UP DOWN , i.e. hysterisis ref select
        else:
            self.add_var('state','DOWN')  # values UP DOWN , i.e. hysterisis ref select

    def run_init(self):
        mrflog.warn("%s run_init"%(self.__class__.__name__))
        # start timer
        

    def var_changed(self,name,wsid=None):
        mrflog.warn("%s var_changed %s "%(self.__class__.__name__, name))

        
        
    def cmd_mrfctrl(self,data,wsid):
        mrflog.warn( "cmd_mrfctrl here, data was %s"%repr(data))

        if data['tab'] != 'timer_pulse':
            return

        tmr = self._timers[self.cdata['rad']+'_P0']
        onv = tmr.__dict__['on']
        offv = tmr.__dict__['off']
        env = tmr.__dict__['en']
        actv = tmr.__dict__['active']
        
        if data['row'] == 'add':
            mrflog.warn( "try to set timer to 5 mins")
            start = datetime.datetime.now()

            if actv.val:  # if timer active we add 5 mins otherwise set 5 mins from now
                end = datetime.datetime.combine(start.date(),offv.tod) +  datetime.timedelta(minutes = self.var.pulse_time.val)
            else:
                end = start + datetime.timedelta(minutes = self.var.pulse_time.val)

            onv.set(start.strftime("%H:%M"))                
            offv.set(end.strftime("%H:%M"))
            env.set(True)
        elif data['row'] == 'clear':
            mrflog.warn( "try to clear timer")
            onv.set("00:00")
            offv.set("00:00")
            env.set(False)
            
    def var_changed(self,name,wsid):
        # manage P0 timer expiry , simply clear associated variables
        if name == self.cdata['rad']+'_P0' + '_active':
            mrflog.warn(self.cdata['rad']+'_P0' + '_active changed to %s'%repr(self.var.__dict__[name]))
            if self.var.__dict__[name].val == False:
                mrflog.warn(self.cdata['rad']+'_P0' + '_active is False')
                tmr = self._timers[self.cdata['rad']+'_P0']
                onv = tmr.__dict__['on']
                offv = tmr.__dict__['off']
                env = tmr.__dict__['en']
                onv.set("00:00")
                offv.set("00:00")
                env.set(False)
        elif name == 'return_temp':
            # modulate pump to limit return temperature
            #mrflog.warn("rad return temp changed")
            if self.var.state.val == 'UP':
                if self.var.return_temp.val > self.var.max_return.val:
                    mrflog.warn("%s rad upper return temp limit reached forcing off")
                    self.var.state.set("DOWN")
                    self.pump.force(0)
            elif self.var.state.val == 'DOWN':
                if self.var.return_temp.val < (self.var.max_return.val - self.var.hysterisis.val):
                    mrflog.warn("%s rad lower return temp limit reached releasing pump control")
                    self.var.state.set("UP")
                    self.pump.release()
                    
 
     
    def pane_html(self):
        s =  """
        <h2>"""+self.label+" </h2>"  #   "+self.var.tank_top.html+" &#176;C</h2>"
        
        s += self.rm.graph_inst({
            "temp" : [self.flowsens.label, self.retsens.label],
            "relay": [self.pump.label]
        })

        s += mrfctrl_butt_html(self.tag,'timer_pulse','add','5mins', cls='glyphicon-time')
        s += mrfctrl_butt_html(self.tag,'timer_pulse','clear','clear', cls='glyphicon-remove')
        

        s += self.timer_ctrl_table(include_list=[self.cdata['rad']+'_P0'],ro=True)

        s += self.timer_ctrl_table(exclude_list=[self.cdata['rad']+'_P0'])

        
        s += """
        <hr>
        <h3>Status</h3>"""

        s += self.html_var_table(
            [
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
                self.var.max_return.name,
                self.var.hysterisis.name,
                self.var.pulse_time.name,
                
            ]
        )
        
        return s