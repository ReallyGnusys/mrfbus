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

from mrf_structs import PktHeader, MrfSysCmds
MRFBUFFLEN = 128

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
                print "rx_thread.read got something, length is %d"%len(resp)
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

        hdr = PktHeader()
        print "len PktHeader is %d"%len(hdr)
        #buffhex =  ''.join('{:02x}'.format(x) for x in bytearray(resp))
        print "got response..%s"%self.bin2hex(resp)
        if len(resp) >= len(hdr):
            hdr_data = bytes(resp)[0:len(hdr)]
            print "length hdr_data is %d"%len(hdr_data)
            print "hdr_data is %s"%self.bin2hex(hdr_data)
            hdr.load(bytes(resp)[0:len(hdr)])
            print "got hdr %s"%str(hdr)
            if hdr.type in MrfSysCmds.keys():
                print "yes aye... we have decoded a command .. %s"%MrfSysCmds[hdr.type]['name']
                if MrfSysCmds[hdr.type]['param'] and MrfSysCmds[hdr.type]['name'] == 'USR_RESP':  # testing aye
                    param = MrfSysCmds[hdr.type]['param']()
                    print "got param... %s len %d"%(str(MrfSysCmds[hdr.type]['param']),len(param))
                    param_data = bytes(resp)[len(hdr):len(hdr)+len(param)]
                    print "param_data is %s"%self.bin2hex(param_data)
                    param.load(param_data)
                    print param
                    print "type is %d"%param.type
                    if param.type in MrfSysCmds.keys() and MrfSysCmds[param.type]['resp']:
                        print "got resp type %s"%MrfSysCmds[param.type]['resp']
                        respobj = MrfSysCmds[param.type]['resp']()
                        respobj.load(bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj) ])
                        print respobj
                        devname =  "".join(chr(i) for i in respobj.dev_name)

                        print "Device name is %s"%devname
                        print "total is %s"%bytearray(resp)
                        
        print "exit cmd"
        self.rx_thread.join(0.1)
        print "join might have timeout out"
        self.rx_thread.stop()

if __name__ == "__main__":
    si = StubIf()
    rv = si.cmd(0x2f,3)
    if rv == -1:
        print "error -1"
    

