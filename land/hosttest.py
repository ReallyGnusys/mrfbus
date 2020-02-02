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

from .core_tests import DeviceTestCase, mrf_cmd_app_test, DefaultAppCmds
from .mrfdev_host import *


class TestHost(DeviceTestCase):
    DEST = 0x01
    def setUp(self):
        DeviceTestCase.setUp(self)
        self.devname = 'host'
        self.num_ifs = 4
        self.num_buffs = 16
        self.timeout = 0.6
        self.dest =0x01
        self.host= 0x01
        #self.app_cmds = Pt1000AppCmds
        self.app_cmds = copy(MrfSysCmds)
        self.checkgit = False
        #self.host_test()




    def if_status(self,i_f=0):
        paramstr = PktUint8()
        paramstr.value = i_f
        ccode = mrf_cmd_if_stats
        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        print(("got resp:\n%s"%repr(resp)))
        self.assertEqual(type(PktIfStats()),type(resp))



    def host_app_test(self, addr = 0):
        print("**********************")
        print(("* host_app test addr = %d (dest 0x%02x)"%(addr,self.dest)))
        print("**********************")
        ccode = mrf_cmd_app_test
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)
        print(("got resp:\n%s"%repr(resp)))
        self.assertEqual(type(PktTimeDate()),type(resp))


    def test001_dev_id_tests(self):
        self.dev_info_test(self.dest)
        self.dev_status_test(self.dest)
        self.sys_info_test(self.dest)


    def test002_app_info_tests(self):
        self.app_info_test(self.dest)
        self.app_cmd_info_test(self.dest)


    def test01_core_tests(self):
        #self.set_time_test(self.dest,self.host)
        #self.get_time_test(self.host)
        return
        self.app_info_test(self.dest)




    def skipped_test02_burnin(self):
        for i in range(100):
            self.test01_device_tests()


if __name__ == "__main__":
    if len(sys.argv) > 1:
       TestHost.DEST = int(sys.argv.pop(),16)
       print(("setting dest to 0x%x"%TestHost.DEST))
    unittest.main()
