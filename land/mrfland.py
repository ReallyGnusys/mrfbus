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
import Queue
import json
import traceback
import linuxfd
from mrfland_state import MrflandState

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
    #print "buffToObj len is %d"%len(resp)
    if len(resp) >= len(hdr):
        hdr_data = bytes(resp)[0:len(hdr)]
        hdr.load(bytes(resp)[0:len(hdr)])
        #print "hdr is:\n%s\n"%repr(hdr)
        if hdr.type in MrfSysCmds.keys():
            if hdr.type == mrf_cmd_usr_resp or hdr.type == mrf_cmd_usr_struct:
                param = MrfSysCmds[hdr.type]['param']()
                param_data = bytes(resp)[len(hdr):len(hdr)+len(param)]
                param.load(param_data)
                #print "resp should be %s"%repr(param)
                if param.type in MrfSysCmds.keys() and MrfSysCmds[param.type]['resp']:
                    respobj = MrfSysCmds[param.type]['resp']()
                    #print "respopb is %s"%type(respobj)
                    respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
                    respobj.load(respdat)
                    #print "repr %s"%repr(respobj)
                    return hdr, respobj
                elif param.type in app_cmds.keys() and app_cmds[param.type]['resp']:
                    respobj = app_cmds[param.type]['resp']()
                    respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
                    respobj.load(respdat)
                    return hdr, respobj
                else:
                    print "got param type %d "%param.type
            else:
                print "got hdr.type %d"%hdr.type
    return None,None


class Mrfland(object):
    """
        MRF LAN Daemon
    """
    def _error_exit(self,msg):
        print "CRITICAL ERROR: %s"%msg
        sys.exit(-1)

    def start_logging(self):
        self.log = logging.getLogger(__name__)
        self.log.setLevel(logging.INFO)
        ch = logging.StreamHandler()
        formatter = logging.Formatter(_DEFAULT_LOG_FORMAT)
        ch.setFormatter(formatter)
        self.log.addHandler(ch)
        self.log.info("log started")

    def __init__ (self, portnum=7777,stub_out_resp_path='/tmp/mrf_bus/0-app-out',stub_out_str_path='/tmp/mrf_bus/0-app-str',apps = {}):
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            self.log.warn( "CTRL-C pressed , quitting")
            sys.exit(0)
        self.hostaddr = 1
        self.start_logging()
        self.log.info("apps are %s"%repr(apps))
        papps = {}
        for appn in apps.keys():
            papps[appn] = apps[appn]()
            papps[appn].setlog(self.log)
        self.apps = papps
        self.log.info("apps are %s"%repr(self.apps))
        self.q = Queue.Queue()
        self.active_cmd = None
        self.active_timer = 0
        self._comm_active = False  # flag indication bus communications active
        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)
        #signal.signal(signal.SIGALRM, itimer_handler)
        #signal.setitimer(signal.ITIMER_REAL,2,2)   # keep overseer process running every 2 seconds

        self.state = MrflandState(self)
        self.stub_out_resp_path = stub_out_resp_path
        self.stub_out_str_path = stub_out_str_path

        self.portnum = portnum

        self.ss = self._create_server_socket()
        self.log.info( "we have a socket %s"%repr(self.ss))
        self.conns = {}  # connections
        self.ep = select.epoll()

        self.ep.register(self.ss.fileno())

        try:
            self.tfd = linuxfd.timerfd()
            self.tfd.settime(value=2, interval = 2)
            self.log.info("created timerfd %d"%self.tfd.fileno())
            self.ep.register(self.tfd.fileno())
        except:
            self.log.error("timerfd exception")


        self.start_network()

        

        # probe for host device to test initial network state
        self.init_cmds()   # run any initial commands

        
        try:
            while True:
                self.poll()
        finally:
            self.log.warn("about to quit... exception")
            self.ep.unregister(self.ss.fileno())
            self.ep.unregister(self.tfd.fileno())
            self.ep.unregister(self.rfd)
            self.ep.unregister(self.sfd)
            self.ep.close()
            self.ss.close()

    def start_network(self):
        self.rfd =  os.open(self.stub_out_resp_path, os.O_RDONLY | os.O_NONBLOCK)
        if self.rfd == -1:
            self._error_exit("could not open response fifo %s"%self.stub_out_resp_path)
        else:
            self.ep.register(self.rfd)

        self.sfd =  os.open(self.stub_out_str_path, os.O_RDONLY | os.O_NONBLOCK)
        if self.sfd == -1:
            self._error_exit("could not open structure fifo %s"%self.stub_out_str_path)
        else:
            self.ep.register(self.sfd)


        self.log.info( "opened pipes, trying to open pipe to stub")
        self.app_fifo = open("/tmp/mrf_bus/0-app-in","w+")
        self.log.info( "app_fifo opened")
        
            
    def init_cmds(self):
        self.init_cmd(1,mrf_cmd_device_info)
        self.init_cmd(1,mrf_cmd_device_status)
        self.init_cmd(1,mrf_cmd_app_info)
        self.init_cmd(1,mrf_cmd_get_time)

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
 
        if fd == self.rfd:
            self.log.debug("Input on response pipe (%d)"%event_type)
            if event_type & select.EPOLLIN:
                resp = os.read(self.rfd, MRFBUFFLEN)
                self.log.debug("have response len %d"%len(resp))
                hdr, rstr = buffToObj(resp)
                self.log.info("received object %s response from  %d : %s"%(type(rstr),hdr.usrc,repr(rstr)))
                self.handle_response(hdr,rstr)
                #self.log.info(repr(rstr))
                #self.log.info("end obj")
                #sys.stdout.write(repr(rstr))
            elif event_type & select.EPOLLHUP:
                self.log.info("response connection has hung up")
                self.network_down()

                
        elif fd == self.sfd:
            self.log.info("Input on data pipe (%d)"%event_type)
            if event_type & select.EPOLLIN:
                resp = os.read(self.rfd, MRFBUFFLEN)
                #b = fd.recv(1024)
                sys.stdout.write(repr(resp))
            elif event_type & select.EPOLLHUP:
                self.log.info("data connection has hung up")
                self.network_down()

        elif fd == self.tfd.fileno():
            self.log.debug("timerfd: (%d)", event_type)
            self.tfd.read()
            self.overseer()
        

        # Activity on the server socket means a new client connection.                
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
                    self.log.info("trying to decode *%s*  len %d"%(b,len(b)))
                    b = b.rstrip()
                    self.log.info("trying to decode *%s*  len %d type %s"%(b,len(b),type(b)))
                    try:
                        jcmd = json.loads(b)
                        self.log.info("got json cmd %s"%repr(jcmd))
                        c.send("got it , thanks\n")
                    except:
                        
                        self.log.info("unintelligible input *%s*"%b)
                        print "Exception processing input:"
                        print '-'*60
                        traceback.print_exc(file=sys.stdout)
                        print '-'*60
                        c.send("no idea what you're on about\n")
                        
                    sys.stdout.write(b)                    
 
    def network_down(self):        
        self.log.warn("network_down entry")
        self.ep.unregister(self.rfd)
        self.ep.unregister(self.sfd)
        os.close(self.rfd)
        os.close(self.sfd)
        self.app_fifo.close()
        self.log.warn("closed all fifos..attempting restart")
        self.start_network()
        self.log.warn("fifos opened")
        
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
        
    def _activate(self):  # ramp up tick while responses or txqueue outstanding
        if not self._comm_active:
            #self.log.warn("activating!")
            self.active_timer = 0
            self._comm_active = True
            self.tfd.settime(value=0.001, interval = 0.01)

    def _deactivate(self):  # relax tick while all quiet
        self._comm_active = False
        self.tfd.settime(value=2, interval = 2)
        #self.log.warn("deactivating .. timer was %d",self.active_timer)
            
    def handle_response(self,hdr,rstr):
        self.state.fyi(hdr,rstr)  # state sees everything
        """ check response is for active_cmd"""
        if self.active_cmd and hdr.usrc == self.active_cmd[1]:  # FIXME - make queue items a bit nicer
            self.active_cmd = None

    def cmd(self, dest,cmd_code,dstruct=None):  # only for this code to call
        self.queue_cmd(0 , dest,cmd_code,dstruct)   # tag is zero for this process -  responses are never forwarded

    def _next_cmd(self,cmdarr):
        (tag, dest,cmd_code,dstruct) = cmdarr
        self.resp_timer = 0
        self.active_cmd = cmdarr            
        self._run_cmd(dest,cmd_code,dstruct)
        
    def queue_cmd(self,tag, dest,cmd_code,dstruct=None):
        if self.active_cmd:            
            self.q.put((tag, dest,cmd_code,dstruct))
        else:
            self._next_cmd((tag, dest,cmd_code,dstruct))
            self._activate()
            
    def _run_cmd(self,dest,cmd_code,dstruct = None):
        self.log.debug("cmd : dest %d cmd_code %s"%(dest,repr(cmd_code)))
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
    def send_ndr(self):
        return
    def overseer(self):
        """ overseer process checks things over on regular timerfd """
        #self.log.debug("overseer entry")
        if self._comm_active:
            self.active_timer += 1
            if self.active_cmd:
                self.resp_timer += 1
                if self.resp_timer > 2:  # give up
                    self.log.warn("timed out waiting for response for %s"%(repr(self.active_cmd)))
                    if self.active_cmd[0] :  # send ndr if response destined to socket
                        self.send_ndr()
                    self.active_cmd = None
            if not self.active_cmd:
                if self.q.empty():  # deactivate
                    self._deactivate()
                else:
                    self._next_cmd(self.q.get())
                    
                    
        self.state.task()

if __name__ == "__main__":
    ml =  Mrfland()

