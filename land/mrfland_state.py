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
from .mrf_structs import *
from .mrfland_app import MrflandApp

class DevState(object):
    def __init__(self,addr,log):
        self.addr = addr
        self.log  = log
        self.device_info = None
        self.sys_info = None
        self.app_info = None
        self.device_status = None
        self.last_hdr = None
        self.last_msg = None
        self.last_msg_time = None
    def __repr__(self):
        if self.device_info and self.sys_info and self.app_info:
            s = "%s addr %d:%d device %s app %s version %s"%\
                (self.__class__.__name__,
                 self.device_info.netid,self.device_info.mrfid,
                 self.device_info.attstr("dev_name"),self.app_info.attstr("name"),
                 self.sys_info.attstr("mrfbus_version"))
            return s
                 
                 
    def command_request(self):
        if type(self.device_info) != type(PktDeviceInfo()) :
            return mrf_cmd_device_info
        if type(self.sys_info) != type(PktSysInfo()) :
            return mrf_cmd_sys_info
        if type(self.app_info) != type(PktAppInfo()) :
            return mrf_cmd_app_info
        if type(self.device_status) != type(PktDeviceStatus()) :
            return mrf_cmd_device_status
        return None

    def fyi(self, hdr, pkt):
        if hdr.usrc != self.addr:
            self.log.warn("not for us!! we are %d this is %d"%(self.addr,hdr.usrc))
            return

        self.last_hdr = hdr
        self.last_msg = pkt
        self.last_msg_time = datetime.now()

        if type(pkt) == type(PktDeviceInfo()) :
            #print "initial type self.device_info %s"%(type(self.device_info))
            self.device_info = pkt
            self.log.info("DevState addr 0x%02x device info is  %s"%(self.addr,repr(self.device_info)))
        elif type(pkt) == type(PktSysInfo()) :
            self.sys_info = pkt
            self.log.info("DevState addr 0x%02x sys info is  %s"%(self.addr,repr(self.sys_info)))
        if type(pkt) == type(PktAppInfo()) :
            self.app_info = pkt
            self.log.info("DevState addr 0x%02x app_info is  %s"%(self.addr,repr(self.app_info)))
        if type(pkt) == type(PktDeviceStatus()) :
            self.device_status = pkt         
            self.log.info("DevState addr 0x%02x device status is  %s"%(self.addr,repr(self.device_status)))
            return
            return '{"rx_pkts":%d,"tx_pkts":%d,"errors":%d}'%\
                (self.device_status.rx_pkts,self.device_status.tx_pkts,
                 self.device_status.errors)
             
            
class MrflandState(MrflandApp):
    def __init__(self,mld,tag="state"):
        self.log = mld.log
        self.mld = mld
        self.tag = tag
        self.host = DevState(self.mld.hostaddr,self.log)
        self.devices = {}
        self._comm_active = False  # flag indication bus communications active
        self.idle_mark = 300 # every ten minutes send idle mark cmd
        self.network_accessible = False
        self.network_last_up = None
        self.network_last_down = None
        self.task_count = 0
        self.last_msg_time = None


    def __repr__(self):
        s = "%s : host_rx_pkts %d  last msg %s"%\
            (self.__class__.__name__,
             self.host.device_status.rx_pkts,str(self.last_msg_time))
        return s
        s += "\n----host------"
        s += "\n %s"%repr(self.host)
        s += "\n----devices %d------"%len(self.devices)
        for d in list(self.devices.keys()):
            s += "\n----device addr %d------"%d
            s += "\n %s"%repr(self.devices[d])
        return s+"\n"
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
            self.mld._callback(self.tag,self.mld.hostaddr,cr)
            self.log.info("state task - chose host task %s"%repr(cr))
            return

        for da in list(self.devices.keys()):
            self.log.debug("checking command_request for %d"%da)
            cr = self.devices[da].command_request()
            if cr:
                self.log.info("state task - chose device %d task %s"%(da,repr(cr)))
                self.mld._callback(self.tag,da,cr)
                return
        
        if self.task_count % self.idle_mark == 0:
            self.mld._callback(self.tag,self.mld.hostaddr,mrf_cmd_device_status)
            self.log.info(repr(self))

    def fyi(self, hdr, rsp, robj):
        self.last_msg_time = datetime.now()
        if hdr.usrc == self.mld.hostaddr:            
            return self.host.fyi(hdr,robj)
        elif hdr.usrc in list(self.devices.keys()):
            self.log.info("state fyi : got something from dev 0x%02x %s"%(hdr.usrc,repr(robj)))
            return self.devices[hdr.usrc].fyi(hdr,robj)
        else:
            self.log.info( "mrfland state added device %d"%hdr.usrc)
            self.devices[hdr.usrc] = DevState(hdr.usrc,self.log)
        
    def cmd_devices(self,data):
        self.log.info("state cmd_devices")
