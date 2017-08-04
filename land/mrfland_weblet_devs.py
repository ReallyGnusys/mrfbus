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
import re


class MrfLandWebletDevs(MrflandWeblet):
    def post_init(self):
        mrflog.info("%s post_init"%(self.__class__.__name__))
        # do subscriptions here
        ## looking for all devs

        dadds = self.rm.devmap.keys()
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
        mrflog.warn("DevsWeblet : sens_callback  %s  data %s"%(label,repr(data)))
        for ccode in data.keys():
            if ccode == 3:
                tab = 'dev_info'  # ouch
            elif ccode == 4:
                tab = 'dev_status'
            elif ccode == 5:
                tab = 'sys_info'
            elif ccode == 11:
                tab = 'app_info'
            else:
                mrflog.error("DevsWeblet unknown ccode %s"%repr(ccode))
                continue
            tag = self.mktag( tab, label )

            mrflog.warn("trying webupdate with tag %s  data %s"%(repr(tag),repr(data[ccode])))
            self.rm.webupdate(self.mktag( tab, label ), data[ccode])
                                      
    def pane_html(self):
        """ want to display pt1000sens output stucture with column of controls"""
        cls = {'start_test' : 'glyphicon-check'}
        s =  """
        <h2>%s</h2>"""%self.label
        re1 = re.compile("_")
        
        for tab in self.pod.keys():
            s += "<hr>\n"
            s += " <h3>%s</h3>\n"%re1.sub(" ", tab)
            if tab == 'unit_test':
                s += MrflandObjectTable(self.tag,tab, self.pod[tab],self.devs.keys(), postcontrols = [("start_test","_mrf_ctrl_butt")],iclasses=cls)
            else:
                s += MrflandObjectTable(self.tag,tab, self.pod[tab],self.devs.keys())

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
            
        return _sbcb 
    
    def cmd_mrfctrl(self,data):
        mrflog.warn( "cmd_mrfctrl here, data was %s"%repr(data))
        if not data.has_key("tab") or not data.has_key("row"):
            mrflog.error("cmd_mrfctrl data problem in %s"%repr(data))
            return

        if data["tab"] != 'unit_test':
            mrflog.error("weblet devs cmd_mrfctrl tab was not unit_test - got %s"%(repr(data["tab"])))
            return

        dn =  data["row"]
        
        if not self.test_active.has_key(dn):
            mrflog.error("weblet devs cmd_mrfctrl row %s does not match any devname"%dn)
            return

        if self.test_active[dn]:
            mrflog.warn( "unit_test already active for device  %s"%repr(dn))
            return

        mrflog.warn( "starting unit_test for device  %s"%repr(dn))
        
        if dn == 'host':  #FIXME this shouldn't be hardcoded - and needs mrfid as param
            tp =  'hosttest.py'
        elif dn == 'pt1000_boiler_room':
            tp = 'pt1000test.py'
        elif dn == 'heatbox_kitchen':
            tp = 'heatbox_test.py'
        else:
            mrflog.error("weblet devs cmd_mrfctrl unexpected dev name %s : no idea which unit_test"%dn)
            return
        rv = self.rm.subprocess(['/usr/bin/python', tp ] , self.subproc_callback(dn))
        mrflog.warn("subprocess returned")
        
                         

