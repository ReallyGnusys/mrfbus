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
import mrflog


class MrfLandWebletDevs(MrflandWeblet):
    def post_init(self):
        mrflog.info("%s post_init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all devs

        dadds = self.rm.devmap.keys()
        mrflog.warn("got dadds %s"%repr(dadds))
        self.devs = OrderedDict()
        dadds.sort()
        for dadd in dadds:
            dev = self.rm.devmap[dadd]
            self.devs[dev.label] = dev
            mrflog.warn("MrfLandWebletDevs added dev label %s add %s"%(dev.label, dev.address))
            
        # build prototype output data

        self.pod = OrderedDict()
        self.pod['dev_info']    =  PktDeviceInfo().dic()        
        self.pod['dev_status']  =  PktDeviceStatus().dic()        
        self.pod['sys_info']    =  PktSysInfo().dic()
        self.pod['app_info']       =  PktAppInfo().dic()

            

    def sens_callback(self, label, data ):
        mrflog.warn("DevsWeblet : sens_callback  %s  data %s"%(label,repr(data)))
        self.rm.webupdate(self.mktag(self.tag, label), data)
                                      
    def pane_html(self):
        """ want to display pt1000sens output stucture with column of controls"""
        s =  """
        <h2>%s</h2>"""%self.label
        for tab in self.pod.keys():
            s += " <h3>%s</h3>\n"%tab
            s += MrflandObjectTable(self.label,tab, self.pod[tab],self.devs.keys())
        return s


