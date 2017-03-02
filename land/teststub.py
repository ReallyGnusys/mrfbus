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
import ctypes
import unittest

import signal

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

def buffToObj(resp,app_cmds):
    hdr = PktHeader()
    print "buffToObj len is %d"%len(resp)
    if len(resp) >= len(hdr):
        hdr_data = bytes(resp)[0:len(hdr)]
        hdr.load(bytes(resp)[0:len(hdr)])
        print "hdr is %s"%repr(hdr)
        if hdr.type in MrfSysCmds.keys():
            if MrfSysCmds[hdr.type]['param'] and ( MrfSysCmds[hdr.type]['name'] == 'USR_RESP' or MrfSysCmds[hdr.type]['name'] == 'USR_STRUCT' ):  # testing aye
                param = MrfSysCmds[hdr.type]['param']()
                param_data = bytes(resp)[len(hdr):len(hdr)+len(param)]
                param.load(param_data)
                if param.type in MrfSysCmds.keys() and MrfSysCmds[param.type]['resp']:
                    respobj = MrfSysCmds[param.type]['resp']()
                    respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
                    respobj.load(respdat)
                    return respobj
                elif param.type in app_cmds.keys() and app_cmds[param.type]['resp']:
                    respobj = app_cmds[param.type]['resp']()
                    respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
                    respobj.load(respdat)
                    return respobj
                else:
                    print "got param type %d "%param.type
            else:
                print "got hdr.type %d"%hdr.type
    return None

                
class struct_thread(threading.Thread):
    """
        A thread class to read stub structure output
    """

    def __init__ (self, stub_out_fifo_path,app_cmds):        
        self.stub_out_fifo_path = stub_out_fifo_path
        print "struct_thread.init: stub_out_fifo_path = %s"%self.stub_out_fifo_path
        self.app_out_fifo = os.open(self.stub_out_fifo_path, os.O_RDONLY | os.O_NONBLOCK)
        print "struct_thread app_out_fifo type is %s"%type(self.app_out_fifo)
        self._stop = False
        self.app_cmds = app_cmds
        threading.Thread.__init__ (self)
   
    def stop(self):
        self._stop = True
    def run(self):
        while True:
            try:
                resp = os.read(self.app_out_fifo, MRFBUFFLEN)
                print "struct_thread.read got something, length is %d"%len(resp)
                rstr = buffToObj(resp)
                print "rstr %s"%repr(rstr)
                #self.q.put(resp)
            except:
                if self._stop:
                    return

    

class StubIf(object):
    def __init__(self):
        self.q = Queue.Queue()
        self.stub_out_pipe_path = "/tmp/mrf_bus/0-app-out"
        self.stub_struct_pipe_path = "/tmp/mrf_bus/0-app-str"
        self.app_fifo_pipe_path = "/tmp/mrf_bus/0-app-in"
        self.rx_thread = rx_thread(self.stub_out_pipe_path, self.q)
        self.rx_thread.start()
        print "StubIf trying to open app_fifo %s"%self.app_fifo_pipe_path
        self.app_fifo = open(self.app_fifo_pipe_path,"w")
        print "StubIf.__init__  opened app_fifo "
        print repr(self.app_fifo)
        outfname = "/tmp/mrf_bus/0-app-out"
        self.app_cmds = {}  # need to override in some ugly way to allow app command testing for now

        self.struct_thread = struct_thread(self.stub_struct_pipe_path,self.app_cmds)
        self.struct_thread.start()

        
    def bin2hex(self,buff):
        return ''.join('{:02x}'.format(x) for x in bytearray(buff))

    def cmd(self,dest,cmd_code,dstruct=None):

        if dest > 255:
            print "dest > 255"
            return -1


        if cmd_code in MrfSysCmds.keys():
            paramtype = MrfSysCmds[cmd_code]['param']

        elif cmd_code in self.app_cmds.keys():
            paramtype = self.app_cmds[cmd_code]['param']
            #print "got app command - cmd_code %d  paramtype %s :\n   %s"%(cmd_code,
            #                                                              repr(paramtype),
            #                                                              repr(dstruct),
            #)
        else:
            print "unrecognised cmd_code (01xs) %d"%cmd_code
            return -1
            
         
        if type(dstruct) == type(None) and type(paramtype) != type(None):
            print "No param sent , expected %s"%type(paramtype)
            return -1
        
        if type(paramtype) == type(None) and type(dstruct) != type(None):
            print "Param sent ( type %s ) but None expected"%type(dstruct)
            return -1

        if type(dstruct) != type(None) and type(dstruct) != type(paramtype()):
            print "Param sent ( type %s ) but  %s expected"%(type(dstruct),type(paramtype()))
            return -1

        while not self.q.empty():
            self.q.get()

        mlen = 4
        if dstruct:
            mlen += len(dstruct)
        if mlen > 64:
            print "mlen = %d"%mlen
            return -1
        msg = bytearray(mlen)
        msg[0] = mlen
        msg[1] = dest
        msg[2] = cmd_code
        
        n = 3
        if dstruct:
            dbytes = dstruct.dump()
            for b in dbytes:
                msg[ n ] =  b
                n += 1

        csum = 0
        for i in xrange(mlen -1):
            csum += int(msg[i])
        csum = csum % 256
        msg[mlen - 1] = csum
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
        return buffToObj(resp,self.app_cmds)
                        
    def quit(self):
        self.rx_thread.join(0.1)
        self.rx_thread.stop()
        self.struct_thread.join(0.1)
        self.struct_thread.stop()

    def cmd_test(self,dest,cmd_code,expected,dstruct=None):
        """
        simple test interface to send cmds to destinations, supplying expected values for checking
        """
        rv = self.cmd(dest,cmd_code)
        if (rv != 0):
            return rv
        rsp = self.response()

        print "received:\n %s"%repr(rsp)
        print "expected:\n %s"%repr(expected)        
        if rsp != expected:
            print "ERROR "
            print "cmd_test failed"
            return -1
        else:
            print "cmd_test passed"
            return 0


class StubTestCase(unittest.TestCase):
    def setUp(self):
        print "StubTestCase setUp : entry"
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            print "CTRL-C pressed , quitting"
            self.tearDown()            
        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)
        #unittest.installHandler()
        self.timeout = 0.4
        self.stub = StubIf()

    def tearDown(self):
        self.stub.quit()
