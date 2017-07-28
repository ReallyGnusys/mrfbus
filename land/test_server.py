#! /usr/bin/env python

from tornado.options import  options, parse_command_line  #FIXME move mrfland_server.py

import mrfland
import mrflog
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_heatbox import DevHeatbox
from mrfdev_sim import MrfDevSim
from mrfland_server import MrflandServer

            
if __name__ == '__main__':
    parse_command_line()
    mrflog.info('Application started')
    mrflog.info("Mrfland web server starting on port "+str(options.port))
    
    rm = mrfland.MrflandRegManager()
    
    sx01 = MrfDevSim(rm, "sx01", 1, {} )
    sx02 = MrfDevSim(rm, "sx02", 2, {} )
    sx20 = MrfDevSim(rm, "sx20", 0x20, {} )
    sxsf = MrfDevSim(rm, "sx2f", 0x2f, {} )
    
    ml =  MrflandServer(rm)


