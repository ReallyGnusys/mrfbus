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
import queue
import subprocess
import time
from copy import copy
import sys
import traceback
from .mrf_structs import *
import unittest
MRFBUFFLEN = 128
import ctypes
import random
from datetime import datetime

from .core_tests import DeviceTestCase, mrf_cmd_app_test, DefaultAppCmds

from .mrfdev_lnxtst import *



class TestLnxtst(DeviceTestCase):
    DEST = 0x02
    def host_test(self):
        self.devname = 'hostsim'
        self.num_ifs = 4
        self.num_buffs = 16
        self.timeout = 0.6
        self.dest = TestLnxtst.DEST
        self.app_cmds = DefaultAppCmds

    def setUp(self):
        DeviceTestCase.setUp(self)
        self.devname = 'usbrf'
        self.num_ifs = 2
        self.num_buffs = 8
        self.timeout = 0.6
        self.dest = TestLnxtst.DEST
        self.host= 0x01
        #self.app_cmds = LnxtstAppCmds
        self.app_cmds = copy(MrfSysCmds)
        self.app_cmds.update(LnxtstAppCmds)
        self.checkgit = False
        #self.host_test()


        

    def if_status(self,i_f=0):
        paramstr = PktUint8()
        paramstr.value = i_f
        ccode = mrf_cmd_if_stats
        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        print("got resp:\n%s"%repr(resp))
        self.assertEqual(type(PktIfStats()),type(resp))
        
        

    def host_app_test(self, addr = 0):
        print("**********************")
        print("* host_app test addr = %d (dest 0x%02x)"%(addr,self.dest))
        print("**********************")
        ccode = mrf_cmd_app_test
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)
        print("got resp:\n%s"%repr(resp))
        self.assertEqual(type(PktTimeDate()),type(resp))


    def test001_dev_id_tests(self):
        self.dev_info_test(self.dest)
        self.dev_status_test(self.dest)
        self.sys_info_test(self.dest)


    def test002_app_info_tests(self):
        self.app_info_test(self.dest)
        self.app_cmd_info_test(self.dest)

    
    def test01_core_tests(self):
        self.set_time_test(self.dest,self.host)
        #self.get_time_test(self.host)
        return

    def test04_app_cmd_test(self):
        print("**********************")
        print("* PT1000 app_cmd_test (dest 0x%02x)"%(self.dest))
        print("**********************")
        print("Sending mrf_app_cmd_test")
        ccode = mrf_app_cmd_test
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)
        self.assertTrue(self.check_attrs(resp,PktTimeDate()))
        print("pt1000 app_cmd_test PASSED")


    def test05_read_mstats(self):
        print("**********************")
        print("* lnxtst read_mstats test (dest 0x%02x)"%self.dest)
        print("**********************")
       
        ccode = mrf_app_cmd_mstats
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)
        print("resp %s"%repr(resp))
        self.assertTrue(self.check_attrs(resp,PktLnxMemStats()))
        print("pt1000 read_state test PASSED")

        


if __name__ == "__main__":
    if len(sys.argv) > 1:
       print("sys.argv = %s"%repr(sys.argv))
       #pa = sys.argv.pop()
       #print "pa is %s"%repr(pa)
       #print "sys.argv = %s"%repr(sys.argv)
       
       TestLnxtst.DEST = int(sys.argv.pop(),16)
       print("setting dest to 0x%x"%TestLnxtst.DEST)
    unittest.main()
