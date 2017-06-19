
from mrf_sens import MrfSens, MrfDev
from datetime import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test, DefaultAppCmds


    
        
class MrfDevHost(MrfDev):

    _capspec = {}                 
    _cmdset = DefaultAppCmds

    def app_packet(self, hdr, param , resp):
        self.log.info("%s app_packet type %s"%(self.__class__.__name__, type(resp)))        
        self.log.info("hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))
        return
                
