#! /usr/bin/env python

from tornado.options import  define, options, parse_command_line  #FIXME move mrfland_server.py

import mrfland
from mrflog import mrflog
#from mrfdev_pt1000 import Pt1000Dev
#from mrfdev_heatbox import DevHeatbox
from mrfdev_host import MrfDevHost

from mrfdev_lnxtst import DevLnxtst

from mrfland_server import MrflandServer
from mrfland_regmanager import MrflandRegManager
from mrfland_weblet_devs   import MrfLandWebletDevs
from mrfland_weblet_relays import MrfLandWebletRelays
from mrfland_weblet_mem   import MrfLandWebletMem
from mrfland_weblet_period_test import MrfLandWebletPeriodTest
import install

if __name__ == '__main__':
    define("mrfnet", type=int, help="mrfnet ID",default=0x25)
    parse_command_line()
    mrflog.info('Application started')

    rm = MrflandRegManager(
        {
            'http_port'       : 8889,
            'periods'  : ["RAD2", "RAD20"]
        })

    MrfDevHost(rm, "host", 1)

    sx02 = DevLnxtst(rm, "sx02", 0x02 ,
                     {
                         'memory' : ["mem_0x02"],
                         'relay' : ["RAD2_PUMP", "DHW2_HX_PUMP" , "DHW2_HEAT","LIGHT2"]
                     }

    )
    sx20 = DevLnxtst(rm, "sx20", 0x20,
                     {
                         'memory' : ["mem_sx20"],
                         'relay' : ["RAD20_PUMP", "DHW20_HX_PUMP" , "DHW20_HEAT","LIGHT20"]
                     }


    )
    sx21 = DevLnxtst(rm, "sx21", 0x21,
                     {
                         'memory' : ["mem_sx21"],
                         'relay' : ["RAD21_PUMP", "DHW21_HX_PUMP" , "DHW21_HEAT","LIGHT21"]
                     }

    )


    MrfLandWebletMem(rm,
                       {
                           'tag'  : 'mem',
                           'label': 'Memory'
                       })

    MrfLandWebletRelays(rm,
                        {
                            'tag':'relays',
                            'label':'Relays'
                        }
    )

    MrfLandWebletDevs(rm,
                       {
                           'tag'  : 'devs',
                           'label': 'Devices'
                       })


    MrfLandWebletPeriodTest(rm,
                        {
                           'tag'  : 'periods',
                            'label': 'Periods',
                            'timers'     :  [ 'RAD2_P0', 'RAD2_P1' , 'RAD2_P2', 'RAD20_P1','RAD20_P2']

                       })

    ml =  MrflandServer(rm,
                        {
                            'mrfbus_host_port': install.mrfbus_host_port,
                            'mrf_netid'     : 0x25,
                            'tcp_test_port'   : install.tcpport
                        })
