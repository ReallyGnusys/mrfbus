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
from mrfland_weblet import MrflandWeblet, MrflandObjectTable
from mrflog import mrflog
from collections import OrderedDict

class MrfLandWebletRelays(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all MrfSensPt1000 types

        if not self.rm.senstypes.has_key(MrfSensRelay):
            mrflog.error("%s post_init failed to find sensor type MrfSensRelay in rm"%self.__class__.__name__)
            return
        self.sl = self.rm.senstypes[MrfSensRelay]

        mrflog.warn("num MrfSensRelay found was %d"%len(self.sl))
        self.slabs = []
        self.sens = OrderedDict()
        for s in self.sl:
            self.slabs.append(s.label)
            self.sens[s.label] = s
        mrflog.warn("MrfSensRelay : %s"%repr(self.slabs))

        for s in self.sens.keys():
            self.sens[s].subscribe(self.sens_callback)
    def run_init(self):
        mrflog.warn("%s run_init .. clearing relay states")
        for s in self.sens.keys():
            self.sens[s].clear()

    def sens_callback(self, label, data ):
        mrflog.info("RelaysWeblet : sens_callback  %s  data %s"%(label,repr(data)))
        self.rm.webupdate(self.mktag(self.tag, label), data)



    def pane_html(self):
        """ want to display pt1000sens output stucture with column of controls"""
        s =  """
        <h2>%s</h2>"""%self.label
        if len(self.sl):
            s += MrflandObjectTable(self.tag,"relays",self.sl[0]._output,self.slabs, postcontrols = [("control","_mrf_ctrl_cb")])
        return s


    def mrfctrl_handler(self,data,wsid=None):
        mrflog.info( "mrfctrl_handler here, data was %s"%repr(data))
        if not data.has_key("tab") or not data.has_key("row"):
            mrflog.error("cmd_mrfctrl data problem in %s"%repr(data))
            return


        if not self.rm.sensors.has_key(str(data['row'])):
            mrflog.error("cmd_mrfctrl no device %s"%str(data['row']))
            mrflog.error("got %s"%repr(self.rm.sensors.keys()))
            return
        sens = self.rm.sensors[str(data['row'])]

        sens.set(data['val'])

        """
        sensmap = self.rm.senslookup(sens.label)

        if sensmap == None:
            mrflog.error("couldn't find mapping of sensor label %s"%sens.label)
            return

        cdata = {}

        cdata['chan'] = sensmap['chan']
        cdata['val'] = data['val']
        param = PktRelayState()
        param.dic_set(cdata)

        mrflog.info("cmd_mrfctrl have data %s"%repr(data))
        self.rm.devupdaten(self.tag,sensmap['addr'],'SET_RELAY',param)

        """
