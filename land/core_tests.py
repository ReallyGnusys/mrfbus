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
import subprocess
import time
import sys
import traceback
from mrf_structs import *
MRFBUFFLEN = 128
import ctypes

#from teststub import StubTestCase
from landtest import LandTestCase

import unittest


## default app def for sanity checking

mrf_cmd_app_test = 128


DefaultAppCmds = {

    mrf_cmd_app_test : {
       'name'  : "APP_TEST",
       'param' : None,
       'resp'  : PktTimeDate
   }
}


class DeviceTestCase(LandTestCase):

    def setUp(self):
        print "DeviceTestCase setup"
        LandTestCase.setUp(self)
        self.devname = 'hostsim'
        self.num_ifs = 4
        self.num_buffs = 16
        self.checkgit = True

    def dev_info_test(self,dest):
        print "*********************************"
        print "* device info test (dest 0x%02x)"%dest
        print "*********************************"
        ccode = mrf_cmd_device_info
        exp = PktDeviceInfo()
        exp.netid = 0x25
        exp.num_buffs = self.num_buffs
        exp.num_ifs = self.num_ifs
        exp.dev_name = (ctypes.c_uint8*10)(*(bytearray(self.devname)))
        #exp.dev_name = 'host'
        #setattr(exp,'dev_name','host')
        #exp.dev_name = (ctypes.c_uint8*10)(*(bytearray("host")))
        exp.mrfid = dest
        rv = self.cmd_test(dest,ccode,exp,dstruct=None)
        self.assertEqual(rv,0)
        if rv == 0:
            print "PASSED device info test (dest 0x%02x)"%dest
        else:
            print "FAILED device info test (dest 0x%02x)"%dest
            

    def dev_status(self,dest):
        ccode = mrf_cmd_device_status
        self.cmd(dest,ccode)
        resp = self.response(timeout=self.timeout)
        return resp
    
    def dev_status_test(self,dest):
        print "**********************"
        print "* device status test (dest 0x%02x)"%dest
        print "**********************"
        resp = self.dev_status(dest)
        print "dev_status_test : dest %u , received :\n%s"%(dest,repr(resp))

        self.assertTrue(self.check_attrs(resp,PktDeviceStatus()))
        # assumes this is  linux hostsim 
        self.assertEqual(resp['errors'], 0)

        ## check rx tx pkts increment
        rxp = resp['rx_pkts']
        txp = resp['tx_pkts']

        resp = self.dev_status(dest)
                
        self.assertEqual(resp['rx_pkts'], rxp+1)
        self.assertEqual(resp['tx_pkts'], txp+1)
    def sys_info_test(self,dest):
        print "**********************"
        print "* sys info test   (dest 0x%02x)"%dest
        print "**********************"
        gitversion = subprocess.check_output(["git", "rev-parse",'HEAD']).rstrip('\n')

        ccode = mrf_cmd_sys_info
        self.cmd(dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "****got resp:\n%s"%repr(resp)
        pass_fail = self.check_attrs(resp,PktSysInfo())
        self.assertTrue(pass_fail)

        if resp['num_cmds'] != MRF_NUM_SYS_CMDS:
            pass_fail = False
        self.assertTrue(pass_fail)
                                     

        if self.checkgit and gitversion != resp["mrfbus_version"]:
            pass_fail = False
            self.assertTrue(pass_fail)

        if pass_fail:
            print "PASSED sys_info test (dest 0x%02x)"%dest
        else:
            print "FAILED sys_info test (dest 0x%02x)"%dest
            

    def sys_cmd_info_test(self,dest):
        print "**********************"
        print "* sys cmd info test   (dest 0x%02x)"%dest
        print "**********************"
        gitversion = subprocess.check_output(["git", "rev-parse",'HEAD']).rstrip('\n')

        ccode = mrf_cmd_sys_info
        self.cmd(dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertTrue(self.check_attrs(resp,PktSysInfo()))


        self.assertEqual(resp['num_cmds'],MRF_NUM_SYS_CMDS)

        
        if self.checkgit:
            print "checking git hash - self.checkgit = %s , gitcheck set"%self.checkgit
            self.assertEqual(gitversion,resp["mrfbus_version"])

        ## long winded faff to check git hash
        """
        exp = PktSysInfo()
        exp.mrfbus_version = (ctypes.c_uint8*40)(*(bytearray(gitversion)))
        att1 = getattr(exp,'mrfbus_version')
        ever = exp.attstr(att1)
        att1 = getattr(resp,'mrfbus_version')
        rver = resp.attstr(att1)
        
        if self.checkgit:
            print "checking git hash - self.checkgit = %s , gitcheck set"%self.checkgit
            self.assertEqual(ever,rver)
        """
        ccode = mrf_cmd_cmd_info   # eyup
        paramstr = PktUint8()
        for cmdnum in xrange(MRF_NUM_SYS_CMDS): # oooer!
            paramstr.value = cmdnum
            print "trying to get cmd info for cmd %d"%cmdnum
            self.cmd(dest,ccode,dstruct=paramstr)
            resp = self.response(timeout=self.timeout)
            print "got resp:\n%s"%str(resp)
            self.assertTrue(self.check_attrs(resp,PktCmdInfo()))
            
    def get_time_test(self,dest):
        print "**********************"
        print "* get time test (dest 0x%02x)"%dest
        print "**********************"
        ccode = mrf_cmd_get_time
        self.cmd(dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertTrue(self.check_attrs(resp,PktTimeDate()))

    def set_time_test(self,dest,ref):
        print "**********************"
        print "* set time test (dest 0x%02x)"%dest
        print "**********************"
        ccode = mrf_cmd_get_time
        self.cmd(ref,ccode)
        reftime = self.response(timeout=self.timeout)
        print "ref node time (addr 0x%0x :\n%s"%(ref,repr(reftime))

        result = self.check_attrs(reftime,PktTimeDate())
        self.assertTrue(result)

        self.cmd(dest,mrf_cmd_set_time,dstruct=reftime)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        result2 = self.check_attrs(resp,PktTimeDate())
        self.assertTrue(result2)

        
        self.cmd(dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        result3 = self.check_attrs(resp,PktTimeDate())
        self.assertTrue(result3)

        if result and result2 and result3:
            print "PASSED set time test (dest 0x%02x)"%dest
        else:
            print "FAILED set time test (dest 0x%02x)"%dest
            
        print "set_time_test PASSED"
    def app_info_test(self,dest):
        print "**********************"
        print "* app info test (dest 0x%02x)"%dest
        print "**********************"
        ccode = mrf_cmd_app_info
        self.cmd(dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        res = self.check_attrs(resp,PktAppInfo())
        if res:
            print "PASSED app info test (dest 0x%02x)"%dest
        else:
            print "FAILED app info test (dest 0x%02x)"%dest
            
        self.assertTrue(res)
            
            
    def app_cmd_info_test(self,dest):
        print "**********************"
        print "* app cmd info test (dest 0x%02x)"%dest
        print "**********************"
        ccode = mrf_cmd_app_info
        self.cmd(dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertTrue(self.check_attrs(resp,PktAppInfo()))
        num_cmds = resp['num_cmds']
        ccode = mrf_cmd_app_cmd_info   # eyup
        paramstr = PktUint8()
        for cmdnum in xrange(num_cmds): # oooer!
            paramstr.value = cmdnum
            print "trying to get app cmd info for cmd %d"%cmdnum
            self.cmd(dest,ccode,dstruct=paramstr)
            resp = self.response(timeout=self.timeout)
            print "got resp:\n%s"%str(resp)


    def device_tests(self,dest):
        self.dev_info_test(dest)
        self.dev_status_test(dest)
        self.sys_info_test(dest)
        self.sys_cmd_info_test(dest)
        self.app_info_test(dest)
        self.app_cmd_info_test(dest)
        self.get_time_test(dest)

class TestMrfBus(DeviceTestCase):

        
    @unittest.skip("temp disabled - too long")
    def test01_discover_devices(self,dests = [ 0x01, 0x2,0x20, 0x2f]):
        devs = []
        cmd_code = mrf_cmd_device_info
        for dest in range(1,0x30):
            rv = self.cmd(dest,cmd_code)
            rsp = self.response(timeout=self.timeout)
            if self.check_attrs(rsp,PktDeviceInfo()):
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



    def test02_device_tests(self, dests=[ 0x01, 0x2,0x20, 0x2f ] ):
        for dest in dests:
            self.device_tests(dest)

    @unittest.skip("temp disabled - too long")
    def test03_device_tests_repeat(self):
        for i in xrange(10):
            self.test02_device_tests()


if __name__ == "__main__":
    unittest.main()
