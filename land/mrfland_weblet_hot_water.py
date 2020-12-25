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

    _config_ = [
        ('target_temp'  ,  60.0  ,  { 'min_val' :  40.0,  'max_val' :  65.0, 'step' : 0.5}),
        ('min_head',       10.0   , { 'min_val' :   5.0,  'max_val' :  15.0, 'step'  : 0.5}),
        ('min_wait_hours', 16.0   , { 'min_val' :   0.5,  'max_val' :  24.0, 'step'  : 0.5}),
        ('hx_timeout_mins', 90.0   , { 'min_val':  20.0,  'max_val' :  120.0, 'step' : 5.0}),
        ('delta_flow_rx',  10.0   , { 'min_val' :   6.0,  'max_val' :  25.0, 'step' : 0.5}), #
        ('delta_flow_tank',  -2.0 , { 'min_val' :  -5.0,  'max_val' :  5.0, 'step' : 0.5}),
        ('immersion_temp' ,  62.0  , { 'min_val':  40.0,  'max_val' :  65.0, 'step': 0.5}),
        ('hysteresis'   ,  4.0   , { 'min_val'  :   0.25,  'max_val' :  4.0, 'step'  : 0.25})

    ]

    _tagperiods_  = [{'name':'HX','pulse' :True , 'num' : 3}, {'name' : 'IM','pulse':True , 'num' : 3}]


    def __init__(self, rm, cdata, vdata={}):
        if not 'timers' in cdata:  # slightly crass way to set timer for associated RAD
            cdata['timers'] = []
        if not 'rad' in cdata:
            mrflog.error('no rad in cdata')
            self.radtimer = None
        else:
            timer_name = cdata['rad'] + '_EN_dhwcirculate' # create new timer for rad to allow us to pump cold water out of pipes
            cdata['timers'].append(timer_name)
            mrflog.warn("added timer "+timer_name)
            self.radtimer = timer_name


        super(MrfLandWebletHotWater, self).__init__(rm,cdata,vdata)
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))

        # make ref for IM tagperiod variable

        self.im_period = self.var.__dict__[self.tagperiodvar['IM']]
        self.hx_period = self.var.__dict__[self.tagperiodvar['HX']]

        # can override label used for display in tables
        self.im_period.label = 'im_active'
        self.hx_period.label = 'hx_active'

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

        if 'rad' not in self.cdata:
            mrflog.error("%s , no rad in data"%self.__class__.__name__)
            return


        if 'acctop' not in self.cdata:
            mrflog.error("%s , no acctop in data"%self.__class__.__name__)
            return

        if 'heatbox' not in self.cdata:
            mrflog.error("%s , no heatbox in data"%self.__class__.__name__)
            return
        if 'tag' not in self.cdata:
            mrflog.error("%s , tag"%self.__class__.__name__)
            return

        if 'litres' not in self.cdata:
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

        ## normal var for heater hysterisis state
        self.heater_state = 'UP'


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
        start = datetime.now()
        end = start + timedelta(seconds = seconds)
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



    def run_init(self):
        mrflog.warn("%s run_init"%(self.__class__.__name__))
        # start timer

        if self.hx_period.val == False:
            self.var.state.set('DISABLED')

        else:
            self.var.state.set('IDLE')




    def var_changed(self,name,wsid):
        if name == 'state':  # otherwise infinite recursion!
            return
        #mrflog.warn("%s var_changed %s "%(self.__class__.__name__, name))
        self.state_update()

    def hx_pump_next(self,pump_curr,debug=False):
        stopped = int(pump_curr == 0)
        running = int(pump_curr == 1)
        # checks temp against target ( with hysterisis)
        if not (self.var.tank_top.val < (self.var.target_temp.val - stopped * self.var.hysteresis.val)):
            if pump_curr or debug:  # print message if threshold reached while pumping
                mrflog.warn("target temperature reached - now  %.2f"%self.var.tank_top.val)
            return 0 , 0

        # checks sufficient accumulator head ( with hysterisis)

        if not (self.var.acc_top.val > (self.var.tank_top.val + self.var.min_head.val - running * self.var.hysteresis.val)):
            if pump_curr or debug: # print message if threshold reached while pumping
                mrflog.warn("min_head threshold reached acc %.2f , tank  %.2f"%(self.var.acc_top.val,self.var.tank_top.val))

            return 0 , 0


        ## switchover from prepump to charge when rx_flow reaches threshold
        swth =  (self.var.tank_top.val + self.var.delta_flow_tank.val - running * self.var.hysteresis.val)
        sw = self.var.hx_flow.val > swth


        if sw :
            # hx_ret is not over threshold ( with hysterisis) - but only if hx_flow up to speed
            if pump_curr != 1:
                mrflog.warn("over switchover threshold hx_pump changing to on: hx_flow   %.2f , tank  %.2f"%(self.var.hx_flow.val,self.var.tank_top.val))

            return 1 , 0
        else:
            return 0, 1
        """
            if self.var.hx_ret.val > rxth:
                if pump_curr or debug: # print message if threshold reached while pumping
                    mrflog.warn("switch to hx threshold reached  %.2f (th was %.2f)"%(self.var.hx_ret.val,rxth))

                return 1, 0
        else:
            return 0 , 1

        return 1 , 0
        """
    def state_update(self, timeout=False):

        ## evalute Immersion heater rules for setting heater relay
        heat_curr = self.var.heat_relay.val

        ## eval hysterisis state of heater

        if self.heater_state == 'UP' and self.var.tank_top.val > self.var.immersion_temp.val:
            self.heater_state = 'DOWN'
        elif self.heater_state == 'DOWN' and self.var.tank_top.val < ( self.var.immersion_temp.val - 1.5):
            self.heater_state = 'UP'

        ## set heater relay on whenever period is active and heater_state == UP
        if self.im_period.val and self.heater_state == 'UP':
            heat_next = 1
        else:
            heat_next = 0

        if heat_next != heat_curr:
            mrflog.warn("heat_next %d != heat_curr %d - setting heat_relay to %d"%(heat_next,heat_curr,heat_next))
            self.heat_relay.set(heat_next)


        # Control HX charging
        next_state = self.var.state.val
        pump_curr = self.var.hx_relay.val
        pump_next  = 0

        if self.hx_period.val == False:
            next_state = 'DISABLED'
        elif self.var.state.val == 'DISABLED' and self.hx_period.val:
            next_state = 'IDLE'
        elif self.var.state.val == 'REST':
            if timeout:
                mrflog.warn("%s timeout in state REST , changing to IDLE"%(self.__class__.__name__))

                next_state = 'IDLE'
        elif self.var.state.val == 'IDLE':
            pnext, radnext  = self.hx_pump_next(pump_curr)
            #if pnext or radnext:
            #    self.hx_pump_next(pump_curr,debug=True)  # just for printout
            if pnext:
                mrflog.warn("%s state_update to CHARGING flow_temp %.2f top temp %.2f"%(self.__class__.__name__,self.var.hx_flow.val, self.var.tank_top.val))
                next_state = 'CHARGE1'
                pump_next = 1
                self.set_timeout(2*60)

            elif radnext:
                next_state = 'PREPUMP'
                mrflog.warn("%s state_update to PREPUMP flow_temp %.2f top temp %.2f"%(self.__class__.__name__,self.var.hx_flow.val, self.var.tank_top.val))
                self.set_rad_timer(60*4)
                self.set_timeout(60*4)

        elif self.var.state.val == 'PREPUMP':
            if timeout or self.var.hx_flow.val > (self.var.tank_top.val + self.var.delta_flow_tank.val):
                mrflog.warn("%s state_update to CHARGE1 (from PREPUMP) flow_temp %.2f top temp %.2f"%(self.__class__.__name__,self.var.hx_flow.val, self.var.tank_top.val))
                self.clear_rad_timer()
                pump_next = 1
                next_state = 'CHARGE1'
                self.set_timeout(5*60)

        #5 min fixed time (set by call code above )  with hx pump on to let hx_flow and hx_ret stabilise
        elif self.var.state.val == 'CHARGE1':
            pump_next = 1
            if timeout:  # go to main CHARGING mode , with config timeout
                self.set_timeout(self.var.hx_timeout_mins.val*60)
                next_state = 'CHARGING'


        elif self.var.state.val == 'CHARGING':

            trans = False
            pump_next = 1

            pnext,radnext = self.hx_pump_next(pump_curr)
            if not pnext:

                mrflog.warn("%s %s hx pump changing to False in state  CHARGING: hx_flow   %.2f , hx_return  %.2f"%(self.__class__.__name__,self.label,self.var.hx_flow.val,self.var.hx_ret.val))


            if pnext: # check return is not too hot cf. flow
                rxth = (self.var.hx_flow.val - self.var.delta_flow_rx.val - int(pump_curr==0) * self.var.hysteresis.val)
                pnext = self.var.hx_ret.val < rxth

                if not pnext:
                    mrflog.warn("%s %s hx return over threshold during state CHARGING: hx_flow   %.2f , hx_return  %.2f"%(self.__class__.__name__,self.label,self.var.hx_flow.val,self.var.hx_ret.val))


            if timeout:
                mrflog.warn("%s %s timeout in state %s"%(self.__class__.__name__,self.label,self.var.state.val))
                trans = True

            elif not pnext:
                trans = True

            if trans:
                pump_next = 0
                next_state = 'REST'
                self.set_timeout(60*60*self.var.min_wait_hours.val)   # wait 2 hours before checking again

        if pump_next != pump_curr:
            mrflog.warn("pump_next %d != pump_curr %d - setting hx_relay to %d"%(pump_next,pump_curr,pump_next))
            self.hx_relay.set(pump_next)

        if next_state != self.var.state.val:
            self.var.state.set(next_state)
            mrflog.warn("%s %s  state change to %s"%(self.__class__.__name__,self.label,self.var.state.val))
            #tg = self.mktag('hwstat', 'state')
            #dt =  { 'val' : self.var.state.val}
            #self.rm.webupdate(tg, dt)

    def pane_html(self):
        s =  """
        <h1>"""+self.label+"    "+self.var.tank_top.html+" &#176;C</h1>"

        s += self.rm.graph_inst({
            "temp" : [self.top_ts.label, self.flow_sens.label, self.return_sens.label, self.acc_sens.label],
            "relay": [self.hx_relay.label, self.rad_relay.label, self.heat_relay.label]
        })
        s += """
        <hr>
        <h2>Immersion heater</h2>"""

        s += self.timer_ctrl_table(include_list=self.tagperiods['IM'])

        s += """
        <hr>
        <h3>Status</h3>"""
        s += self.html_var_table(
            [
                self.tagperiodvar['IM'],
                self.var.tank_top.name,
                self.var.heat_relay.name
            ]
        )
        s += """
        <hr>
        <h3>Config</h3>"""

        s += self.html_var_ctrl_table(
            [
                self.var.immersion_temp.name
            ])


        s += """
        <hr>
        <h2>Heat exchanger</h2>"""

        s += self.timer_ctrl_table(include_list=self.tagperiods['HX'])

        s += """
        <hr>
        <h3>Status</h3>"""

        s += self.html_var_table(
            [
                self.tagperiodvar['HX'],
                self.var.tank_top.name,
                self.var.acc_top.name,
                self.var.hx_flow.name,
                self.var.hx_ret.name,
                self.var.state.name,
                self.var.hx_relay.name,
                self.var.rad_relay.name
            ]
        )

        s += """
        <hr>
        <h3>Config</h3>"""

        s += self.html_var_ctrl_table(
            [
                self.var.target_temp.name,
                self.var.min_head.name,
                self.var.min_wait_hours.name,
                self.var.hx_timeout_mins.name,
                self.var.delta_flow_rx.name,
                self.var.delta_flow_tank.name,
                self.var.hysteresis.name
            ]
        )

        return s
