

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

## some app commands for the time being here.. ideally would be auto discovered codes

mrf_cmd_spi_read = 129


Pt1000AppCmds = {

   mrf_cmd_spi_read : {

       'name'  : "SPI_READ",
       'param' : PktUint8,
       'resp'  : PktUint8
   }
}

## default app def for sanity checking

mrf_cmd_app_test = 128


DefaultAppCmds = {

    mrf_cmd_app_test : {

       'name'  : "APP_TEST",
       'param' : None,
       'resp'  : PktTimeDate
   }
}


class TestPt1000(DeviceTestCase):
    def host_test(self):
        self.devname = 'hostsim'
        self.num_ifs = 4
        self.num_buffs = 16
        self.timeout = 0.6
        self.dest =0x01
        self.stub.app_cmds = DefaultAppCmds
   
    def setUp(self):
        DeviceTestCase.setUp(self)
        self.devname = 'usbrf'
        self.num_ifs = 2
        self.num_buffs = 8
        self.timeout = 0.6
        self.dest =0x02
        self.stub.app_cmds = Pt1000AppCmds
        #self.host_test()
    def read_spi_test(self, addr = 0):
        print "**********************"
        print "* read_spi test addr = %d (dest 0x%02x)"%(addr,self.dest)
        print "**********************"
        ccode = mrf_cmd_spi_read
        paramstr = PktUint8()
        paramstr.value = addr

        self.stub.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.stub.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktUint8()),type(resp))

    def host_app_test(self, addr = 0):
        print "**********************"
        print "* host_app test addr = %d (dest 0x%02x)"%(addr,self.dest)
        print "**********************"
        ccode = mrf_cmd_app_test

        self.stub.cmd(self.dest,ccode)
        resp = self.stub.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktTimeDate()),type(resp))

    
    def test01_device_tests(self):
        #self.host_app_test()
        #self.read_spi_test()
        #return
        ccode = mrf_cmd_device_status
        self.stub.cmd(self.dest,ccode)
        sresp = self.stub.response(timeout=self.timeout)
        print "device_status at start of test:\n"
        print sresp
        #return
        self.dev_info_test(self.dest)
        self.dev_status_test(self.dest)
        self.sys_info_test(self.dest,checkgit=False)
        self.app_info_test(self.dest)
        self.get_time_test(self.dest)


        print "device_status at start of test:\n"
        print sresp       
        self.stub.cmd(self.dest,ccode)
        fresp = self.stub.response(timeout=self.timeout)
        print "device_status at end of test:\n"
        print fresp
    def skipped_test02_burnin(self):
        for i in xrange(200):
            self.test01_device_tests()
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
