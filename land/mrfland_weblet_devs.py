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


class MrfLandWebletDevs(MrflandWeblet):
    def post_init(self):
        self.log.info("%s post_init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all devs

        dadds = self.devmap.keys()

        self.devs = OrderedDict()
        dadds = dadds.sort()
        for dadd in dadds:
            dev = self.devmap[dadd]
            self.devs[dev.tag] = dev
            self.log.warn("MrfLandWebletDevs added dev tag %s add %s"%(dev.tag, dev.address))
        


    def sens_callback(self, label, data ):
        self.log.warn("DevsWeblet : sens_callback  %s  data %s"%(label,repr(data)))
        self.rm.webupdate(self.mktag(self.tag, label), data)
                          
        
    def pane_js_cmd(self):
        s = """
   var nobskit = True;
"""
        return s


    def pane_js(self):
        s = """
   var nobwit = True;
"""
        return s
    
    def pane_html(self):
        """ want to display pt1000sens output stucture with column of controls"""
        s =  """
        <h2>%s</h2>"""%self.label
        if len(self.sl):
            s += MrflandObjectTable(self.tag,"relays",self.sl[0]._output,self.slabs, postcontrols = [("control","_mrf_ctrl_cb")])
        return s


    def cmd_mrfctrl(self,data):
        self.log.warn( "cmd_mrfctrl here, data was %s"%repr(data))
        if not data.has_key("tab") or not data.has_key("row"):
            self.log.error("cmd_mrfctrl data problem in %s"%repr(data))
            return


        if not self.rm.sensors.has_key(str(data['row'])):
            self.log.error("cmd_mrfctrl no device %s"%str(data['row']))
            self.log.error("got %s"%repr(self.rm.sensors.keys()))
            return

        sens = self.rm.sensors[str(data['row'])]

        cdata = {}

        cdata['chan'] = sens.channel
        cdata['val'] = data['val']
        param = PktRelayState()
        param.dic_set(cdata)
        
        self.log.warn("cmd_mrfctrl have data %s"%repr(data))
        self.rm.devupdate(self.tag,sens.address,mrf_cmd_set_relay,param)
        """
        dest = self.rm.devices(row).address
        """
    
