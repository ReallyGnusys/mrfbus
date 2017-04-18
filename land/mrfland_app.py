#!/usr/bin/env python
'''  Copyright (c) 2012-17 Gnusys Ltd

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

"""
 base class for mrfland apps  
 try to keep app specific stuff out of main mrfland
 attempt to present a simple command set, appropriate to each app, to users
 support subscription mechanism for automatic updates
"""


class MrflandApp(object):
    def __init__(self,tag, log , cmd_callback):
        self.tag = tag
        self.log = log
        self.cmd_callback = cmd_callback
        self.managed_addrs = {}

    def i_manage(self,addr):
        return addr in self.managed_addrs

    def cmd_set(self,addr):
        if self.i_manage(addr):
            return self.managed_addrs[addr]
        else:
            return None

    
    def setlog(self,log):
        self.log = log

    def cmd(self,cmd, data=None):
        fn = 'cmd_'+cmd
        if hasattr(self, fn):
            self.log.info( "OK you can go")
            return getattr(self,fn)(data)
        else:
            self.log.info("you're not coming in here")

