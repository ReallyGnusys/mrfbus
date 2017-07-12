
from mrf_sens import MrfSens, MrfDev
from datetime import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test, DefaultAppCmds
import mrflog

    
        
class MrfDevSim(MrfDev):
 
    _cmdset = DefaultAppCmds
    _capspec = {
    }                 

    def app_packet(self, hdr, param , resp):
        mrflog.warn("%s app_packet type %s"%(self.__class__.__name__, type(resp)))        
        mrflog.warn("hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))
        return
                
