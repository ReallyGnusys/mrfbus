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

from mrf_sens import MrfSens
from mrf_dev  import MrfDev
import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test
from math import sqrt
from mrflog import mrflog
from collections import deque


    

mrf_app_cmd_test = 128


DefaultAppCmds = {
    mrf_cmd_app_test : {
        'name'  : "APP_TEST",
        'param' : None,
        'resp'  : PktTimeDate
    }
}





        
class DevDefault(MrfDev):

    _capspec = {}                 
    _cmdset = DefaultAppCmds

    def app_packet(self, hdr, param , resp):
        mrflog.info("%s app_packet type %s"%(self.__class__.__name__, type(resp)))
        
        mrflog.info("DefaultDev app_packet, hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))


