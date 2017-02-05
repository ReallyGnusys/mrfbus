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

        self.portnum = portnum

        self.ss = self._create_server_socket()
        self.log.info( "we have a socket %s"%repr(self.ss))
        self.conns = {}  # connections
        self.ep = select.epoll()

        self.ep.register(self.rfd)
        self.ep.register(self.sfd)
        self.ep.register(self.ss.fileno())
        
        try:
            while True:
                events = self.ep.poll(_EPOLL_BLOCK_DURATION_S)
                for fd, event_type in events:
                    self._handle_event(fd, event_type)
        finally:
            self.ep.unregister(self.ss.fileno())
            self.ep.unregister(self.rfd)
            self.ep.unregister(self.sfd)
            self.ep.close()
            self.ss.close()

    def _handle_event(self, fd, event_type):
        # Common, but we're not interested.
        flag_list = _get_flag_names(event_type)
        self.log.debug("got event on fd %d : %s", fd, flag_list)
        
        if (event_type & select.EPOLLOUT) == 0:
            self.log.debug("Received (%d): %s", 
                           fd, flag_list)
 
        # Activity on the master socket means a new connection.
        if fd == self.rfd:
            self.log.debug("Input on response pipe", event_type)
            if event_type & select.EPOLLIN:
                b = c.recv(1024)
                sys.stdout.write(b)
        elif fd == self.rfd:
            self.log.debug("Input on response pipe", event_type)
            if event_type & select.EPOLLIN:
                b = c.recv(1024)
                sys.stdout.write(b)

                
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
 
            # Child connection can read.
            if event_type & select.EPOLLHUP:
                self._error_exit("got EPOLLHUP")

            if event_type & select.EPOLLERR:
                self._error_exit("got EPOLLERR")
                
            if event_type & select.EPOLLIN:
                b = c.recv(1024)
                if len(b) == 0:
                    self.log.warn("got zero length on fd %d"%fd)
                    self.ep.unregister(fd)
                    del (self.conns[fd])
                    self.log.info("%d connections"%len(self.conns))
                sys.stdout.write(b)
 
        
    def _create_server_socket(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('0.0.0.0', self.portnum))
        s.listen(_MAX_CONNECTION_BACKLOG)
        s.setblocking(0)
        self.log.info("created server socket")
        return s

if __name__ == "__main__":
    ml =  mrfland()
