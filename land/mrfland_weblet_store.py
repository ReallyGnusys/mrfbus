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

class MrfLandWebletStore(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPt1000):
            mrflog.error("%s post_init failed to find sensor type MrfSensPt1000 in rm"%self.__class__.__name__)
            return
        ts = self.rm.senstypes[MrfSensPt1000]

        mrflog.info("num MrfSensPt1000 found was %d"%len(ts))
        self.slabs = []
        self.sens = OrderedDict()
        if not self.data.has_key('acc_tag'):
            mrflog.error("%s , no acc_tag in data")
            return
        if not self.data.has_key('acc_litres'):
            mrflog.error("%s , no acc_litres in data")
            return

        self.litres = self.data['acc_litres']
        reh    = re.compile(r'%s([0-9]+)'%self.data['acc_tag'])
        reflow = re.compile(r'%s(FLOW)'%self.data['acc_tag'])
        reret  = re.compile(r'%s(RET)'%self.data['acc_tag'])

        self.ts = {}
        self.flow_sens = None
        self.return_sens = None
        self.temps = {}  # hashed by acc level (%)
        self.levels = []
        for s in ts:
            mob = reh.match(s.label)
            if mob:
                level = int(mob.group(1))
                self.ts[level] = s
                self.temps[level] = 0
                self.levels.append(level)
                self.ts[level].subscribe(self.tsens_callback(level))                
            elif reflow.match(s.label):
                self.flow_sens = s
                self.flow_sens.subscribe(self.flow_callback)
                mrflog.warn("Found flow sensor  %s"%repr(self.flow_sens.label))

            elif reret.match(s.label):
                self.return_sens = s
                self.return_sens.subscribe(self.return_callback)
                mrflog.warn("Found return sensor  %s"%repr(self.return_sens.label))
        self.levels.sort(reverse=True)
        mrflog.warn("Store has temp sensors at following levels %s"%repr(self.levels))
        
    def eval_capacity(self):
        return
    
    def tsens_callback(self,level):
        def _tscb(label,data):
            mrflog.info("weblet store tsens callback for level %d label %s data %s"%(level,label, repr(data)))
            self.temps[level] = data['temp']
            tg = self.mktag('acctemp', str(level))
            mrflog.info("tag is %s"%(repr(tg)))
            self.rm.webupdate(self.mktag('acctemp', str(level)), data)
            if level == 100:
                tg = self.mktag('accstat', 'top_temp')
                dt =  { 'val' : data['temp']}
                mrflog.info("top tank tag is %s dt %s"%(repr(tg),repr(dt)))
                self.rm.webupdate(tg, dt)
            self.eval_capacity()
        return _tscb 
  
    def flow_callback(self, label, data):
        mrflog.info("weblet store flow callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('accstat', 'flow_temp')
        dt =  { 'val' : data['temp']} 
        self.rm.webupdate(tg, dt)
        
    def return_callback(self, label, data):
        mrflog.info("weblet store return callback for  label %s data %s"%(label, repr(data)))
        tg = self.mktag('accstat', 'return_temp')
        dt =  { 'val' : data['temp']} 
        self.rm.webupdate(tg, dt)

        

    def pane_html(self):
        """ just want to display pt1000sens output stucture"""
        s =  """
        <h2>Heatstore</h2>
        """
        s += MrflandObjectTable(self.tag,"accstat", { 'val': {0}} ,['top_temp','flow_temp','return_temp'], tr_hdr={ 'tag' : '', 'val': ''} )
        s += "<hr>\n"
        s += " <h3>Tank sensors</h3>\n"
        s += MrflandObjectTable(self.tag,"acctemp", { 'temp': {0.0}} ,self.levels, tr_hdr={ 'tag' : 'level'} )

        return s
