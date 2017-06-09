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

#from pt1000test import *
from mrfdev_pt1000 import *

from heatbox_test import HeatboxAppCmds
from datetime import datetime
from mrfland_weblet import MrflandWeblet
from collections import OrderedDict
from mrf_sens import MrfSens, MrfDev
Pt1000MaxChanns = 7
Pt1000MaxRelays = 4

#### start new implementation here


    
## end of new , start of old

class Pt1000TempSensor(object):
    def __init__(self,address,channel,log):
        self.label = ""
        self.address = address        
        self.channel = channel
        self.log = log
        self.milliohms = 0
        self.temperature = -273.16
    def res_to_temp(self,milliohms):
        R = milliohms/1000.0
        A=3.9083e-3
        B=-5.775e-7
        R0 = 1000.0
        R=R/R0
        T=0.0-A
        tmp = (A*A) - 4.0 * B * (1.0 - R)
        try:
            T = T +  sqrt(tmp)
            T = T/ (2.0 * B)
        except:
            self.log.error("res_to_temp error chan %d  with milliohms %d tmp was %f"%(self.channel,milliohms,tmp))
            T = -273.16
        return T
    def __repr__(self):
        return "Channel %d milliohms %7d %7.3f C  label %s"%(self.channel,self.milliohms,self.temperature, self.label)
    
    def new_reading(self,milliohms,td):
        #self.log.info("new_reading chan %d : %d milliohms"%(self.channel,milliohms))
        #if milliohms == self.milliohms:  # no change
        #    return None
        self.milliohms = milliohms
        self.temperature = self.res_to_temp(self.milliohms)
        self.date = td.to_datetime()
        return self.temperature
        
class Pt1000State(object):
    def __init__(self,address,log):
        self.address = address
        self.log = log
        self.temps = []
        self.updated = []
        self.relay_state = 0
        self.last_reading = None
        
        for i in xrange(Pt1000MaxChanns):
            self.temps.append(Pt1000TempSensor(self.address,i,self.log))

            
    def __repr__(self):
        s = "%s address %d last reading %s\n"%(self.__class__.__name__,self.address,str(self.last_reading))
        for chan in xrange(Pt1000MaxChanns):
            s += " %d) %s\n"%(chan,repr(self.temps[chan]))
        s += "Relays ["
        for chan in xrange(Pt1000MaxRelays):
            s += "%d"%self.relays[chan]
            if chan < Pt1000MaxRelays - 1:
                s += ","
        s += "]\n"
        return s

    def relays(self):  # return relay state as array
        _relays = []
        for i in xrange(Pt1000MaxRelays):
            if self.relay_state & ( 1 << i) != 0:
                _relays.append(1)
            else:
                _relays.append(0)
        return _relays
    
    def new_msg(self,hdr,state):
        if hdr.usrc != self.address:   # just a spot sanity check..
            self.log.warn("PT1000 state wrong address!!")
            return

        if type(state) == type(PktPt1000State()):
            #self.log.warn( "PT1000 state wrong type!! got  %s expected %s"%( type(state),type(PktPt1000State())))
            return self.new_state(hdr,state)
        elif type(state) == type(PktRelayState()):
            #self.log.warn( "PT1000 state wrong type!! got  %s expected %s"%( type(state),type(PktPt1000State())))
            return self.new_relay_state(hdr,state)
        else:
            return
            

    def new_relay_state(self,hdr,state):
        self.log.info("new_relay_state %s"%repr(state))
        _relay_state = self.relay_state
        if state.chan > Pt1000MaxRelays:
            self.log.error("new_relay_state got illegal chan in %s"%repr(state))
            return
        
        if state.val == 0:
            _relay_state = _relay_state  & ~(1 << state.chan)
        elif state.val == 1:
            _relay_state = _relay_state  | (1 << state.chan)
        else:
            self.log.error("new_relay_state got illegal val in %s"%repr(state))
            return

        if _relay_state != self.relay_state:
            self.log.info("relay state change from %0x to %0x "%(self.relay_state,_relay_state))
            self.relay_state = _relay_state
            relays = self.relays()
            robj = {"relays" : relays  }
            return robj
            
        
    def new_state(self,hdr,state):
        
        now = state.td.to_datetime()
    
        if not ( self.last_reading == None or now >= self.last_reading):
            
            self.log.warn( "PT1000 state - time confusion!!! now %s  last %s"%(repr(now),repr(self.last_reading)))
            #return

        self.log.debug("pt1000state %s"%repr(state))
        updated = []
        for ch in xrange(Pt1000MaxChanns):
            updated.append(self.temps[ch].new_reading(state.milliohms[ch],state.td))
            
        msg = ""

        tsensors = {}
        notfirst = False
        for ch in xrange(Pt1000MaxChanns):
            if updated[ch]:
                tsensors[ch] = {}
                tsensors[ch]["temperature"] = self.temps[ch].temperature
                tsensors[ch]["milliohms"]   = self.temps[ch].milliohms
                tsensors[ch]["date"] = now
                msg += "ch %d) %.3f :"%(ch,updated[ch])
                if ch not in self.updated:
                    self.updated.append(ch)
                #self.log.info("ch %s was updated now %s"%(ch,repr(self.updated)))

        self.last_reading = now

        robj = {"tempsensors" : tsensors  }

        if state.relay_state != self.relay_state:
            self.relay_state = state.relay_state
            robj["relays"] = self.relays()
            msg += "relay_state 0x%x"%self.relay_state
            
        
        if msg != "":
            self.log.debug("Pt1000State updated %s"%msg)
            return robj
        return
    
    def curr_state(self):

        if len(self.updated) == 0:
            self.log.info("%s curr_state updated %s returning None"%(self.__class__.__name__,repr(self.updated)))
            return None                    
        tsensors = {}
        for ch in self.updated:            
            tsensors[ch] = {}
            tsensors[ch]["temperature"] = self.temps[ch].temperature
            tsensors[ch]["milliohms"]   = self.temps[ch].milliohms                             
            tsensors[ch]["date"]        = self.temps[ch].date     
            
 
        robj = {"tempsensors" : tsensors , "relays" : self.relays() }
        self.log.info("%s curr_state returning %s"%(self.__class__.__name__,repr(robj)))
        return robj



class MrfLandWebletTemps(MrflandWeblet):
    def pane_js_cmd(self):
        s = """
   var nobskit = True;
"""
        return s


    def pane_js(self):
        s = """
   var nobwit = True;
"""
        return s
    
    def pane_html(self):
        return """
        <h2>Temps(nobbit2)</h2>
        <table class="table">
          <thead>
            <tr>
             <th>ID</th>
             <th>label</th>
             <th>value</th>
             <th>time</th>
            </tr>
          </thead>
          <tbody>
            <tr>
             <td>temp-000</td>
             <td>Top temperature</td>
             <td id="tempsensor-0-value">NADA</td>
             <td id="timedate-0">NADA</td>
            </tr>
            <tr>
             <td>temp-001</td>
             <td>Top temperature</td>
             <td id="tempsensor-1-value">NADA</td>
             <td id="timedate-1">NADA</td>
            </tr>
            <tr>
             <td>temp-002</td>
             <td>Top temperature</td>
             <td id="tempsensor-2-value">NADA</td>
             <td id="timedate-2">NADA</td>
            </tr>
            <tr>
             <td>temp-003</td>
             <td>Top temperature</td>
             <td id="tempsensor-3-value">NADA</td>
             <td id="timedate-3">NADA</td>
            </tr>
          </tbody>
         </table>
"""


class MrfLandWebletPumps(MrflandWeblet):

    def pane_js_cmd(self):
        s = """
        "state" : function();
"""
        return s


    def pane_js(self):
        s = """
   var nobwitpump = True;
"""
        return s

    
    def pane_html(self):
         return """
        <h2>Pumps</h2>
        <table class="table">
          <thead>
            <tr>
             <th>ID</th>
             <th>label</th>
             <th>value</th>
             <th>control</th>
            </tr>
          </thead>
          <tbody>
            <tr>
             <td>pump-000</td>
             <td>Main rads</td>
             <td id="pump-0-value">NADA</td>
             <td id="pump-0-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-0-cb" value="0"></div></td>
            </tr>
            <tr>
             <td>pump-001</td>
             <td>Underfloor</td>
             <td id="pump-1-value">NADA</td>
             <td id="pump-1-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-1-cb" value="1"></div></td>

            </tr>
            <tr>
             <td>pump-002</td>
             <td>DHW charge 1</td>
             <td id="pump-2-value">NADA</td>
              <td id="pump-2-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-2-cb" value="2"></div></td>
           </tr>
            <tr>
             <td>pump-003</td>
             <td>DHW charge 2</td>
             <td id="pump-3-value">NADA</td>
             <td id="pump-3-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-3-cb" value="3"></div></td>
            </tr>
          </tbody>
         </table>
"""

    
_channel_assignment = {
    'temperatures' : { 
        'ACC_TOP'  : { 'mrfid' : 2, 'chan': 0 }, 
        'ACC_MID'  : { 'mrfid' : 2, 'chan': 1 },
        'ACC_LOW'  : { 'mrfid' : 2, 'chan': 2 },
        'MIX_MAIN' : { 'mrfid' : 2, 'chan': 3 },
        'MIX_PUFH' : { 'mrfid'  : 2, 'chan': 4 },
        'MIX_UFH'  : { 'mrfid'  : 2, 'chan': 5 },
        'RET_MAIN' : { 'mrfid'  : 2, 'chan': 6 },
        'FLOW_ST1' : { 'mrfid' : 4, 'chan': 0 }, 
        'RET_HX1'  : { 'mrfid' : 4, 'chan': 1 }, 
        'RET_RAD1'  : { 'mrfid' : 4, 'chan': 2 }, 
        'DHW1_TOP'  : { 'mrfid' : 4, 'chan': 3 }, 
        'DHW1_MID'  : { 'mrfid' : 4, 'chan': 4 }
    }
}
   




class MrflandAppHeating(MrflandApp):


    
    def __init__(self , tag , log=None, cmd_callback=None):
        MrflandApp.__init__( self,tag , log,cmd_callback)
        self.log.info("MrflandAppHeating __init__ called MrflandApp.__init__")
        self._pt1000_addrs = { 2 : Pt1000AppCmds ,4 : HeatboxAppCmds }  #set of addresses

        self.managed_addrs = self._pt1000_addrs

        self.pt1000state = {}

        for add in self._pt1000_addrs:
            self.pt1000state[add] = Pt1000State(add,self.log)


        self.weblets['temps'] =  MrfLandWebletTemps(self.log, {'tag':'temps','label':'Temperatures'})
        self.weblets['pumps'] =  MrfLandWebletPumps(self.log, {'tag':'pumps','label':'Pumps'})

            
                #do labels

        for label in _channel_assignment['temperatures'].keys():
            ts = _channel_assignment['temperatures'][label]
            self.log.info("labelling temp device %s mrfid %d chan %d"%(label,ts['mrfid'],ts['chan']))
            self.pt1000state[ts['mrfid']].temps[ts['chan']].label = label  
        
        # set device RTC

        self.log.info("MrflandAppHeating __init__ exit")

    def post_init(self):
        td = PktTimeDate()
        td.set(datetime.now())
        self.log.info("MrflandAppHeating post_init calling cmd_callbacks")

        for add in self._pt1000_addrs:
            self.cmd_callback(self.tag,add,mrf_cmd_set_time,td)
            self.log.info("setting time for device %d  %s"%(add,repr(td)))
        

    def tbd_cmd_set(self,addr):
        if addr in self._pt1000_addrs:
            return Pt1000AppCmds
        else:
            return None

    def cmd_boris(self,data):
        self.log.info( "cmd boris here, data was %s"%repr(data))

    def cmd_nancy(self,data):
        self.log.info( "cmd nancy here, data was %s"%repr(data))

    def cmd_relay_set(self,data):
        self.log.info( "cmd relay_set here, data was %s"%repr(data))
        if not data.has_key("chan") or not data.has_key("val"):
            self.log.error("cmd_relay_set data problem in %s"%repr(data))
            return
        dest = list(self._pt1000_addrs)[0]
        param = PktRelayState()
        param.dic_set(data)
        self.cmd_callback(self.tag,dest,mrf_cmd_set_relay,param)

        

    def pt1000msg(self,hdr,rsp,robj):
        #if type(robj) != type(PktPt1000State()):
        return self.pt1000state[hdr.usrc].new_msg(hdr,robj)

        
            
    def curr_state(self):
        rv = []
        for add in self._pt1000_addrs:
            st = self.pt1000state[add].curr_state()
            if st:
                rv.append(st)
        return rv
    def test1(self):
        self.log.info("test1")
    def fyi(self,hdr,rsp, robj):
        self.log.debug("heating app got hdr %s"%repr(hdr))
        if hdr.usrc in self._pt1000_addrs :
            self.log.debug( "heating app fyi  he say yes")
            return self.pt1000msg(hdr,rsp, robj)
            
        else:
            self.log.debug("heating app fyi  says no")
        
