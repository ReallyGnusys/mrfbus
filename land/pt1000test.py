#!/usr/bin/env python
'''  Copyright (c) 2012-16 Gnusys Ltd

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
import os
import threading
import Queue
import subprocess
import time
import sys
import traceback
from mrf_structs import *
MRFBUFFLEN = 128
import ctypes

from teststub import StubIf


class TestMrfBus():

    def __init__(self):
        self.timeout = 0.5
        self.stub = StubIf()

    def assertEqual(self,a,b):
        return a == b
    def assertTrue(self,a):
        return a

    def dev_info_test(self,dest):
        ccode = mrf_cmd_device_info
        exp = PktDeviceInfo()
        exp.netid = 0x25
        exp.num_buffs = 8
        exp.num_ifs = 2
        #setattr(exp,'dev_name','usbrf')
        exp.dev_name = (ctypes.c_uint8*10)(*(bytearray('usbrf')))
        exp.mrfid = dest
        rv = self.stub.cmd_test(dest,ccode,exp,dstruct=None)
        self.assertEqual(rv,0)
        
    def dev_status_test(self,dest):
        ccode = mrf_cmd_device_status

        self.stub.cmd(dest,ccode)
        resp = self.stub.response(timeout=self.timeout)
        
        print "dev_status_test : dest 0x%u , received :\n%s"%(dest,repr(resp))

        self.assertEqual(type(PktDeviceStatus()),type(resp))
        # assumes this is  linux hostsim 
        self.assertEqual(resp.num_if,4)
        self.assertEqual(resp.buffs_total,0x10)
        self.assertEqual(resp.errors,0)

        ## check rx tx pkts increment
        rxp = resp.rx_pkts
        txp = resp.tx_pkts

        self.stub.cmd(dest,ccode)
        resp = self.stub.response(timeout=self.timeout)
        
        self.assertEqual(resp.rx_pkts, rxp+1)
        self.assertEqual(resp.tx_pkts, txp+1)
    def sys_info_test(self,dest):
        gitversion = subprocess.check_output(["git", "rev-parse",'HEAD']).rstrip('\n')

        ccode = mrf_cmd_sys_info
        self.stub.cmd(dest,ccode)
        resp = self.stub.response(timeout=self.timeout)
        print "got resp %s"%repr(resp)
        self.assertEqual(type(PktSysInfo()),type(resp))

        exp = PktSysInfo()
        exp.mrfbus_version = (ctypes.c_uint8*40)(*(bytearray(gitversion)))
        att1 = getattr(exp,'mrfbus_version')
        ever = exp.attstr(att1)
        att1 = getattr(resp,'mrfbus_version')
        rver = resp.attstr(att1)
        
        self.assertEqual(ever,rver)


    def app_info_test(self,dest):
        gitversion = subprocess.check_output(["git", "rev-parse",'HEAD']).rstrip('\n')

        ccode = mrf_cmd_sys_info
        self.stub.cmd(dest,ccode)
        resp = self.stub.response(timeout=self.timeout)
        print "got resp %s"%repr(resp)
        self.assertEqual(type(PktSysInfo()),type(resp))


    def test01_discover_devices(self,dests = [0x2]):
        devs = []
        cmd_code = mrf_cmd_device_info
        for dest in range(1,0x30):
            rv = self.stub.cmd(dest,cmd_code)
            rsp = self.stub.response(timeout=self.timeout)
            if type(rsp) == type(PktDeviceInfo()):
                devs.append(rsp)
                print "found device at dest %x"%dest
                print repr(rsp)

        self.assertEqual(len(devs),len(dests))
        found = []
        for dev in devs:
            print dev
            self.assertTrue(dev.mrfid in dests)
            self.assertTrue(dev.mrfid not in found)
            found.append(dev.mrfid)


    def device_tests(self,dest):
        self.dev_info_test(dest)
        self.dev_status_test(dest)
        self.sys_info_test(dest)

    def test02_device_tests(self,dests =  [0x2]):
        for dest in dests:
            self.device_tests(dest)
    def test03_device_tests_repeat(self):
        for i in xrange(10):
            self.test02_device_tests()


if __name__ == "__main__":
    tst = TestMrfBus()
    try:
        tst.dev_info_test(0x2)
        tst.dev_status_test(0x2)
        tst.sys_info_test(0x2)
        tst.test03_device_tests_repeat()
    except Exception as inst:
        print "exception...doh!"
        print type(inst)     # the exception instance
        print inst.args      # arguments stored in .args
        print inst           # __str__ allows args to be printed directly
        traceback.print_exc(file=sys.stdout)
    tst.stub.quit()
