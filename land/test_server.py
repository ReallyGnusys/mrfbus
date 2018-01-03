#! /usr/bin/env python

from tornado.options import  options, parse_command_line  #FIXME move mrfland_server.py

import mrfland
from mrflog import mrflog
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_heatbox import DevHeatbox
from mrfdev_sim import MrfDevSim
from mrfland_server import MrflandServer
from mrfland_regmanager import MrflandRegManager

import install
            
if __name__ == '__main__':
    parse_command_line()
    mrflog.info('Application started')
    
    rm = MrflandRegManager(
        {
            'http_port'       : 8889
        })
     
    
    sx01 = MrfDevSim(rm, "sx01", 1, {} )
    sx02 = MrfDevSim(rm, "sx02", 2, {} )
    sx20 = MrfDevSim(rm, "sx20", 0x20, {} )
    sxsf = MrfDevSim(rm, "sx2f", 0x2f, {} )
    
    ml =  MrflandServer(rm,
                        {
                            'mrfbus_host_port': install.mrfbus_host_port,
                            'mrf_netid'     : 0x25,
                            'tcp_test_port'   : install.tcpport 
                        })


