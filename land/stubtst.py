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

from mrf_structs import PktHeader


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
        hdr = PktHeader()
        while True:
            #print "rx_thread.run , calling read"
            
            try:
                resp = os.read(self.app_out_fifo, 8)
                print "rx_thread.read got something, length is %d"%len(resp)
            
                if len(resp) == len(hdr):
                    hdr.load(bytes(resp))
                    print "got hdr %s"%str(hdr)
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

 

    def cmd(self,dest,cmd_code,dstruct=None):

        
        if dest > 255:
            print "dest > 255"
            return -1

        print "cmd .. clearing q .."
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
        print "wrote msg %s"%repr(msg)
        
        print "going to wait for q now..."

        while self.q.empty():
            time.sleep(0.1)
        resp = self.q.get()
        print "got response..%s"%str(resp)
        
        
        print "exit cmd"
        self.rx_thread.join(0.1)
        print "join might have timeout out"
        self.rx_thread.stop()

if __name__ == "__main__":
    si = StubIf()
    rv = si.cmd(0x2f,3)
    if rv == -1:
        print "error -1"
    

