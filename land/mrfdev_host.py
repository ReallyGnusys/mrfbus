
from mrf_sens import MrfSens, MrfDev
from datetime import datetime
from mrf_sens_timer import MrfSensTimer
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test, DefaultAppCmds


    
        
class MrfDevHost(MrfDev):
    _capspec = {
        'timer' : MrfSensTimer
    }                 
 
    _cmdset = DefaultAppCmds

    def app_packet(self, hdr, param , resp):
        self.log.warn("%s app_packet type %s"%(self.__class__.__name__, type(resp)))        
        self.log.warn("hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))
        return
                
