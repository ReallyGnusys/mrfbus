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
import os

class MrfLandWebletDevs(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all devs

        dadds = list(self.rm.devmap.keys())
        mrflog.warn("got dadds %s"%repr(dadds))
        self.devs = OrderedDict()
        dadds.sort()
        self.devnames = []
        for dadd in dadds:
            dev = self.rm.devmap[dadd]
            self.devs[dev.label] = dev
            self.devnames.append(dev.label)
            mrflog.warn("MrfLandWebletDevs added dev label %s add %s"%(dev.label, dev.address))
            self.devs[dev.label].subscribe(self.sens_callback)
        # build prototype output data

        utest = OrderedDict()
        utest['last_run'] = ''
        utest['last_result']   = ''
        self.pod = OrderedDict()
        self.pod['unit_test']   =  utest
        self.pod['dev_info']    =  PktDeviceInfo().dic()
        self.pod['dev_status']  =  PktDeviceStatus().dic()
        self.pod['sys_info']    =  PktSysInfo().dic()
        self.pod['app_info']    =  PktAppInfo().dic()

        self.test_active = {}

        for dn in self.devnames:
            self.test_active[dn] = False

    def sens_callback(self, label, data ):
        mrflog.info("DevsWeblet : sens_callback  %s  data %s"%(label,repr(data)))
        for ccode in list(data.keys()):
            if ccode == 3:
                tab = 'dev_info'  # ouch .. this is vile
            elif ccode == 4:
                tab = 'dev_status'
            elif ccode == 5:
                tab = 'sys_info'
            elif ccode == 11:
                tab = 'app_info'
            else:
                mrflog.debug("DevsWeblet unknown ccode %s"%repr(ccode))
                continue
            tag = self.mktag( tab, label )

            mrflog.debug("trying webupdate with tag %s  data %s"%(repr(tag),repr(data[ccode])))
            self.rm.webupdate(self.mktag( tab, label ), data[ccode])

    def pane_html(self):
        """ want to display pt1000sens output stucture with column of controls"""
        cls = {'start_test' : 'glyphicon-check'}
        s =  """
        <h2>%s</h2>"""%self.label
        re1 = re.compile("_")

        for tab in list(self.pod.keys()):
            s += "<hr>\n"
            s += " <h3>%s</h3>\n"%re1.sub(" ", tab)
            if tab == 'unit_test':
                s += MrflandObjectTable(self.tag,tab, self.pod[tab],list(self.devs.keys()), postcontrols = [("start_test","_mrf_ctrl_butt")],iclasses=cls)
            else:
                s += MrflandObjectTable(self.tag,tab, self.pod[tab],list(self.devs.keys()))

        return s
    def subproc_callback(self, dn):
        def _sbcb(rv):
            mrflog.warn("weblet devs subproc callback for dn %s returned %d"%(dn,rv))
            nowstr = datetime.datetime.now().isoformat()
            if rv == 0 :
                pstr = 'PASS'
            else:
                pstr = 'FAIL'
            data = {'last_result' : pstr , 'last_run': nowstr }
            self.rm.webupdate(self.mktag( 'unit_test', dn ), data)
            self.rm.server._run_updates() # ouch
        return _sbcb

    def mrfctrl_handler(self,data, wsid=None):
        mrflog.warn( "cmd_mrfctrl here, data was %s"%repr(data))
        if "tab" not in data or "row" not in data:
            mrflog.error("cmd_mrfctrl data problem in %s"%repr(data))
            return

        if data["tab"] != 'unit_test':
            mrflog.error("weblet devs cmd_mrfctrl tab was not unit_test - got %s"%(repr(data["tab"])))
            return

        dn =  data["row"]

        if dn not in self.test_active:
            mrflog.error("weblet devs cmd_mrfctrl row %s does not match any devname"%dn)
            return

        if self.test_active[dn]:
            mrflog.warn( "unit_test already active for device  %s"%repr(dn))
            return

        if dn not in self.rm.devices:
            mrflog.warn( "can't find device  %s registered"%repr(dn))
            return

        tdev = self.rm.devices[dn]
        address = tdev.address

        mrflog.warn( "starting unit_test for device  %s address 0x%x"%(repr(dn),address))





        rv = self.rm.subprocess(['/usr/bin/python3', os.path.join(os.environ['MRFBUS_HOME'],'land','test_default.py' ), hex(address)] , self.subproc_callback(dn))
        mrflog.warn("subprocess returned")
