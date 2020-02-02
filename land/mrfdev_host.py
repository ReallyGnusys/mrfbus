
from .mrf_sens import MrfSens
from .mrf_dev  import MrfDev
from datetime import datetime
#from mrf_sens_timer import MrfSensTimer
import ctypes
from .mrf_structs import *
from .core_tests import mrf_cmd_app_test, DefaultAppCmds
from .mrflog import mrflog

    
        
class MrfDevHost(MrfDev):
    #_capspec = {
    #    'timer' : MrfSensTimer
    #
    _capspec = {}
    
    _cmdset = DefaultAppCmds

    def app_packet(self, hdr, param , resp):
        mrflog.warn("%s app_packet type %s"%(self.__class__.__name__, type(resp)))        
        mrflog.warn("hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))
        return
                
