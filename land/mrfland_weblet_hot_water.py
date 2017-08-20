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
from datetime import datetime, timedelta

class MrfLandWebletHotWater(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        self.hyst = 4.0
        self.state = 'INIT'
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPt1000):
            mrflog.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return
        ts = self.rm.senstypes[MrfSensPt1000]
        mrflog.info("num MrfSensPt1000 found was %d"%len(ts))

        ## looking for all MrfSensPtRelay types

        if not self.rm.senstypes.has_key(MrfSensPtRelay):
            mrflog.error("%s post_init failed to find sensor type MrfSensPtRelay in rm"%self.__class__.__name__)
            return
        rs = self.rm.senstypes[MrfSensPtRelay]
        mrflog.info("num MrfSensPtRelay found was %d"%len(rs))

        if not self.data.has_key('rad'):
            mrflog.error("%s , no rad in data"%self.__class__.__name__)
            return
        if not self.data.has_key('acctop'):
            mrflog.error("%s , no acctop in data"%self.__class__.__name__)
            return
        
        if not self.data.has_key('heatbox'):
            mrflog.error("%s , no heatbox in data"%self.__class__.__name__)
            return
        if not self.data.has_key('target_temp'):
            mrflog.error("%s , no target_temp in data"%self.__class__.__name__)
            return
        if not self.data.has_key('tag'):
            mrflog.error("%s , tag"%self.__class__.__name__)
            return
        
        if not self.data.has_key('litres'):
            mrflog.error("%s , litres"%self.__class__.__name__)
            return

        self.litres      = self.data['litres']
        self.target_temp = self.data['target_temp']
        

        # sort through temp sensors
        self.slabs = []
        self.sens = OrderedDict()

        reh    = re.compile(r'%s_([0-9]+)'%self.data['tag'])

        mrflog.warn("reh pattern is %s"%reh.pattern)
        reflow = re.compile(r'%s(_FLOW)'%self.data['heatbox'])
        reret  = re.compile(r'%s(_RET)'%self.data['heatbox'])
        reacc  = re.compile(r'%s'%self.data['acctop'])

        self.ts = {}
        self.flow_sens = None
        self.return_sens = None
        self.temps = {}  # hashed by hw level (%)
        self.levels = []
        for s in ts:
            mob = reh.match(s.label)
            if mob:
                level = int(mob.group(1))
                self.ts[level] = s
                mrflog.warn("reh matched  %s"%s.label)
                self.temps[level] = 0
                self.levels.append(level)
                self.ts[level].subscribe(self.tsens_callback(level))                
            elif reflow.match(s.label):
                self.flow_sens = s
                self.flow_sens.subscribe(self.flow_callback)
                mrflog.warn("%s Found flow sensor  %s"%(self.__class__.__name__,repr(self.flow_sens.label)))

            elif reret.match(s.label):
                self.return_sens = s
                self.return_sens.subscribe(self.return_callback)
                mrflog.warn("%s Found return sensor  %s"%(self.__class__.__name__,repr(self.return_sens.label)))
            elif reacc.match(s.label):
                self.acc_sens = s
                self.acc_sens.subscribe(self.acc_callback)
                mrflog.warn("%s Found acc sensor  %s"%(self.__class__.__name__,repr(self.acc_sens.label)))
        self.levels.sort(reverse=True)
        mrflog.warn("%s has temp sensors at following levels %s"%(self.label,repr(self.levels)))
        
        # sort through relays

        self.rlabs = []
        self.relays = OrderedDict()
        rehx   = re.compile(r'%s(_HX_PUMP)'%self.data['tag'])
        rerad  = re.compile(r'%s(_PUMP)'%self.data['rad'])

        self.rs = {}
        for s in rs:
            if rehx.match(s.label):
                self.hx_relay = s
                self.hx_relay.subscribe(self.hx_relay_callback)
                mrflog.warn("%s Found hx relay  %s"%(self.__class__.__name__,repr(self.hx_relay.label)))
                self.hx_relay_map = self.rm.senslookup(self.hx_relay.label)
            elif rerad.match(s.label):
                self.rad_relay = s
                self.rad_relay.subscribe(self.rad_relay_callback)
                self.rad_relay_map = self.rm.senslookup(self.rad_relay.label)
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
    def timer_callback(self, label, act):
        mrflog.warn("%s : timer_callback label %s act %s  "%(self.__class__.__name__,label,act))
        self.state_update(timeout=True)

        
    def eval_capacity(self):
        return

    def hx_relay_control(self, on_off):
        mrflog.warn("%s hx_relay_control on_off %d"%(self.__class__.__name__, on_off))
        self.hx_relay.set(on_off)
        """
        cdata = {}
        cdata['chan'] = self.hx_relay_map['chan']
        cdata['val'] = on_off
        param = PktRelayState()
        param.dic_set(cdata)       
        mrflog.warn(" sending SET_RELAY to addr %d with param %s"%
                    (self.hx_relay_map['addr'] , repr(param)))
        self.rm.devupdaten(self.tag,self.hx_relay_map['addr'],'SET_RELAY',param)
        """



        
    def rad_relay_force(self, on_off):
        mrflog.warn("%s rad_relay_force on_off %d"%(self.__class__.__name__, on_off))
        self.rad_relay.force(on_off)

    def rad_relay_release(self):
        mrflog.warn("%s rad_relay_release"%(self.__class__.__name__))
        self.rad_relay.release
        
        """
        cdata = {}
        cdata['chan'] = self.rad_relay_map['chan']
        #cdata['val'] = int(data['active'])
        cdata['val'] = on_off
        param = PktRelayState()
        param.dic_set(cdata)       
        mrflog.warn(" sending SET_RELAY to addr %d with param %s"%
                    (self.rad_relay_map['addr'] , repr(param)))
        self.rm.devupdaten(self.tag,self.rad_relay_map['addr'],'SET_RELAY',param)
        """
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
        if self.state == 'INIT':
            if timeout:
                next_state = 'IDLE'
                
        if self.state == 'IDLE':
            if self.temps[100] < (self.target_temp - self.hyst):
                if self.store_temp > (self.temps[100] + 10.0):
                    if self.flow_temp > (self.temps[100] - 5.0):
                        mrflog.warn("%s state_update to CHARGING flow_temp %.2f top temp %.2f"%(self.__class__.__name__,self.flow_temp, self.temps[100]))
                        next_state = 'CHARGING'
                        self.hx_relay_control(1)
                        
                    else:
                        next_state = 'PREPUMP'
                        self.rad_relay_force(1)
                        
        elif self.state == 'PREPUMP':
            if self.flow_temp > (self.temps[100] - 5.0):
                self.rad_relay_release()
                self.hx_relay_control(1)                
                next_state = 'CHARGING'
        elif self.state == 'CHARGING':
            if self.temps[100] > self.target_temp:
                self.hx_relay_control(0)                
                next_state = 'IDLE'
                
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
                mrflog.warn("top tank tag is %s dt %s"%(repr(tg),repr(dt)))
                self.rm.webupdate(tg, dt)
                self.state_update()
            self.eval_capacity()
        return _tscb 

    def hx_relay_callback(self, label, data ):
        mrflog.warn("HotWaterWeblet : hx_relay_callback  %s  data %s"%(label,repr(data)))
        tag = self.mktag(self.tag, label)
        mrflog.warn("HotWaterWeblet : hx_relay_callback  %s  data %s tag %s"%(label,repr(data),repr(tag)))        
        self.rm.webupdate(self.mktag('relays', label), data)

    def rad_relay_callback(self, label, data ):
        mrflog.warn("HotWaterWeblet : rad_relay_callback  %s  data %s"%(label,repr(data)))
        self.rm.webupdate(self.mktag('relays', label), data)

    
    def flow_callback(self, label, data):
        mrflog.warn("HotWaterWeblet flow callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('hwstat', 'hx_flow_temp')
        dt =  { 'val' : data['temp']}
        self.flow_temp = data['temp']
        self.rm.webupdate(tg, dt)
        self.state_update()
        
    def return_callback(self, label, data):
        mrflog.warn("HotWaterWeblet return callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('hwstat', 'hx_return_temp')
        dt =  { 'val' : data['temp']} 
        self.return_temp = data['temp']
        self.rm.webupdate(tg, dt)
        self.state_update()

    def acc_callback(self, label, data):
        mrflog.warn("HotWaterWeblet acc callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('hwstat', 'store_temp')
        dt =  { 'val' : data['temp']} 
        self.store_temp = data['temp']
        self.rm.webupdate(tg, dt)
        self.state_update()
        

    def pane_html(self):
        s =  """
        <h2>%s</h2>
        """%(self.label)
        s += MrflandObjectTable(self.tag,"hwstat", { 'val': {0}} ,['state', 'top_temp','store_temp','hx_flow_temp','hx_return_temp'], tr_hdr={ 'tag' : '', 'val': ''}, init_vals = {'state' : {'val' : self.state }})
        s += "<hr>\n"
        s += " <h3>Tank sensors</h3>\n"
        s += MrflandObjectTable(self.tag,"hwtemp", { 'temp': {0.0}} ,self.levels, tr_hdr={ 'tag' : 'level'} )
        s += "<hr>\n"
        s += " <h3>Relays</h3>\n"
        s += MrflandObjectTable(self.tag,"relays",self.rs[0]._output,self.rlabs)
        
        return s
