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

"""
    object to maintain state of mrfbus network for mrfland
    network accessibility status is updated  by overseer process
    maintains data on devices - device info , sys info , app info 
    should maintain an up to date map of network 
"""

from datetime import datetime
from mrf_structs import *

class DevState(object):
    def __init__(self,addr):
        self.addr = addr
        self.device_info = None
        self.sys_info = None
        self.app_info = None
        self.device_status = None
        self.last_hdr = None
        self.last_msg = None
        self.last_msg_time = None
    def command_request(self):
        if type(self.device_info) != type(PktDeviceInfo()) :
            return mrf_cmd_device_info
        if type(self.sys_info) != type(PktSysInfo()) :
            return mrf_cmd_sys_info
        if type(self.app_info) != type(PktAppInfo()) :
            return mrf_cmd_app_info
        if type(self.device_status) != type(PktDeviceStatus()) :
            return mrf_cmd_dev_status
        return None

    def fyi(self, hdr, pkt):
        if hdr.usrc != self.addr:
            print "not for us!! we are %d this is %d"%(self.addr,hdr.usrc)
            return
        #print "dev %d fyi %s  cf  %s"%(hdr.usrc,type(pkt),type(PktDeviceInfo()))
        if type(pkt) == type(PktDeviceInfo()) :
            print "initial type self.device_info %s"%(type(self.device_info))
            self.device_info = pkt
            print "final type self.device_info %s"%(type(self.device_info))
        elif type(pkt) == type(PktSysInfo()) :
            self.sys_info = pkt
        if type(pkt) == type(PktAppInfo()) :
            self.app_info = pkt
        if type(pkt) == type(PktDeviceStatus()) :
            self.device_status = pkt         

        self.last_hdr = hdr
        self.last_msg = pkt
        self.last_msg_time = datetime.now()

class MrflandState(object):
    def __init__(self,mld):
        self.log = mld.log
        self.mld = mld
        self.host = DevState(self.mld.hostaddr)
        self.devices = {}
        self.idle_mark = 5 # every ten seconds send idle mark cmd
        self.network_accessible = False
        self.network_last_up = None
        self.network_last_down = None
        self.task_count = 0
    def network_is_accessible(self):
        return network_accessible

    def network_up(self):
        self.network_last_up = datetime.now()
        self.network_accessible = True

    def network_down(self):
        self.network_last_down = datetime.now()
        self.network_accessible = False
    
    def task(self):
        cr = self.host.command_request()
        self.task_count += 1
        if cr:
            self.mld.cmd(self.mld.hostaddr,cr)
        else:
            if self.task_count % self.idle_mark == 0:
                self.mld.cmd(self.mld.hostaddr,mrf_cmd_get_time)

    def fyi(self, hdr, pkt):
        if hdr.usrc == self.mld.hostaddr:            
            self.host.fyi(hdr,pkt)
        
