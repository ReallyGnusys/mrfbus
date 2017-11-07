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
from mrfland_weblet import MrflandWeblet, MrflandObjectTable, MrfWebletSensorVar
from mrflog import mrflog
import re
from datetime import datetime, timedelta

class MrfLandWebletHotWater(MrflandWeblet):

    _config_ = [ ('enabled'    , bool,  False ),
                 ('target_temp', float, 60.0, 40.0,65.0,1.0),
                 ('delta_targ_rx', float, 8.0, 6.0,10.0,1.0),
                 ('min_wait_mins', int , 16*60, 2*60,24*60,60),
                 ('hysteresis' , float, 4.0, 2.0, 12.0, 1.0)
    ]
    
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        self.state = 'REST'
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPt1000):
            mrflog.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return

        
        ts = self.rm.senstypes[MrfSensPt1000]
        mrflog.info("num MrfSensPt1000 found was %d"%len(ts))

        ## this app must have a params block supplied

        if not self.cdata.has_key('rad'):
            mrflog.error("%s , no rad in data"%self.__class__.__name__)
            return
        
        
        ## looking for all MrfSensPtRelay types

        if not self.rm.senstypes.has_key(MrfSensPtRelay):
            mrflog.error("%s post_init failed to find sensor type MrfSensPtRelay in rm"%self.__class__.__name__)
            return
        rs = self.rm.senstypes[MrfSensPtRelay]
        mrflog.info("num MrfSensPtRelay found was %d"%len(rs))

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

        self.litres      = self.cdata['litres']
        

        # sort through temp sensors
        self.slabs = []
        self.sens = OrderedDict()



        self.ts = self.rm.sens_search_vector(MrfSensPt1000,self.tag)
        self.top_ts = self.rm.sens_search_vector_max(MrfSensPt1000,self.tag)
        
        self.rm.graph_req(self.top_ts.label)  # ask for managed graph
        self.add_var('tank_top',  MrfWebletSensorVar(self.tag,'tank_top',self.top_ts, field='temp'))

        
        self.flow_sens = self.rm.sens_search(self.cdata['heatbox'] + "_FLOW")
        self.add_var('hx_flow',  MrfWebletSensorVar(self.tag,'hx_flow',self.flow_sens, field='temp'))
        self.rm.graph_req(self.flow_sens.label)  # ask for managed graph
        self.return_sens = self.rm.sens_search(self.tag + "_HX_RET")
        self.add_var('hx_ret',  MrfWebletSensorVar(self.tag,'hx_ret',self.return_sens, field='temp'))
        self.rm.graph_req(self.return_sens.label)  # ask for managed graph
        self.acc_sens = self.rm.sens_search(self.cdata['acctop'])
        self.add_var('acc_top',  MrfWebletSensorVar(self.tag,'acc_top',self.acc_sens, field='temp'))
        self.rm.graph_req(self.acc_sens.label)  # ask for managed graph

        
        
        ## temp old style callback hookup
        self.temps = {}  # hashed by hw level (%)

        for lev in self.ts.keys():
            self.ts[lev].subscribe(self.tsens_callback(lev))
            self.temps[lev] = 0.0
        self.flow_sens.subscribe(self.flow_callback)
        self.return_sens.subscribe(self.return_callback)
        self.acc_sens.subscribe(self.acc_callback)
        
        # sort through relays  - old style

        self.rlabs = []
        self.relays = OrderedDict()
        rehx   = re.compile(r'%s(_HX_PUMP)'%self.cdata['tag'])
        rerad  = re.compile(r'%s(_PUMP)'%self.cdata['rad'])

        self.rs = {}
        for s in rs:
            if rehx.match(s.label):
                self.hx_relay = s
                self.hx_relay.subscribe(self.hx_relay_callback)
                mrflog.warn("%s Found hx relay  %s"%(self.__class__.__name__,repr(self.hx_relay.label)))
                self.rm.graph_req(s.label)  # ask for managed graph
                self.hx_relay_map = self.rm.senslookup(self.hx_relay.label)
            elif rerad.match(s.label):
                self.rad_relay = s
                self.rad_relay.subscribe(self.rad_relay_callback)
                self.rad_relay_map = self.rm.senslookup(self.rad_relay.label)
                self.rm.graph_req(s.label)  # ask for managed graph
                mrflog.warn("%s Found rad relay  %s"%(self.__class__.__name__,repr(self.rad_relay.label)))


        # make list of relays for gwebpage
        self.rs = OrderedDict()
        self.rlabs = []
        self.rs[0] = self.hx_relay
        self.rlabs.append(self.hx_relay.label)
        self.rs[1] = self.rad_relay
        self.rlabs.append(self.rad_relay.label)
        mrflog.warn("num rs %d  labs %s"%(len(self.rs),repr(self.rlabs)))


    def run_init(self):
        mrflog.warn("%s run_init"%(self.__class__.__name__))
        # start timer
        now = datetime.now()
        td  = timedelta(seconds = 30)
        tod = now + td
        self.rm.set_timer( tod.time() , self.label , 'TO')
        self.rm.timer_reg_callback( self.label, 'TO', self.timer_callback)

    def set_timeout(self,seconds):
        # start timer
        mrflog.warn("%s : set_timeout seconds  %d  "%(self.__class__.__name__,seconds))
        now = datetime.now()
        td  = timedelta(seconds = seconds)
        tod = now + td
        self.rm.set_timer( tod.time() , self.label , 'TO')
        
    def timer_callback(self, label, act):
        mrflog.warn("%s : timer_callback label %s act %s  "%(self.__class__.__name__,label,act))
        self.state_update(timeout=True)

        
    def eval_capacity(self):
        return

    def hx_relay_control(self, on_off):
        mrflog.warn("%s hx_relay_control on_off %d"%(self.__class__.__name__, on_off))
        self.hx_relay.set(on_off)
        
    def rad_relay_force(self, on_off):
        mrflog.warn("%s rad_relay_force on_off %d"%(self.__class__.__name__, on_off))
        self.rad_relay.force(on_off)

    def rad_relay_release(self):
        mrflog.warn("%s rad_relay_release"%(self.__class__.__name__))
        self.rad_relay.release()
        
    def state_update(self, timeout = False):
        next_state = self.state
        if not hasattr(self,'store_temp'):
            return
        if not hasattr(self,'temps'):
            return
        if not hasattr(self,'flow_temp'):
            return
        if not hasattr(self,'return_temp'):
            return


        if self.vars.enabled.val == False:
            next_state = 'DISABLED'
            
        elif self.state == 'REST':
            if timeout:
                next_state = 'IDLE'
        elif self.state == 'IDLE':
            if self.vars.tank_top.val < (self.vars.target_temp.val - self.vars.hysteresis.val):
                if self.vars.acc_top.val > (self.vars.tank_top.val + 12.0):
                    if self.vars.hx_flow.val > (self.vars.tank_top.val - 3.0):
                        mrflog.warn("%s state_update to CHARGE1 flow_temp %.2f top temp %.2f"%(self.__class__.__name__,self.vars.hx_flow.val, self.vars.tank_top.val))
                        next_state = 'CHARGE1'
                        self.hx_relay_control(1)
                        self.set_timeout(60*10)

                    else:
                        next_state = 'PREPUMP'
                        self.rad_relay_force(1)
                        self.set_timeout(60*4)
                        
        elif self.state == 'PREPUMP':
            if timeout or self.vars.hx_flow.val > (self.vars.tank_top.val - 3.0):
                self.rad_relay_release()
                self.hx_relay_control(1)                
                next_state = 'CHARGE1'
                self.set_timeout(60*10)

        elif self.state == 'CHARGE1':
            if timeout:
                mrflog.warn("%s %s timeout in state %s"%(self.__class__.__name__,self.label,self.state))
                next_state = 'CHARGING'
                self.set_timeout(90*60)   #90 mins max for now
            elif self.vars.tank_top.val > self.vars.target_temp.val :
                mrflog.warn("%s %s reached target temperature in state %s"%(self.__class__.__name__,self.label,self.state))
                next_state = 'REST'
                self.set_timeout(60*self.cdata['min_wait_mins']) 
                self.hx_relay_control(0)                

            
            """
            elif self.vars.tank_top.val > (self.vars.hx_flow.val - 10.0):
                mrflog.warn("%s %s flow temp (%.2f) insufficient - giving up in state %s"%
                            (self.__class__.__name__,self.label,self.vars.hx_flow.val,self.state))
                next_state = 'REST'
                self.hx_relay_control(0)                
                self.set_timeout(60*self.cdata['min_wait_mins'])   # wait 2 hours before checking again
            """

                
        elif self.state == 'CHARGING':

            trans = False

            if timeout:
                mrflog.warn("%s %s timeout in state %s"%(self.__class__.__name__,self.label,self.state))
                trans = True
                
            elif self.vars.tank_top.val > self.vars.target_temp.val:
                mrflog.warn("%s %s reached target temperature in state %s"%(self.__class__.__name__,self.label,self.state))
                trans = True
            elif self.vars.tank_top.val > (self.vars.hx_flow.val - 5.0):
                mrflog.warn("%s %s tank top (%.2f) reached min diff re. flow temp (%.2f)  in state %s"%
                            (self.__class__.__name__,self.label,self.vars.tank_top.val,self.vars.hx_flow.val,self.state))
                trans = True
            elif self.vars.hx_ret.val > (self.vars.target_temp.val - self.vars.delta_targ_rx.val):  # don't want return to get too high
                mrflog.warn("%s %s return temp (%.2f) reached limit in state %s"%
                            (self.__class__.__name__,self.label, self.vars.hx_ret.val,self.state))
                trans = True

            if trans:
                self.hx_relay_control(0)                
                next_state = 'REST'
                self.set_timeout(60*self.cdata['min_wait_mins'])   # wait 2 hours before checking again

        if next_state != self.state:
            self.state = next_state
            mrflog.warn("%s %s  state change to %s"%(self.__class__.__name__,self.label,self.state))
            tg = self.mktag('hwstat', 'state')
            dt =  { 'val' : self.state}
            self.rm.webupdate(tg, dt)
                    
    def tsens_callback(self,level):
        def _tscb(label,data):
            mrflog.info("weblet hot_water tsens callback for level %d label %s data %s"%(level,label, repr(data)))
            self.temps[level] = data['temp']
            tg = self.mktag('hwtemp', str(level))
            mrflog.info("tag is %s"%(repr(tg)))
            self.rm.webupdate(self.mktag('hwtemp', str(level)), data)
            if level == 100:
                tg = self.mktag('hwstat', 'top_temp')
                dt =  { 'val' : data['temp']}
                mrflog.info("top tank tag is %s dt %s"%(repr(tg),repr(dt)))
                self.rm.webupdate(tg, dt)
                self.state_update()
            self.eval_capacity()
        return _tscb 

    def hx_relay_callback(self, label, data ):
        tag = self.mktag(self.tag, label)
        mrflog.info("HotWaterWeblet : hx_relay_callback  %s  data %s tag %s"%(label,repr(data),repr(tag)))        
        self.rm.webupdate(self.mktag('relays', label), data)

    def rad_relay_callback(self, label, data ):
        mrflog.info("HotWaterWeblet : rad_relay_callback  %s  data %s"%(label,repr(data)))
        self.rm.webupdate(self.mktag('relays', label), data)

    
    def flow_callback(self, label, data):
        mrflog.info("HotWaterWeblet flow callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('hwstat', 'hx_flow_temp')
        dt =  { 'val' : data['temp']}
        self.flow_temp = data['temp']
        self.rm.webupdate(tg, dt)
        self.state_update()
        
    def return_callback(self, label, data):
        mrflog.info("HotWaterWeblet return callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('hwstat', 'hx_return_temp')
        dt =  { 'val' : data['temp']} 
        self.return_temp = data['temp']
        self.rm.webupdate(tg, dt)
        self.state_update()

    def acc_callback(self, label, data):
        mrflog.info("HotWaterWeblet acc callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('hwstat', 'store_temp')
        dt =  { 'val' : data['temp']} 
        self.store_temp = data['temp']
        self.rm.webupdate(tg, dt)
        self.state_update()

    def avg_callback(self, label, data):
        mrflog.warn("HotWaterWeblet average callback for  label %s data %s"%(label, repr(data)))

        
    def pane_html(self):
        s =  """
        <h2>"""+self.label+"""</h2>
        """
        
        s += self.rm.graph_inst({
            "temp" : [self.ts[100].label, self.flow_sens.label, self.return_sens.label, self.acc_sens.label],
            "relay": [self.hx_relay.label, self.rad_relay.label]
        })
        s += MrflandObjectTable(self.tag,"hwstat", { 'val': {0}} ,['state', 'top_temp','store_temp','hx_flow_temp','hx_return_temp'], tr_hdr={ 'tag' : '', 'val': ''}, init_vals = {'state' : {'val' : self.state }})
        s += "<hr>\n"
        s += " <h3>Tank sensors</h3>\n"
        s += MrflandObjectTable(self.tag,"hwtemp", { 'temp': {0.0}} ,self.ts.keys(), tr_hdr={ 'tag' : 'level'} )
        s += "<hr>\n"
        s += " <h3>Relays</h3>\n"
        s += MrflandObjectTable(self.tag,"relays",self.rs[0]._output,self.rlabs)
        
        return s
