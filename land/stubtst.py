#!/usr/bin/env python




class StubIf(object):
    def __init__(self):
        self.app_fifo = open("/tmp/mrf_bus/0-app","w")
        print "StubIf.__init__  opened app_fifo "


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
    print "hello from main"

    si = StubIf()
    #rv = si.cmd(1,3)
    rv = si.cmd(2,3)
    if rv == -1:
        print "error -1"

    
