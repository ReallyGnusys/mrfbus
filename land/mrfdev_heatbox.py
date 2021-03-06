
from mrf_sens import MrfSens
from mrf_dev import MrfDev
import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test
from math import sqrt
from mrfdev_pt1000 import PktPt1000State, PktRelayState, MrfSensPt1000, MrfSensPtRelay
from mrflog import mrflog

MAX_RTDS = 7


mrf_cmd_app_test = 128
mrf_cmd_read_state  = 129
mrf_cmd_get_relay   = 130
mrf_cmd_set_relay   = 131


HeatboxAppCmds = {
    mrf_cmd_app_test : {
        'name'  : "APP_TEST",
        'param' : None,
        'resp'  : PktTimeDate
    },
    mrf_cmd_read_state : {
        'name'  : "READ_STATE",
        'param' : None,
        'resp'  : PktPt1000State
    },
    mrf_cmd_get_relay : {
        'name'  : "GET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },
    mrf_cmd_set_relay : {
        'name'  : "SET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },

}






    
        
class DevHeatbox(MrfDev):

    _capspec = {
        'temp' : MrfSensPt1000,
        'relay' : MrfSensPtRelay}                 
    _cmdset = HeatboxAppCmds

    def init(self):
        mrflog.warn("%s init function"%(self.__class__.__name__))
        for s in self.caps['temp']:
            mrflog.warn("%s setting 5 tap filter on sens %s"%(self.__class__.__name__,s.label))
            s.filter(10)

    def app_packet(self, hdr, param , resp):
        mrflog.warn("%s app_packet type %s"%(self.__class__.__name__, type(resp)))
        
        mrflog.warn("Heatbox app_packet, hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))
        if hasattr(resp,'td') and resp.td.day == 0:
            resp.td.day = 16  # FIXME-just broken h/w work around

        if param.type == mrf_cmd_read_state:

            for ch in range(len(self.caps['temp'])):
                #mrflog.warn("chan %s milliohms %d type %s"%(ch, resp.milliohms[ch], type(resp.milliohms[ch])))
                inp = { 'date' : resp.td,
                        'milliohms' : resp.milliohms[ch]
                }
                self.caps['temp'][ch].input(inp)


            for ch in range(len(self.caps['relay'])):
                (resp.relay_state >> ch) & 0x01
                inp = { 'date' : resp.td,
                        'relay' :  (resp.relay_state >> ch) & 0x01
                }
                self.caps['relay'][ch].input(inp)
                

        elif  param.type == mrf_cmd_set_relay or param.type == mrf_cmd_get_relay:
            now = datetime.datetime.now()
            td =  PktTimeDate()
            td.set(now)
            inp = { 'date' : td,
                    'relay' :  resp.val
            }
            self.caps['relay'][resp.chan].input(inp)
            
