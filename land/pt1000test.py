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
import unittest
MRFBUFFLEN = 128
import ctypes

from core_tests import DeviceTestCase


class TestPt1000(DeviceTestCase):
    def setUp(self):
        DeviceTestCase.setUp(self)
        self.devname = 'usbrf'
        self.num_ifs = 2
        self.num_buffs = 8
        self.timeout = 0.6
        self.dest =0x02




    def test01_device_tests(self):

        self.dev_info_test(self.dest)
        self.dev_status_test(self.dest)
        self.sys_info_test(self.dest)

if __name__ == "__main__":
    unittest.main()
    """
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
    """
