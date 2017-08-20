#! /usr/bin/env python

from tornado.options import  options, parse_command_line  #yFIXME move mrfland_server.py

import mrfland

from mrflog import mrflog, mrf_log_init
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_heatbox import DevHeatbox
from mrfdev_host import MrfDevHost
from mrfland_weblet_temps import MrfLandWebletTemps
from mrfland_weblet_relays import MrfLandWebletRelays
from mrfland_weblet_timers import MrfLandWebletTimers
from mrfland_weblet_devs   import MrfLandWebletDevs
from mrfland_weblet_store  import MrfLandWebletStore
from mrfland_weblet_hot_water  import MrfLandWebletHotWater
from mrfland_server import MrflandServer
import install
            
if __name__ == '__main__':
    #mrf_log_init()
    parse_command_line()
    mrflog.warn("Mrfland web server starting on port "+str(options.port))
    

    rm = mrfland.MrflandRegManager()
    
    MrfDevHost(rm, "host", 1,
                {
                    'timer' : [ "RAD1_P0", "RAD1_P1", "RAD2_P0", "RAD2_P1", "UFH_P0", "UFH_P1"]
                })
    
    Pt1000Dev(rm, "pt1000_boiler_room", 2,
              {
                  'temp' : ["ACC_100", "ACC_50", "ACC_10", "ACC_FLOW", "ACC_RET", "MIX_1", "UFH_MIX"],
                  'relay' : ["UFH_PUMP", "ACC_HEAT"]
              })

    DevHeatbox(rm, "heatbox_kitchen"  , 4,
               {
                   'temp' : ["DHW1_100", "DHW1_25", "HB1_FLOW", "HB1_RET"],
                   'relay' : ["RAD1_PUMP", "DHW1_HX_PUMP"]
               })

    MrfLandWebletStore(rm,
                    {
                        'tag'        : 'store',
                        'label'      : 'Heatstore',
                        'acc_tag'    : 'ACC_',
                        'acc_litres' : 2200
                           
                    })

    MrfLandWebletHotWater(rm,
                    {
                        'tag'        : 'DHW1',
                        'label'      : 'Hot Water 1',
                        'rad'        : 'RAD1',
                        'acctop'     : 'ACC_100',
                        'heatbox'    : 'HB1',
                        'litres'     : 200,
                        'target_temp': 68.0
                           
                    })

    MrfLandWebletTemps(rm,
                       {
                           'tag'  : 'temps',
                           'label': 'Temperatures'
                       })

    MrfLandWebletTimers(rm,
                        {
                            'tag':'timers',
                            'label':'Timers'
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

    ml =  MrflandServer(rm)
