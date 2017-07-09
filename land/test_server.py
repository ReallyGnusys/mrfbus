#! /usr/bin/env python

from tornado.options import  options, parse_command_line  #yFIXME move mrfland_server.py

import mrfland
from mrflog import mrf_log_init, mrflog
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_heatbox import DevHeatbox
from mrfdev_sim import MrfDevSim
from mrfland_server import MrflandServer

            
if __name__ == '__main__':
    parse_command_line()
    alog = mrf_log_init()
    alog.info('Application started')
    alog.info("Mrfland web server starting on port "+str(options.port))
    
    rm = mrfland.MrflandRegManager(alog)



    
    sx01 = MrfDevSim(rm, "sx01", 1, {}, alog)
    sx02 = MrfDevSim(rm, "sx02", 1, {}, alog)
    sx20 = MrfDevSim(rm, "sx20", 1, {}, alog)
    sxsf = MrfDevSim(rm, "sx2f", 1, {}, alog)
    
    ml =  MrflandServer(rm, alog )


