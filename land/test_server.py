#! /usr/bin/env python

from tornado.options import  define, options, parse_command_line  #FIXME move mrfland_server.py

import mrfland
from mrflog import mrflog
#from mrfdev_pt1000 import Pt1000Dev
#from mrfdev_heatbox import DevHeatbox
from mrfdev_lnxtst import DevLnxtst

from mrfland_server import MrflandServer
from mrfland_regmanager import MrflandRegManager

import install
            
if __name__ == '__main__':
    define("mrfnet", type=int, help="mrfnet ID",default=0x25)
    parse_command_line()
    mrflog.info('Application started')
    
    rm = MrflandRegManager(
        {
            'http_port'       : 8889
        })
     
    
    sx01 = DevLnxtst(rm, "sx01", 1)
    sx02 = DevLnxtst(rm, "sx02", 2)
    sx20 = DevLnxtst(rm, "sx20", 0x20)
    sxsf = DevLnxtst(rm, "sx2f", 0x2f)
    
    ml =  MrflandServer(rm,
                        {
                            'mrfbus_host_port': install.mrfbus_host_port,
                            'mrf_netid'     : 0x25,
                            'tcp_test_port'   : install.tcpport 
                        })


