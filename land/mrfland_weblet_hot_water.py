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
from mrfdev_pt1000 import MrfSensPt1000
from mrf_sens_relay import MrfSensRelay

from mrfland_weblet import MrflandWeblet, MrflandObjectTable, MrfWebletSensorVar
from mrflog import mrflog
import re
from datetime import datetime, timedelta

class MrfLandWebletHotWater(MrflandWeblet):

    _config_ = [ ('enabled'      ,  False , {}),
                 ('target_temp'  ,  60.0  , { 'min_val' : 40.0,  'max_val' :  65.0, 'step' : 0.5}),
                 ('delta_targ_rx',  8.0   , { 'min_val' :  6.0,  'max_val' :  10.0, 'step' : 0.5}),
                 ('min_wait_hours', 16.0   , { 'min_val' :  1.0,  'max_val' : 24.0, 'step' : 0.5}),
                 ('min_head',       10.0   , { 'min_val' :  9.0,  'max_val' : 12.0, 'step' : 0.5}),
                 ('hysteresis'   ,  4.0   , { 'min_val' :  2.0,  'max_val' :  12.0, 'step' : 1.0})
    ]

    def __init__(self, rm, cdata, vdata={}):
        if not 'rad' in cdata:
            mrflog.error('no rad in cdata')
            self.radtimer = None
        else:
            pumpname = cdata['rad'] + '_PUMP'
            timer_name = cdata['rad'] + '_dhwcirculate' # create new timer for rad to allow us to pump cold water out of pipes
            if not 'timers' in cdata:
                cdata['timers'] = []
            cdata['timers'].append(timer_name)
            mrflog.warn("added timer "+timer_name)
            self.radtimer = timer_name

        super(MrfLandWebletHotWater, self).__init__(rm,cdata,vdata)
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))

        # begin sanity checks

        ## expect MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPt1000):
            mrflog.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return

        ## expect MrfSensRelay types

        if not self.rm.senstypes.has_key(MrfSensRelay):
            mrflog.error("%s post_init failed to find sensor type MrfSensRelay in rm"%self.__class__.__name__)
            return


        ## expect config data fields as follows

        if not self.cdata.has_key('rad'):
            mrflog.error("%s , no rad in data"%self.__class__.__name__)
            return


        if not self.cdata.has_key('acctop'):
            mrflog.error("%s , no acctop in data"%self.__class__.__name__)
            return

        if not self.cdata.has_key('heatbox'):
            mrflog.error("%s , no heatbox in data"%self.__class__.__name__)
            return
        if not self.cdata.has_key('tag'):
            mrflog.error("%s , tag"%self.__class__.__name__)
            return

        if not self.cdata.has_key('litres'):
            mrflog.error("%s , litres"%self.__class__.__name__)
            return

        # end sanity checks

        self.litres      = self.cdata['litres']


        # find temp sensors

        self.top_ts = self.rm.sens_search_vector_max(MrfSensPt1000,self.tag)
        self.add_var('tank_top',self.top_ts, field='temp', graph=True)


        self.flow_sens = self.rm.sens_search(self.cdata['heatbox'] + "_FLOW")
        self.add_var('hx_flow', self.flow_sens, field='temp' , graph=True)


        self.return_sens = self.rm.sens_search(self.tag + "_HX_RET")
        self.add_var('hx_ret', self.return_sens, field='temp', graph=True)

        self.acc_sens = self.rm.sens_search(self.cdata['acctop'])
        self.add_var('acc_top', self.acc_sens, field='temp', graph=True)

        # find relays

        self.hx_relay = self.rm.sens_search(self.cdata['tag'] + "_HX_PUMP")
        self.add_var('hx_relay',self.hx_relay, field='relay', graph=True)

        self.rad_relay = self.rm.sens_search(self.cdata['rad'] + "_PUMP")
        self.add_var('rad_relay',self.rad_relay, field='relay', graph=True)


        self.heat_relay = self.rm.sens_search(self.cdata['tag'] + "_HEAT")
        self.add_var('heat_relay',self.heat_relay, field='relay', graph=True)



        ## declare state var

        self.add_var('state','REST')


        # set timer vars for convenience
        # place holder for timer vars set in init
        self.rtimer_onv = None
        self.rtimer_offv = None
        self.rtimer_actv = None

        if not self.radtimer:
            mrflog.error("radtimer not set!")
            return
        tmr = self._timers[self.radtimer]
        self.rtimer_onv  = tmr.__dict__['on']
        self.rtimer_offv = tmr.__dict__['off']
        self.rtimer_env  = tmr.__dict__['en']
        self.rtimer_actv = tmr.__dict__['active']

    def run_init(self):
        mrflog.warn("%s run_init"%(self.__class__.__name__))
        # start timer
        now = datetime.now()
        td  = timedelta(seconds = 30)
        tod = now + td
        self.set_timer( tod.time() , 'state' , 'TO')


    def set_timeout(self,seconds):
        # start timer
        mrflog.warn("%s : set_timeout seconds  %d  "%(self.__class__.__name__,seconds))
        now = datetime.now()
        td  = timedelta(seconds = seconds)
        tod = now + td
        self.set_timer( tod.time() , 'state' , 'TO')

    def set_rad_timer(self,seconds): # pulse on rad timer if we need to pump cold water out of pipe before charge
        if not self.radtimer:
            mrflog.error("radtimer not set!")
            return
        start = datetime.datetime.now()
        end = start + datetime.timedelta(seconds = seconds)
        self.rtimer_onv.set(start.strftime("%H:%M"))
        self.rtimer_offv.set(end.strftime("%H:%M"))
        self.rtimer_env.set(True)

    def clear_rad_timer(self): # pulse on rad timer if we need to pump cold water out of pipe before charge
        if not self.radtimer:
            mrflog.error("radtimer not set!")
            return

        self.rtimer_onv.set("00:00")
        self.rtimer_offv.set("00:00")
        self.rtimer_env.set(False)


    def timer_callback(self, label, act):
        mrflog.warn("%s : timer_callback label %s act %s  "%(self.__class__.__name__,label,act))
        self.state_update(timeout=True)


    def var_changed(self,name,wsid=None):
        #mrflog.warn("%s var_changed %s "%(self.__class__.__name__, name))
        """
        if name == 'enabled':
            if self.var.__dict__[name].val:
                self.var.state.set('REST')
                now = datetime.now()
                td  = timedelta(seconds = 10)
                tod = now + td
                self.set_timer( tod.time() , 'state' , 'TO')
            else:
                next_state = 'DISABLED'
                self.hx_relay.set(0)
        """
        if name != 'state':  # otherwise infinite recursion!
            self.state_update()


    # check/set hx_relay state for every input change
    def state_update(self, timeout = False):
        next_state = self.var.state.val
        pump_curr = self.var.hx_relay.val

        pump_next  = 0

        if self.var.enabled.val == False:
            next_state = 'DISABLED'
        elif self.var.state.val == 'REST':
            if timeout:
                next_state = 'IDLE'
        elif self.var.state.val == 'IDLE':
            if self.var.tank_top.val < (self.var.target_temp.val - self.var.hysteresis.val):
                if self.var.acc_top.val > (self.var.tank_top.val + self.var.min_head.val):
                    if self.var.hx_flow.val > (self.var.tank_top.val - 3.0):
                        mrflog.warn("%s state_update to CHARGE1 flow_temp %.2f top temp %.2f"%(self.__class__.__name__,self.var.hx_flow.val, self.var.tank_top.val))
                        next_state = 'CHARGE1'
                        pump_next = 1
                        self.set_timeout(60*10)

                    else:
                        next_state = 'PREPUMP'
                        self.set_rad_timer(60*4)
                        self.set_timeout(60*4)

        elif self.var.state.val == 'PREPUMP':
            if timeout or self.var.hx_flow.val > (self.var.tank_top.val - 3.0):
                self.clear_rad_timer()
                pump_next = 1
                next_state = 'CHARGE1'
                self.set_timeout(60*10)

        elif self.var.state.val == 'CHARGE1':
            pump_next = 1
            if timeout:
                mrflog.warn("%s %s timeout in state %s"%(self.__class__.__name__,self.label,self.var.state.val))
                next_state = 'CHARGING'
                self.set_timeout(90*60)   #90 mins max for now
            elif self.var.tank_top.val > self.var.target_temp.val :
                mrflog.warn("%s %s reached target temperature in state %s"%(self.__class__.__name__,self.label,self.var.state.val))
                next_state = 'REST'
                self.set_timeout(60*60*self.var.min_wait_hours.val)
                pump_next = 0

        elif self.var.state.val == 'CHARGING':

            trans = False
            pump_next = 1

            if timeout:
                mrflog.warn("%s %s timeout in state %s"%(self.__class__.__name__,self.label,self.var.state.val))
                trans = True

            elif self.var.tank_top.val > self.var.target_temp.val:
                mrflog.warn("%s %s reached target temperature in state %s"%(self.__class__.__name__,self.label,self.var.state.val))
                trans = True
            elif self.var.tank_top.val > (self.var.hx_flow.val - 5.0):
                mrflog.warn("%s %s tank top (%.2f) reached min diff re. flow temp (%.2f)  in state %s"%
                            (self.__class__.__name__,self.label,self.var.tank_top.val,self.var.hx_flow.val,self.var.state.val))
                trans = True
            elif self.var.hx_ret.val > (self.var.target_temp.val - self.var.delta_targ_rx.val):  # don't want return to get too high
                mrflog.warn("%s %s return temp (%.2f) reached limit in state %s"%
                            (self.__class__.__name__,self.label, self.var.hx_ret.val,self.var.state.val))
                trans = True

            if trans:
                pump_next = 0
                next_state = 'REST'
                self.set_timeout(60*60*self.var.min_wait_hours.val)   # wait 2 hours before checking again

        if pump_next != pump_curr:
            mrflog.warn("pump_next %d != pump_curr - setting hx_relay to %d"%(pump_next,pump_curr,pump_next))
            self.hx_relay.set(pump_next)

        if next_state != self.var.state.val:
            self.var.state.set(next_state)
            mrflog.warn("%s %s  state change to %s"%(self.__class__.__name__,self.label,self.var.state.val))
            #tg = self.mktag('hwstat', 'state')
            #dt =  { 'val' : self.var.state.val}
            #self.rm.webupdate(tg, dt)

    def pane_html(self):
        s =  """
        <h2>"""+self.label+"    "+self.var.tank_top.html+" &#176;C</h2>"

        s += self.rm.graph_inst({
            "temp" : [self.top_ts.label, self.flow_sens.label, self.return_sens.label, self.acc_sens.label],
            "relay": [self.hx_relay.label, self.rad_relay.label, self.heat_relay.label]
        })

        s += """
        <hr>
        <h3>Status</h3>"""

        s += self.html_var_table(
            [
                self.var.tank_top.name,
                self.var.acc_top.name,
                self.var.hx_flow.name,
                self.var.hx_ret.name,
                self.var.state.name,
                self.var.enabled.name,
                self.var.hx_relay.name,
                self.var.rad_relay.name,
                self.var.heat_relay.name
            ]
        )

        s += """
        <hr>
        <h3>Config</h3>"""

        s += self.html_var_ctrl_table(
            [
                self.var.enabled.name,
                self.var.target_temp.name,
                self.var.min_head.name,
                self.var.min_wait_hours.name
            ]
        )

        return s
