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

from core_tests import DeviceTestCase, mrf_cmd_app_test, DefaultAppCmds


## some app commands and structs for the time being here.. ideally would be auto discovered codes

class PktSpiDebug(MrfStruct):
    _fields_ = [
        ("spi_rx_int_cnt", c_uint16),
        ("spi_tx_int_cnt", c_uint16),
        ("spi_rx_bytes", c_uint16),
        ("spi_tx_bytes", c_uint16),
        ("spi_rxov", c_uint16),
        ("spi_rx_queue_level", c_uint16),
        ("spi_tx_queue_level", c_uint16),
        ("spi_rx_queue_data_avail", c_uint8),
        ("spi_tx_queue_data_avail", c_uint8),
        ("spi_rxq_qip", c_uint8),
        ("spi_rxq_qop", c_uint8),
        ("spi_rxq_items", c_uint8),
        ("spi_rxq_push_errors", c_uint8),
        ("spi_rxq_pop_errors", c_uint8),
        ("spi_txq_qip", c_uint8),
        ("spi_txq_qop", c_uint8),
        ("spi_txq_items", c_uint8),
        ("spi_txq_push_errors", c_uint8),
        ("spi_txq_pop_errors", c_uint8),
        ("ucb0_ifg", c_uint8),
        ("ucb0_ie", c_uint8),
        ("ucb0_cntrl0", c_uint8),
        ("ucb0_cntrl1", c_uint8),
        ("ucb0_stat", c_uint8),

       
    ]


mrf_cmd_spi_read = 129
mrf_cmd_spi_write = 130
mrf_cmd_spi_debug = 131


Pt1000AppCmds = {
    mrf_cmd_app_test : {
        'name'  : "APP_TEST",
        'param' : None,
        'resp'  : PktTimeDate
    },
    mrf_cmd_spi_read : {
        
        'name'  : "SPI_READ",
        'param' : PktUint8,
        'resp'  : PktUint8
    },
    mrf_cmd_spi_write : {
        
        'name'  : "SPI_WRITE",
        'param' : PktUint8_2,
        'resp'  : None
    },
    mrf_cmd_spi_debug : {
        
        'name'  : "SPI_DEBUG",
        'param' : None,
        'resp'  : PktSpiDebug
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
        self.host= 0x01
        #self.stub.app_cmds = Pt1000AppCmds
        self.stub.app_cmds = copy(MrfSysCmds)
        self.stub.app_cmds.update(Pt1000AppCmds)
        self.checkgit = False
        #self.host_test()

    def spi_debug(self):
        self.stub.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.stub.response(timeout=self.timeout)
        print "spi debug:\n"
        print dresp   
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
        val = resp.value
        print "address %d , read value %d"%(addr,val)

        ccode = mrf_cmd_spi_write
        paramstr = PktUint8_2()
        paramstr.value[0] = addr
        paramstr.value[1] = val

        print "paramstr = addr %d val %d "%(paramstr.value[0],paramstr.value[1])
        self.stub.cmd(self.dest,ccode,dstruct=paramstr)
        
        print "wrote spi write"

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

        self.stub.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.stub.response(timeout=self.timeout)
        print "spi debug:\n"
        print dresp

        if False:
            self.get_time_test(self.host)
            self.set_time_test(self.dest,self.host)
        
        
            self.dev_info_test(self.dest)
            self.dev_status_test(self.dest)
            self.sys_info_test(self.dest)
            self.app_info_test(self.dest)

            self.get_time_test(self.dest)
    
            return

        regvals = {}
        paramstr = PktUint8()
        for addr in xrange(0xf):        
            paramstr.value = addr
            self.stub.cmd(self.dest,mrf_cmd_spi_read,dstruct=paramstr)
            resp = self.stub.response(timeout=self.timeout)            
            print "reg %x :  %02x"%(addr, resp.value)
            regvals[addr] = resp.value
            
        err_tot = 0
        errs = {}
        chks = 0
        for loop in xrange(100):
            print "check loop %d"%loop
            for addr in xrange(0xf):
                chks = chks + 1
                paramstr.value = addr
                self.stub.cmd(self.dest,mrf_cmd_spi_read,dstruct=paramstr)
                resp = self.stub.response(timeout=self.timeout)
                if resp.value != regvals[addr]:
                    err_tot += 1
                    if not errs.has_key(addr):
                        errs[addr] = 0
                    errs[addr] = errs[addr] +  1
                    #print "addr %d errs %d"%(addr,errs[addr])
                    print "ERROR reg %02d expected %02x got %02x"%(addr,regvals[addr],resp.value)
                    
        print "loops %d err_tot %d out of %d device reads"%(loop,err_tot,chks)

        nks = errs.keys()
        nks.sort()
        print "keys %s"%repr(nks)
        for ky in nks:
            print "reg %d  errors %d"%(ky,errs[ky])
            
        #for addr in xrange(0xf):
        #    print "reg %x : %s"%(addr,regvals[addr])


        self.stub.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.stub.response(timeout=self.timeout)
        print "spi debug:\n"
        print dresp
        
        return
        #self.read_spi_test()

        addr = 7
        paramstr = PktUint8()
        paramstr.value = addr

        self.stub.cmd(self.dest,mrf_cmd_spi_read,dstruct=paramstr)
        resp = self.stub.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.read_spi_test()


        
        

        self.stub.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.stub.response(timeout=self.timeout)
        print "spi debug:\n"
        print dresp

        
        return

        """
        try:
            #self.read_spi_test()
            self.stub.cmd(self.dest,ccode,dstruct=paramstr)
            resp = self.stub.response(timeout=self.timeout)
            print "got resp:\n%s"%repr(resp)
        except:
            print "oops exception"
        """

        self.stub.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.stub.response(timeout=self.timeout)
        print "spi debug:\n"
        print dresp


        return

    
        print "device_status at start of test:\n"
        print sresp
        time.sleep(0.1)  # FIXME!

        self.stub.cmd(self.dest, mrf_cmd_device_status)

        fresp = self.stub.response(timeout=self.timeout)
        print "device_status at end of test:\n"
        print fresp
        """
        self.stub.cmd(self.dest,ccode)
        fresp = self.stub.response(timeout=self.timeout)
        print "device_status at end of test:\n"
        print fresp
        """
    def skipped_test02_burnin(self):
        for i in xrange(100):
            self.test01_device_tests()


if __name__ == "__main__":
    unittest.main()
