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
from mrf_sens import MrfSens, MrfDev
from mrfland_weblet import MrflandWeblet, MrflandObjectTable


class MrfLandWebletTimers(MrflandWeblet):
    def post_init(self):
        self.log.info("%s post_init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensPtRelay):
            self.log.error("%s post_init failed to find sensor type MrfSensPtRelay in rm"%self.__class__.__name__)
            return
        self.sl = self.rm.senstypes[MrfSensPtRelay]

        self.log.info("num MrfSensPtRelay found was %d"%len(self.sl))
        self.slabs = []
        self.sens = OrderedDict()
        for s in self.sl:
            self.slabs.append(s.label)
            self.sens[s.label] = s
        self.log.info("MrfSensPtRelay : %s"%repr(self.slabs))

        for s in self.sens.keys():
            self.sens[s].subscribe(self.sens_callback)
    def sens_callback(self, label, data ):
        self.log.info("TimersWeblet : sens_callback  %s  data %s"%(label,repr(data)))
        self.rm.webupdate(self.mktag(self.tag, label), data)
                          
        
    
    def pane_html(self):
        """ want to display pt1000sens output stucture with column of controls"""
        s =  """
        <h2>%s</h2>"""%self.label
        if len(self.sl):
            s += MrflandObjectTable(self.tag,"relays",self.sl[0]._output,self.slabs, postcontrols = [("control","_mrf_ctrl_timepick")])
        return s


    def cmd_mrfctrl(self,data):
        self.log.info( "cmd_mrfctrl here, data was %s"%repr(data))
        if not data.has_key("tab") or not data.has_key("row"):
            self.log.error("cmd_mrfctrl data problem in %s"%repr(data))
            return


        if not self.rm.sensors.has_key(str(data['row'])):
            self.log.error("cmd_mrfctrl no device %s"%str(data['row']))
            self.log.error("got %s"%repr(self.rm.sensors.keys()))
            return
        sens = self.rm.sensors[str(data['row'])]

        sensmap = self.rm.senslookup(sens.label)

        if sensmap == None:
            self.log.error("couldn't find mapping of sensor label %s"%sens.label)
            return

        cdata = {}

        cdata['chan'] = sensmap['chan']
        cdata['val'] = data['val']
        param = PktRelayState()
        param.dic_set(cdata)
        
        self.log.info("cmd_mrfctrl have data %s"%repr(data))
        self.rm.devupdaten(self.tag,sensmap['addr'],'SET_RELAY',param)
        """
        dest = self.rm.devices(row).address
        """
    
