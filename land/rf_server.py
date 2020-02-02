#! /usr/bin/env python

from tornado.options import  define, options, parse_command_line  #FIXME move mrfland_server.py

import mrfland

from mrflog import mrflog, mrf_log_init
from mrfland_server import MrflandServer

from mrfland_weblet_relays import MrfLandWebletRelays
from mrfland_weblet_devs   import MrfLandWebletDevs
from mrfland_weblet_temps   import MrfLandWebletTemps

from mrfland_regmanager import MrflandRegManager
from mrfdev_host import MrfDevHost
from mrfdev_default import DevDefault
#from mrfdev_rftst import DevRftst
from mrfdev_rfmodtc import DevRfmodtc

import install
            
if __name__ == '__main__':
    #mrf_log_init()
    define("mrfnet", type=int, help="mrfnet ID",default=0x25)
    parse_command_line()
    
    
    rm = MrflandRegManager(
        {
            'http_port'       : 9999,
             #'db_uri'          : install.db_uri
        })

    MrfDevHost(rm, "host", 1)
    
    DevDefault(rm, "basestation_2", 2,
              {
              })
    """
    DevRftst(rm, "rftest_20"  , 0x20,
               {
                   'relay' : ["LED1_SWITCH"]
               })
    """
    DevRfmodtc(rm, "rftst_20"  , 0x20,
               {
                   'relay' : ["LED1_SWITCH"],
                   'temp'  : ["RFTST_AMBIENT"]
               })
    
    
    DevRfmodtc(rm, "rfmodtc_21"  , 0x21,
               {
                   'relay' : ["RFTC1A_SWITCH","RFTC1B_SWITCH"],
                   'temp'  : ["RFTC1_AMBIENT"]
               })
    
    
    MrfLandWebletRelays(rm,
                        {
                            'tag':'relays',
                            'label':'Relays'
                        }
    )

    MrfLandWebletTemps(rm,
                       {
                           'tag'  : 'temps',
                           'label': 'Temperatures'
                       })
    
    MrfLandWebletDevs(rm,
                       {
                           'tag'  : 'devs',
                           'label': 'Devices'
                       })
     

    ml =  MrflandServer(rm,
                        {
                            'mrfbus_host_port': install.mrfbus_host_port,
                            'mrf_netid'     : options['mrfnet'],
                            'tcp_test_port'   : install.tcpport ,
                            'console'         : { 'port' : 1234}
                        })
