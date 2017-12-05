#! /usr/bin/env python

from tornado.options import  options, parse_command_line  #FIXME move mrfland_server.py

import mrfland

from mrflog import mrflog, mrf_log_init
from mrfland_server import MrflandServer
from mrfland_weblet_history import MrfLandWebletHistory
from mrfland_weblet_rad_pump import MrfLandWebletRadPump
from mrfland_regmanager import MrflandRegManager
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_host import MrfDevHost

import install
            
if __name__ == '__main__':
    #mrf_log_init()
    parse_command_line()
    

    rm = MrflandRegManager(
        {
            'http_port'       : 9999,
            'db_uri'          : install.db_uri
        })

    MrfDevHost(rm, "host", 1)
    
    Pt1000Dev(rm, "pt1000_boiler_room", 2,
              {
                  'temp' : ["ACC_100", "ACC_60", "ACC_30", "MAIN_MIX", "ACC_RET" , "UFH_FLOW", "UFH_RET"],
                  'relay' : ["UFH_PUMP", "UNUSED_PUMP", "ACC_HEAT"]
              })

    Pt1000Dev(rm, "pt1000_kitchen"  , 4,
               {
                   'temp' : ["DHW1_100",  "HB1_FLOW", "HB1_RET", "RAD1_RET", "DHW1_HX_RET",  "HB1_AMBIENT","LOUNGE_AMBIENT"],
                   'relay' : ["RAD1_PUMP", "DHW1_HX_PUMP" , "DHW1_HEAT"]
               })
    
    Pt1000Dev(rm, "pt1000_guest"  , 6,
               {
                   'temp' : ["DHW2_100",  "DHW2_30", "HB2_FLOW", "HB2_RET", "DHW2_HX_RET",  "HB2_AMBIENT","OUTSIDE_AMBIENT"],
                   'relay' : ["RAD2_PUMP", "DHW2_HX_PUMP" , "DHW2_HEAT"]
               })

    
    
    MrfLandWebletHistory(rm,
                    {
                        'tag'        : 'history',
                        'label'      : 'History'
                           
                    })
    MrfLandWebletRadPump(rm,
                         {
                             'tag'        : 'rad1',
                             'label'      : 'Main Rads',
                             'rad'        : 'RAD1',
                             'pump'       : 'RAD1_PUMP',
                             'flowsens'   : 'HB1_FLOW',
                             'retsens'    : 'RAD1_RET',
                             'timers'     :  [ 'RAD1_P0', 'RAD1_P1' , 'RAD1_P2']
                         },
                         {
                             'max_return' : 45.0,
                             'hysterisis' : 5.0
                         })

    ml =  MrflandServer(rm,
                        {
                            'console'         : { 'port' : 1234}
                        })
