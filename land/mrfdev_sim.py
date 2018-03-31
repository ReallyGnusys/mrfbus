
from mrf_sens import MrfSens
from mrf_dev  import MrfDev
from datetime import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test, DefaultAppCmds
from mrflog import mrflog

    
        
class MrfDevSim(MrfDev):
 
    _cmdset = DefaultAppCmds
    _capspec = {
    }                 

    def app_packet(self, hdr, param , resp):
        mrflog.info("%s app_packet type %s"%(self.__class__.__name__, type(resp)))        
        mrflog.info("hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))
        return
                
