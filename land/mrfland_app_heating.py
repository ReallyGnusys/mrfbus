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

from pt1000test import *

from datetime import datetime

Pt1000MaxChanns = 7


class Pt1000TempSensor(object):
    def __init__(self,address,channel):
        self.label = ""
        self.address = address        
        self.channel = channel
        self.milliohms = 0
        self.temperature = -273.16
    def res_to_temp(self,milliohms):
        R = milliohms/1000.0
        A=3.9083e-3
        B=-5.775e-7
        R0 = 1000.0
        R=R/R0
        T=0.0-A
        T = T +  sqrt((A*A) - 4.0 * B * (1.0 - R))
        T = T/ (2.0 * B)
        return T
    def __repr__(self):
        return "Channel %d milliohms %7d %7.3f C  label %s"%(self.channel,self.milliohms,self.temperature, self.label)
    
    def new_reading(self,milliohms):
        self.milliohms = milliohms
        self.temperature = self.res_to_temp(self.milliohms)
class Pt1000State(object):
    def __init__(self,address,log):
        self.address = address
        self.log = log
        self.temps = []
        self.last_reading = None
        for i in xrange(Pt1000MaxChanns):
            self.temps.append(Pt1000TempSensor(self.address,i))

            
    def __repr__(self):
        s = "%s address %d last reading %s\n"%(self.__class__.__name__,self.address,repr(self.last_reading))
        for chan in xrange(Pt1000MaxChanns):
            s += " %d) %s\n"%(chan,repr(self.temps[chan]))
        return s
    def new_state(self,hdr,state):
        if type(state) != type(PktPt1000State()):
            self.log.warn( "PT1000 state wrong type!!")
            return -1
        if hdr.usrc != self.address:   # just a spot sanity check..
            self.log.warn("PT1000 state wrong address!!")
            return -1

        now = datetime.now()

        if not ( self.last_reading == None or now > self.last_reading):
            self.log.warn( "PT1000 state - time confusion!!!")
            return -1
        
        for ch in xrange(Pt1000MaxChanns):
            self.temps[ch].new_reading(state.milliohms[ch])

        self.last_reading = now
        self.log.info("Pt1000State updated - now")
        self.log.info(repr(self))
        return 0
    
class MrflandAppHeating(MrflandApp):

    def __init__(self , log=None):
        self.log = log
        self._pt1000_addrs = { 2 }  #set of addresses

        self.pt1000state = {}

        for add in self._pt1000_addrs:
            self.pt1000state[add] = Pt1000State(add,self.log)
            
    def cmd_boris(self,data):
        self.log.info( "cmd boris here, data was %s"%repr(data))

    def cmd_nancy(self,data):
        self.log.info( "cmd nancy here, data was %s"%repr(data))

    def pt1000msg(self,hdr,rsp,robj,rdata):
        #if type(robj) != type(PktPt1000State()):
        if not robj:
            self.log.debug( "no robj but have rsp %s"%repr(rsp))
            app_cmd = rsp.type  #   - MRF_APP_CMD_BASE  all wildly inconsistent.. hmpff
            robj = mrf_decode_buff(app_cmd,rdata,cmdset=Pt1000AppCmds)
            self.log.debug("now have robj %s"%repr(robj))
            if not robj:
                self.log.warn("still couldn't make sense of rdata!!!")
                return
            self.pt1000state[hdr.usrc].new_state(hdr,robj)
            

    def fyi(self,hdr,rsp, robj, rdata):
        self.log.debug("heating app got hdr %s"%repr(hdr))
        if hdr.usrc in self._pt1000_addrs :
            self.log.debug( "heating app fyi  he say yes")
            self.pt1000msg(hdr,rsp, robj, rdata)
            
        else:
            self.log.debug("heating app fyi  says no")
        
