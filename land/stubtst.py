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

class StubIf(object):
    def __init__(self):
        self.app_fifo = open("/tmp/mrf_bus/0-app-in","w")
        print "StubIf.__init__  opened app_fifo "
        self.app_out_fifo = open("/tmp/mrf_bus/0-app-out", "r")
        print "exit constructor"
        """
    def read(self):
        buff = bytearray(256)
        try:
            buffer = os.read(self.app_ouf_fifo,256)
        except:
        """
    def cmd(self,dest,cmd_code,dstruct=None):

        
        if dest > 255:
            print "dest > 255"
            return -1

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
    
                   

if __name__ == "__main__":
    si = StubIf()
    rv = si.cmd(0x2f,3)
    if rv == -1:
        print "error -1"

    
