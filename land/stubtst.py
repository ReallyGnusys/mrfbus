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
from mrf_structs import PktHeader,PktDeviceInfo, MrfSysCmds
MRFBUFFLEN = 128
import ctypes

class rx_thread(threading.Thread):
    """
        A thread class to read stub output
    """

    def __init__ (self, stub_out_fifo_path,q):
        self.stub_out_fifo_path = stub_out_fifo_path
        self.q = q
        print "rx_thread.init: stub_out_fifo_path = %s"%self.stub_out_fifo_path
        self.app_out_fifo = os.open(self.stub_out_fifo_path, os.O_RDONLY | os.O_NONBLOCK)
        print "rx_thread app_out_fifo type is %s"%type(self.app_out_fifo)
        self._stop = False
        threading.Thread.__init__ (self)
   
    def stop(self):
        self._stop = True
    def run(self):
        while True:
            try:
                resp = os.read(self.app_out_fifo, MRFBUFFLEN)
                #print "rx_thread.read got something, length is %d"%len(resp)
                self.q.put(resp)
            except:
                if self._stop:
                    return


class StubIf(object):
    def __init__(self):
        self.q = Queue.Queue()
        self.stub_out_pipe_path = "/tmp/mrf_bus/0-app-out"
        
        self.rx_thread = rx_thread(self.stub_out_pipe_path, self.q)
        self.rx_thread.start()
        self.app_fifo = open("/tmp/mrf_bus/0-app-in","w")
        print "StubIf.__init__  opened app_fifo "
        outfname = "/tmp/mrf_bus/0-app-out"
 
    def bin2hex(self,buff):
        return ''.join('{:02x}'.format(x) for x in bytearray(buff))

    def cmd(self,dest,cmd_code,dstruct=None):

        if dest > 255:
            print "dest > 255"
            return -1

        while not self.q.empty():
            self.q.get()

        mlen = 4
        if False and dstruct:
            mlen += len(dstruct)
        if mlen > 64:
            print "mlen = %d"%mlen
            return -1
        msg = bytearray(mlen)
        msg[0] = mlen
        msg[1] = dest
        msg[2] = cmd_code
    
        csum = 0
        for i in xrange(mlen):
            csum += int(msg[i])
        csum = csum % 256
        msg[3] = csum
        self.app_fifo.write(msg)
        self.app_fifo.flush()
        return 0
    def response(self,timeout = 1.0):
        elapsed = 0.0
        tinc = 0.1
        while self.q.empty():
            time.sleep(tinc)
            elapsed += tinc
            if elapsed >= timeout:
                return None
        resp = self.q.get()

        hdr = PktHeader()
        #buffhex =  ''.join('{:02x}'.format(x) for x in bytearray(resp))
        #print "got response..%s"%self.bin2hex(resp)
        if len(resp) >= len(hdr):
            hdr_data = bytes(resp)[0:len(hdr)]
            hdr.load(bytes(resp)[0:len(hdr)])
            if hdr.type in MrfSysCmds.keys():
                if MrfSysCmds[hdr.type]['param'] and MrfSysCmds[hdr.type]['name'] == 'USR_RESP':  # testing aye
                    param = MrfSysCmds[hdr.type]['param']()
                    param_data = bytes(resp)[len(hdr):len(hdr)+len(param)]
                    param.load(param_data)
                    if param.type in MrfSysCmds.keys() and MrfSysCmds[param.type]['resp']:
                        respobj = MrfSysCmds[param.type]['resp']()
                        respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
                        respobj.load(respdat)
                        return respobj
            return None
                        
    def quit(self):
        self.rx_thread.join(0.1)
        print "join might have timeout out"
        self.rx_thread.stop()

    def cmd_test(self,dest,cmd_code,expected,dstruct=None):
        rv = self.cmd(dest,cmd_code)
        if (rv != 0):
            return rv
        rsp = self.response()

        print "received %s"%repr(rsp)
        print "expected %s"%repr(expected)        
        if rsp != expected:
            print "ERROR "
            print "cmd_test failed"
            return -1
        else:
            print "cmd_test passed"
            return 0
    def check_device_infos(self,dests):
        ccode = 3
        exp = PktDeviceInfo()
        exp.netid = 0x25
        exp.num_buffs = 0x10
        exp.num_ifs = 0x4
        #setattr(exp,'dev_name','hostsim')
        exp.dev_name = (ctypes.c_uint8*10)(*(bytearray('hostsim')))

        for dest in [ 0x01, 0x2,0x20, 0x2f]:
            exp.mrfid = dest
            rv = self.cmd_test(dest,ccode,exp,dstruct=None)
            if rv != 0:
                print "error rv was %d"%rv
                return rv
        return 0

    def discover_devices(self):
        devs = []
        cmd_code = 3
        for dest in range(1,0x30):
            rv = self.cmd(dest,cmd_code)
            rsp = self.response(0.2)
            if type(rsp) == type(PktDeviceInfo()):
                devs.append(rsp)
                print "found one at dest %x"%dest
            else:
                print "not found at dest %x"%dest
        return devs

if __name__ == "__main__":
    si = StubIf()

    try:
        if False:
            rv = si.check_device_infos( [ 0x01, 0x2,0x20, 0x2f] )
        else:
            devs = si.discover_devices()
            print "got %d devices"%len(devs)
            for dev in devs:
                print dev

            if len(devs) == 4:
                print "All devices detected"
                rv = 0
        if rv != 0:
            print "tests failed"
            si.quit()

            sys.exit(-1)

        else:
            print "All tests passed"
            
        
        """
        rv = si.cmd(0x2f,3)
        rsp = si.response()
        print "got rsp %s"%repr(rsp)
        if rv == -1:
            print "error -1"
        """

    except Exception as inst:
        print "exception...doh!"
        print type(inst)     # the exception instance
        print inst.args      # arguments stored in .args
        print inst           # __str__ allows args to be printed directly

    si.quit()

    sys.exit(0)
