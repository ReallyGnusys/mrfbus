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
from mrfland_app import MrflandApp

class MrflandAppHeating(MrflandApp):


    def __init__(self):
        self.pt1000 = { 2 }  #set of addresses
    def cmd_boris(self,data):
        print "cmd boris here, data was %s"%repr(data)

    def cmd_nancy(self,data):
        print "cmd nancy here, data was %s"%repr(data)
    

    def fyi(self,hdr,resp):
        if hdr.usrc in self.pt1000 :
            print "heating app fyi  says yes"
            
        else:
            print "heating app fyi  says no"
        
