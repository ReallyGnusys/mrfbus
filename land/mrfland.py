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
import time
import sys
import select
import signal
import logging
import socket
from mrf_structs import *


_DEFAULT_LOG_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'

_MAX_CONNECTION_BACKLOG = 1
_EPOLL_BLOCK_DURATION_S = 1
 
 
 
_EVENT_LOOKUP = {
    select.POLLIN: 'POLLIN',
    select.EPOLLIN: 'EPOLLIN',
    select.EPOLLHUP: 'EPOLLHUP',
    select.EPOLLERR: 'EPOLLERR',
    select.EPOLLOUT: 'EPOLLOUT',
    select.POLLPRI: 'POLLPRI',
    select.POLLOUT: 'POLLOUT',
    select.POLLERR: 'POLLERR',
    select.POLLHUP: 'POLLHUP',
    select.POLLNVAL: 'POLLNVAL',
    
}
def _get_flag_names(flags):
    names = []
    for bit, name in _EVENT_LOOKUP.items():
        if flags & bit:
            names.append(name)
            flags -= bit
 
            if flags == 0:
                break
 
    return names
 

def buffToObj(resp,app_cmds = {}):
    """
    attempt to convert pipe ascii data to a MrfStruct
    """
    
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


class mrfland(object):
    """
        MRF LAN Daemon
    """
    def _error_exit(self,msg):
        print "CRITICAL ERROR: %s"%msg
        sys.exit(-1)

    def start_logging(self):
        self.log = logging.getLogger(__name__)
        self.log.setLevel(logging.DEBUG)
        ch = logging.StreamHandler()
        formatter = logging.Formatter(_DEFAULT_LOG_FORMAT)
        ch.setFormatter(formatter)
        self.log.addHandler(ch)
        self.log.info("log started")
        
    def __init__ (self, portnum=7777,stub_out_resp_path='/tmp/mrf_bus/0-app-out',stub_out_str_path='/tmp/mrf_bus/0-app-str'):
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            self.log.warn( "CTRL-C pressed , quitting")
            sys.exit(0)
        self.start_logging()
        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)
        self.stub_out_resp_path = stub_out_resp_path
        self.rfd =  os.open(self.stub_out_resp_path, os.O_RDONLY | os.O_NONBLOCK)
        if self.rfd == -1:
            self._error_exit("could not open response fifo %s"%self.stub_out_resp_path)
    
        self.stub_out_str_path = stub_out_str_path
        self.sfd =  os.open(self.stub_out_str_path, os.O_RDONLY | os.O_NONBLOCK)
        if self.sfd == -1:
            self._error_exit("could not open structure fifo %s"%self.stub_out_str_path)


        self.log.info( "opened pipes")
        self.app_fifo = open("/tmp/mrf_bus/0-app-in","w")
        self.log.info( "app_fifo opened")

        
        self.portnum = portnum

        self.ss = self._create_server_socket()
        self.log.info( "we have a socket %s"%repr(self.ss))
        self.conns = {}  # connections
        self.ep = select.epoll()

        self.ep.register(self.rfd)
        self.ep.register(self.sfd)
        self.ep.register(self.ss.fileno())
        self.init_cmds()   # run any initial commands

        try:
            while True:
                self.poll()
        finally:
            self.ep.unregister(self.ss.fileno())
            self.ep.unregister(self.rfd)
            self.ep.unregister(self.sfd)
            self.ep.close()
            self.ss.close()

    def init_cmds(self):
        self.init_cmd(1,mrf_cmd_device_info)
        self.init_cmd(1,mrf_cmd_device_status)
        self.init_cmd(1,mrf_cmd_app_info)

    def poll(self):
        events = self.ep.poll(_EPOLL_BLOCK_DURATION_S)
        for fd, event_type in events:
            self._handle_event(fd, event_type)
    
    def _handle_event(self, fd, event_type):
        # Common, but we're not interested.
        flag_list = _get_flag_names(event_type)
        self.log.debug("got event on fd %d : %s", fd, flag_list)
        
        if (event_type & select.EPOLLOUT) == 0:
            self.log.debug("Received (%d): %s", 
                           fd, flag_list)
 
        # Activity on the master socket means a new connection.
        if fd == self.rfd:
            self.log.debug("Input on response pipe (%d)"%event_type)
            if event_type & select.EPOLLIN:
                resp = os.read(self.rfd, MRFBUFFLEN)
                self.log.debug("have response len %d"%len(resp))
                rstr = buffToObj(resp)

                sys.stdout.write(repr(rstr))
        elif fd == self.sfd:
            self.log.debug("Input on data pipe (%d)"%event_type)
            if event_type & select.EPOLLIN:
                resp = os.read(self.rfd, MRFBUFFLEN)
                #b = fd.recv(1024)
                sys.stdout.write(repr(resp))
                
        elif fd == self.ss.fileno():
            self.log.debug("Received connection: (%d)", event_type)
 
            c, address = self.ss.accept()
            c.setblocking(0)
            child_fd = c.fileno()
 
            # Start watching the new connection.
            self.ep.register(child_fd, select.EPOLLIN or select.EPOLLHUP)
            self.conns[child_fd] = c
            self.log.info("%d connections"%len(self.conns))

        else:
            c = self.conns[fd]
            if event_type & select.EPOLLIN:
                b = c.recv(1024)
                if len(b) == 0:
                    self.log.warn("got zero length on fd %d"%fd)
                    self.ep.unregister(fd)
                    del (self.conns[fd])
                    self.log.info("%d connections"%len(self.conns))
                else:
                    sys.stdout.write(b)
 
        
    def _create_server_socket(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('0.0.0.0', self.portnum))
        s.listen(_MAX_CONNECTION_BACKLOG)
        s.setblocking(0)
        self.log.info("created server socket")
        return s
    
    def init_cmd(self,dest,cmd_code,dstruct=None):
        self.cmd(dest,cmd_code,dstruct)
        self.poll()

    def cmd(self,dest,cmd_code,dstruct=None):
        self.log.info("cmd : dest %d cmd_code %s"%(dest,repr(cmd_code)))
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

    
if __name__ == "__main__":
    ml =  mrfland()

