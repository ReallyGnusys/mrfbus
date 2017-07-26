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
from mrf_sens_timer import MrfSensTimer
from mrf_sens import MrfSens, MrfDev
from mrfland_weblet import MrflandWeblet, MrflandObjectTable
from collections import OrderedDict
import mrflog

class MrfLandWebletTimers(MrflandWeblet):
    def post_init(self):
        mrflog.info("%s post_init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensTimer):
            mrflog.error("%s post_init failed to find sensor type MrfSensTimer in rm"%self.__class__.__name__)
            self.sl = {}
            return
        self.sl = self.rm.senstypes[MrfSensTimer]

        mrflog.info("num MrfSensTimer found was %d"%len(self.sl))
        self.slabs = []
        self.sens = OrderedDict()

        self._init_vals = {}
        for s in self.sl:
            self.slabs.append(s.label)
            self.sens[s.label] = s
            self._init_vals[s.label] = { 'on' : '00:00' , 'off' : '00:00' }
        mrflog.info("MrfSensTimer : %s"%repr(self.slabs))

        for s in self.sens.keys():
            sens = self.sens[s]
            sens.subscribe(self.sens_callback)

    def run_init(self):
        mrflog.warn("weblet %s run_init"%self.__class__.__name__)
        for s in self.sens.keys():
            sens = self.sens[s]
            for act in ['on','off']:
                tod = sens.output[act]
                self.rm.set_timer( tod , sens.label , act)
                self.rm.timer_reg_callback( sens.label, act, self.timer_callback)
        
    def timer_callback(self, label, act):
        mrflog.warn("%s : timer_callback label %s act %s  "%(self.__class__.__name__,label,act))
        if not self.sens.has_key(label):
            mrflog.error("%s no sensor with label %s"%(self.__class__.__name__, label))
            return
        sens = self.sens[label]

        if not sens.output.has_key(act):
            mrflog.error("%s sensor %s has now output field %s"%(self.__class__.__name__, label, act))
            return

        tod = sens.output[act]

        ival = {}
        ival[act] = {'hour':tod.hour, 'minute':tod.minute}
        iparam = {'cname': unicode(act), 'val' : ival}
        mrflog.warn("%s : timer_callback refreshing sensor input %s : cname %s val %s"%(self.__class__.__name__,label,
                                                                                        act, repr(iparam)))
        sens.input( iparam)
        
        
    def sens_callback(self, label, data ):
        tag = self.mktag(self.tag, label)
        mrflog.warn("TimersWeblet : sens_callback  %s tag %s  data %s "%(label,repr(tag),repr(data)))
        self._init_vals[label] = data
        
        self.rm.webupdate(self.mktag(self.tag, label), data)
    
        
    
    def pane_html(self):
        """ want to display pt1000sens output stucture with column of controls"""
        s =  """
        <h2>%s</h2>"""%self.label
        if len(self.sl):
            mrflog.warn("timer init_vals is %s"%repr(self._init_vals))
            s += MrflandObjectTable(self.tag, "timers", self.sl[0]._output, self.slabs, postcontrols = [("on","_mrf_ctrl_timepick"),("off","_mrf_ctrl_timepick")], init_vals = self._init_vals)
        return s


    def cmd_mrfctrl(self,data):
        mrflog.info( "cmd_mrfctrl here, data was %s"%repr(data))
        if not data.has_key("tab") or not data.has_key("row"):
            mrflog.error("cmd_mrfctrl data problem in %s"%repr(data))
            return


        if not self.rm.sensors.has_key(str(data['row'])):
            mrflog.error("cmd_mrfctrl no device %s"%str(data['row']))
            mrflog.error("got %s"%repr(self.rm.sensors.keys()))
            return
        sens = self.rm.sensors[str(data['row'])]

        sensmap = self.rm.senslookup(sens.label)

        if sensmap == None:
            mrflog.error("couldn't find mapping of sensor label %s"%sens.label)
            return
        mrflog.warn("%s cmd_mrfctrl sens = %s got data %s"%
                      (self.__class__.__name__, sens.label,repr(data)))
        # timer is sensor of user input more than anything
        inp = {}
        inp['cname'] = data['fld']  
        inp['val'] = data['val']

        sens.input(inp)
        # update ioloop timers...hmpff
        for act in ['on','off']:
            tod = sens.output[act]
            self.rm.set_timer( tod , sens.label , act)
