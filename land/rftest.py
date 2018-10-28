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

from mrfdev_rftst import *



class Rftest(DeviceTestCase):
    DEST = 0x20
    def setUp(self):
        DeviceTestCase.setUp(self)
        self.devname = 'usbrf'
        self.num_ifs = 2
        self.num_buffs = 8
        self.timeout = 0.6
        self.dest = Rftest.DEST
        self.host= 0x01
        #self.app_cmds = Pt1000AppCmds
        self.app_cmds = copy(MrfSysCmds)
        self.app_cmds.update(RftstAppCmds)
        self.checkgit = False
        #self.host_test()


    def if_status(self,i_f=0):
        paramstr = PktUint8()
        paramstr.value = i_f
        ccode = mrf_cmd_if_stats
        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktIfStats()),type(resp))

    def test001_dev_id_tests(self):
        self.dev_info_test(self.dest)
        self.dev_status_test(self.dest)
        self.if_info_test(self.dest)
        #self.sys_info_test(self.dest)


    def test002_app_info_tests(self):
        self.app_info_test(self.dest)
        self.app_cmd_info_test(self.dest)


    def test01_core_tests(self):
        self.set_time_test(self.dest,self.host)
        #self.get_time_test(self.host)
        return


    def toggle_relay(self,chan=0):
        print "**********************"
        print "* pt1000 toggle relay test (dest 0x%02x) chan %d"%(self.dest,chan)
        print "**********************"

        data = PktRelayState()
        data.chan = chan
        data.val = 0
        ## get initial relay state for chan 0
        self.cmd(self.dest,mrf_cmd_get_relay,dstruct=data)
        resp = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp)

        if not self.check_attrs(resp,PktRelayState()):
           print "ERROR : abort chan %d test (1)"%chan
           return -1
        self.assertTrue(self.check_attrs(resp,PktRelayState()))

        # toggle relay state
        if resp['val']:
           data.val = 0
        else:
           data.val = 1

        self.cmd(self.dest,mrf_cmd_set_relay,dstruct=data)
        resp2 = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp2)
        if not self.check_attrs(resp2,data,checkval=True):
           print "ERROR : abort chan %d test (1)"%chan
           return -1

        self.assertTrue(self.check_attrs(resp2,data,checkval=True))

        return 0
        ## check pt1000state has updated
        self.cmd(self.dest,mrf_cmd_read_state)
        rst = self.response(timeout=self.timeout)
        print "resp %s"%repr(rst)
        self.assertTrue(self.check_attrs(rst,PktPt1000State()))




if __name__ == "__main__":
   """
    if len(sys.argv) > 1:
       print "sys.argv = %s"%repr(sys.argv)
       #pa = sys.argv.pop()
       #print "pa is %s"%repr(pa)
       #print "sys.argv = %s"%repr(sys.argv)

       Rftest.DEST = int(sys.argv.pop(),16)
       print "setting dest to 0x%x"%Rftest.DEST
   """
   unittest.main()
