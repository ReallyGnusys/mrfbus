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
import time
import sys
import traceback
from mrf_structs import *
MRFBUFFLEN = 128
import ctypes

from teststub import StubIf

import unittest

class StubTestCase(unittest.TestCase):
    def setUp(self):
        self.stub = StubIf()

    def tearDown(self):
        self.stub.quit()

class TestMrfBus(StubTestCase):
        
    def test_device_infos(self,dests = [ 0x01, 0x2,0x20, 0x2f] ):
        ccode = mrf_cmd_device_info
        exp = PktDeviceInfo()
        exp.netid = 0x25
        exp.num_buffs = 0x10
        exp.num_ifs = 0x4
        #setattr(exp,'dev_name','hostsim')
        exp.dev_name = (ctypes.c_uint8*10)(*(bytearray('hostsim')))

        for dest in dests:
            exp.mrfid = dest
            rv = self.stub.cmd_test(dest,ccode,exp,dstruct=None)
            self.assertEqual(rv,0)
 

    def test_discover_devices(self,dests = [ 0x01, 0x2,0x20, 0x2f]):
        devs = []
        cmd_code = mrf_cmd_device_info
        for dest in range(1,0x30):
            rv = self.stub.cmd(dest,cmd_code)
            rsp = self.stub.response(0.2)
            if type(rsp) == type(PktDeviceInfo()):
                devs.append(rsp)
                print "found one at dest %x"%dest
            else:
                print "not found at dest %x"%dest

        self.assertEqual(len(devs),len(dests))

        for dev in devs:
            print dev
            self.assertTrue(dev.mrfid in dests)

if __name__ == "__main__":
    unittest.main()
