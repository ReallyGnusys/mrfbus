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


## Resistance and RTD calcs 
from math import *

def GetPlatinumRTD(R):
   A=3.9083e-3
   B=-5.775e-7
   R0 = 1000.0
   R=R/R0
   T=0.0-A
   T = T +  sqrt((A*A) - 4.0 * B * (1.0 - R))
   T = T/ (2.0 * B)
   return T

def eval_rx(n,ref=3560.0):  # must subtract series 47 ohm on outputs
    return (n*ref/((2**15)-1)) - 47.0

def eval_rxm(n,ref=3560000):  # must subtract series 47 ohm on outputs
    return (n*ref/((2**15)-1)) - 47000


def eval_temp(n):
    return(GetPlatinumRTD(eval_rx(n)))


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

MAX_RTDS = 7

class PktPt1000State(MrfStruct):
    _fields_ = [
        ("td",PktTimeDate),
        ("relay_cmd", c_uint8),
        ("relay_state", c_uint8),
        ("milliohms", c_uint32*MAX_RTDS),
        ("ref_r",c_uint32),
        ("ref_i",c_uint32),
        ]

class PktRelayState(MrfStruct):
    _fields_ = [
        ("chan", c_uint8),
        ("val", c_uint8),
        ]

    

mrf_app_cmd_test = 128
mrf_cmd_spi_read = 129
mrf_cmd_spi_write = 130
mrf_cmd_spi_debug = 131
mrf_cmd_spi_data  = 132
mrf_cmd_config_adc  = 133
mrf_cmd_read_state  = 134
mrf_cmd_get_relay   = 135
mrf_cmd_set_relay   = 136


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
    },
    mrf_cmd_spi_data : {
        'name'  : "SPI_DATA",
        'param' : None,
        'resp'  : PktUint16
    },
    mrf_cmd_config_adc : {
        'name'  : "CONFIG_ADC",
        'param' : None,
        'resp'  : None
    },
    mrf_cmd_read_state : {
        'name'  : "READ_STATE",
        'param' : None,
        'resp'  : PktPt1000State
    },
    mrf_cmd_get_relay : {
        'name'  : "GET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },
    mrf_cmd_set_relay : {
        'name'  : "SET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },

}



class TestPt1000(DeviceTestCase):
    def host_test(self):
        self.devname = 'hostsim'
        self.num_ifs = 4
        self.num_buffs = 16
        self.timeout = 0.6
        self.dest =0x01
        self.app_cmds = DefaultAppCmds

    def setUp(self):
        DeviceTestCase.setUp(self)
        self.devname = 'usbrf'
        self.num_ifs = 2
        self.num_buffs = 8
        self.timeout = 0.6
        self.dest =0x02
        self.host= 0x01
        #self.app_cmds = Pt1000AppCmds
        self.app_cmds = copy(MrfSysCmds)
        self.app_cmds.update(Pt1000AppCmds)
        self.checkgit = False
        #self.host_test()

    def spi_debug(self):
        self.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.response(timeout=self.timeout)
        print "spi debug:\n"
        print dresp

    def default_values(self):
        ccode = mrf_cmd_spi_write
        paramstr = PktUint8_2()
        paramstr.value[0] = 0
        paramstr.value[1] = 1

        self.cmd(self.dest,ccode,dstruct=paramstr)
        
        print "set default val 01"

    def write_verify(self,addr,val,mask):
        ccode = mrf_cmd_spi_write
        paramstr = PktUint8_2()
        paramstr.value[0] = addr
        paramstr.value[1] = val

        self.cmd(self.dest,ccode,dstruct=paramstr)
        
        print "wrote spi addr %02x val %02x"%(addr,val)

        ccode = mrf_cmd_spi_read
        paramstr = PktUint8()
        paramstr.value = addr
        
        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        print "address %02x , read value %s"%(addr,repr(resp))
        
        #print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktUint8()),type(resp))
        print "address %02x , read value %02x"%(addr,resp.value)

        self.assertEqual(val,resp.value & mask)
        
    def configure_dev(self):
        rvs = { 0   : [7,0xff] , # pos input ain0, neg input ain7
                1   : [0,0xff],  # select ref 0
                2   : [(1 << 5) ,0xff],  # normal operation
                3   : [0,0xff] , # default
                0xa : [4,0x0f],  # 500uA ref
                0xb : [0x0f,0xff] # dac0 to ain0
        }
        print  "Configuring ADS1148 over SPI"
        ras = copy(rvs.keys())
        ras.sort()
        print "reg addresses %s"%repr(ras)
        for addr in ras:
            print "writing reg %02x val %02x vmask %02x"%(addr,rvs[addr][0],rvs[addr][1])
            self.write_verify(addr,rvs[addr][0],rvs[addr][1])
        
    def read_write_spi_test(self, addr = 0):
        self.default_values()
        print "**********************"
        print "* read_write_spi test addr = %d (dest 0x%02x)"%(addr,self.dest)
        print "**********************"
        
        ccode = mrf_cmd_spi_read
        paramstr = PktUint8()
        paramstr.value = addr

        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        #print "read addr %d : %s"%(addr,repr(resp))
        oval = resp.value
        print "address %d , read value %d"%(addr,oval)
        self.assertEqual(type(PktUint8()),type(resp))


        tval = 0xfe

        
        ccode = mrf_cmd_spi_write
        paramstr = PktUint8_2()
        paramstr.value[0] = addr
        paramstr.value[1] = tval

        print "paramstr = addr %d val %d "%(paramstr.value[0],paramstr.value[1])
        self.cmd(self.dest,ccode,dstruct=paramstr)
        
        print "wrote spi write val %02x"%tval

        ccode = mrf_cmd_spi_read
        paramstr = PktUint8()
        paramstr.value = addr
        
        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        print "address %d , read value %d"%(addr,resp.value)
        
        #print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktUint8()),type(resp))
        self.assertEqual(tval,resp.value)

        # restore orig value
        ccode = mrf_cmd_spi_write
        paramstr = PktUint8_2()
        paramstr.value[0] = addr
        paramstr.value[1] = oval

        print "paramstr = addr %d val %d "%(paramstr.value[0],paramstr.value[1])
        self.cmd(self.dest,ccode,dstruct=paramstr)
        
        print "wrote spi write"

        ccode = mrf_cmd_spi_read
        paramstr = PktUint8()
        paramstr.value = addr
        
        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktUint8()),type(resp))
        self.assertEqual(oval,resp.value)

        

    def if_status(self,i_f=0):
        paramstr = PktUint8()
        paramstr.value = i_f
        ccode = mrf_cmd_if_stats
        self.cmd(self.dest,ccode,dstruct=paramstr)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktIfStats()),type(resp))
        
        

    def host_app_test(self, addr = 0):
        print "**********************"
        print "* host_app test addr = %d (dest 0x%02x)"%(addr,self.dest)
        print "**********************"
        ccode = mrf_cmd_app_test
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "got resp:\n%s"%repr(resp)
        self.assertEqual(type(PktTimeDate()),type(resp))

    def skipped_test01a_read_config(self):
        
        paramstr = PktUint8()
        regvals = {}
        for addr in xrange(0xf):        
            paramstr.value = addr
            self.cmd(self.dest,mrf_cmd_spi_read,dstruct=paramstr)
            resp = self.response(timeout=self.timeout)            
            print "reg %x :  %02x"%(addr, resp.value)
            regvals[addr] = resp.value

    def test001_dev_id_tests(self):
        self.dev_info_test(self.dest)
        self.sys_info_test(self.dest)


    def test002_app_info_tests(self):
        self.app_info_test(self.dest)
        self.app_cmd_info_test(self.dest)

    
    def test01_core_tests(self):
        self.set_time_test(self.dest,self.host)
        #self.get_time_test(self.host)
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
    def config_cmd(self):
        print "Sending mrf_cmd_config_adc"
        ccode = mrf_cmd_config_adc
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)
        
    def skipped_test04_configure(self):
        self.config_cmd()
        #self.configure_dev()
        #self.read_adc()

    def read_adc(self):
        ccode = mrf_cmd_spi_data
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)        
        return resp.value
    
    def skipped_test05a_read_adc(self):
        ds = self.dev_status(self.dest)
        print "Initial device status:\n %s",repr(ds)
        for i in xrange(5):
            rv = self.read_adc()
            res = eval_rx(rv)
            temp = eval_temp(rv)
            print "ADC %d  res %.1f temp = %.2f"%(rv,res,temp)
            
    def skipped_test05b_read_continuous(self):
        while True:
            rv = self.read_adc()
            if rv == None:
                continue
            res = eval_rx(rv)
            temp = eval_temp(rv)
            print "ADC %d  res %.1f temp = %.2f"%(rv,res,temp)
            time.sleep(1)

    def test05c_read_state(self):
        print "**********************"
        print "* pt1000 read state test (dest 0x%02x)"%self.dest
        print "**********************"
       
        ccode = mrf_cmd_read_state
        self.cmd(self.dest,ccode)
        resp = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp)
        self.assertTrue(self.check_attrs(resp,PktPt1000State()))
        print "pt1000 read_state test PASSED"

    def skipped_test06_set_relay(self):
        print "**********************"
        print "* pt1000 set relay test (dest 0x%02x)"%self.dest
        print "**********************"
       
        data = PktRelayState()
        data.chan = 0
        data.val = 0
        ## get initial relay state for chan 0
        self.cmd(self.dest,mrf_cmd_get_relay,dstruct=data)
        resp = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp)
        self.assertTrue(self.check_attrs(resp,PktRelayState()))

        # toggle relay state
        if resp['val']:
           data.val = 0
        else:
           data.val = 1

        self.cmd(self.dest,mrf_cmd_set_relay,dstruct=data)
        resp2 = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp2)
        self.assertTrue(self.check_attrs(resp2,data,checkval=True))

        ## check pt1000state has updated
        self.cmd(self.dest,mrf_cmd_read_state)
        rst = self.response(timeout=self.timeout)
        print "resp %s"%repr(rst)
        self.assertTrue(self.check_attrs(rst,PktPt1000State()))
        
        # restore relay state
        self.cmd(self.dest,mrf_cmd_set_relay,dstruct=resp)
        resp2 = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp2)
        exp = PktRelayState()
        exp.dic_set(resp)
        self.assertTrue(self.check_attrs(resp2,exp,checkval=True))



    def skipped_test06_toggle_relay(self):
        print "**********************"
        print "* pt1000 toggle relay test (dest 0x%02x)"%self.dest
        print "**********************"
       
        data = PktRelayState()
        data.chan = 0
        data.val = 0
        ## get initial relay state for chan 0
        self.cmd(self.dest,mrf_cmd_get_relay,dstruct=data)
        resp = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp)
        self.assertTrue(self.check_attrs(resp,PktRelayState()))

        # toggle relay state
        if resp['val']:
           data.val = 0
        else:
           data.val = 1

        self.cmd(self.dest,mrf_cmd_set_relay,dstruct=data)
        resp2 = self.response(timeout=self.timeout)
        print "resp %s"%repr(resp2)
        self.assertTrue(self.check_attrs(resp2,data,checkval=True))

        ## check pt1000state has updated
        self.cmd(self.dest,mrf_cmd_read_state)
        rst = self.response(timeout=self.timeout)
        print "resp %s"%repr(rst)
        self.assertTrue(self.check_attrs(rst,PktPt1000State()))
        
        
        
           
        
    def skipped_test02a_spi_write_test(self):
        self.if_status()
        self.read_write_spi_test()
        self.if_status()
        #rparamstr = PktUint8()
        #wparamstr = PktUint8_2()
        
        
    def skipped_test03_device_read_tests(self):

        self.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.response(timeout=self.timeout)
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
            self.cmd(self.dest,mrf_cmd_spi_read,dstruct=paramstr)
            resp = self.response(timeout=self.timeout)            
            print "reg %x :  %02x"%(addr, resp.value)
            regvals[addr] = resp.value
            
        err_tot = 0
        errs = {}
        chks = 0
        for loop in xrange(1):
            print "check loop %d  checks %d errors %d"%(loop,chks,err_tot)
            for addr in xrange(0xf):
                chks = chks + 1
                paramstr.value = addr
                self.cmd(self.dest,mrf_cmd_spi_read,dstruct=paramstr)
                resp = self.response(timeout=self.timeout)
                if resp.value != regvals[addr]:
                    err_tot += 1
                    if not errs.has_key(addr):
                        errs[addr] = 0
                    errs[addr] = errs[addr] +  1
                    #print "addr %d errs %d"%(addr,errs[addr])
                    print "ERROR reg %02d expected %02x got %02x"%(addr,regvals[addr],resp.value)
                self.assertEqual(resp.value, regvals[addr])

                    
        print "loops %d err_tot %d out of %d device reads"%(loop,err_tot,chks)

        nks = errs.keys()
        nks.sort()
        print "keys %s"%repr(nks)
        for ky in nks:
            print "reg %d  errors %d"%(ky,errs[ky])
            

        self.cmd(self.dest,mrf_cmd_spi_debug)
        dresp = self.response(timeout=self.timeout)
        print "spi debug:\n"
        print dresp
        
        return

    def skipped_test02_burnin(self):
        for i in xrange(100):
            self.test01_device_tests()


if __name__ == "__main__":
    unittest.main()
