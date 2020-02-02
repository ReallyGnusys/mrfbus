#!/usr/bin/env python3
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
from mrf_structs import *
import unittest
MRFBUFFLEN = 128
import ctypes
import random
from datetime import datetime

from core_tests import DeviceTestCase, mrf_cmd_app_test, DefaultAppCmds


class TestDefault(DeviceTestCase):
    DEST = 0x02

    def setUp(self):
        DeviceTestCase.setUp(self)
        self.devname = 'usbrf'
        self.num_ifs = 2
        self.num_buffs = 8
        self.timeout = 0.6
        self.dest = TestDefault.DEST
        self.host= 0x01
        self.app_cmds = copy(MrfSysCmds)
        self.checkgit = False


    def test001_dev_id_tests(self):
        self.dev_info_test(self.dest)
        self.dev_status_test(self.dest)
        self.sys_info_test(self.dest)


    def test002_app_info_tests(self):
        self.app_info_test(self.dest)
        self.app_cmd_info_test(self.dest)


    def test01_core_tests(self):
        self.set_time_test(self.dest,self.host)
        return
        self.app_info_test(self.dest)

        return
        if True:
            self.get_time_test(self.host)
            self.set_time_test(self.dest,self.host)


            self.dev_info_test(self.dest)
            self.dev_status_test(self.dest)
            self.sys_info_test(self.dest)
            self.app_info_test(self.dest)

            self.get_time_test(self.dest)

            return




if __name__ == "__main__":
    if len(sys.argv) > 1:
       print("sys.argv = %s"%repr(sys.argv))
       #pa = sys.argv.pop()
       #print "pa is %s"%repr(pa)
       #print "sys.argv = %s"%repr(sys.argv)

       TestDefault.DEST = int(sys.argv.pop(),16)
       print("setting dest to 0x%x"%TestDefault.DEST)
    unittest.main()
