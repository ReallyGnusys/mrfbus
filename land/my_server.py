#! /usr/bin/env python

from tornado.options import  define, options, parse_command_line  #FIXME move mrfland_server.py

import mrfland

from mrflog import mrflog, mrf_log_init
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_host import MrfDevHost
from mrfdev_rfmodtc import DevRfmodtc

from mrfland_weblet_temps  import MrfLandWebletTemps
from mrfland_weblet_relays import MrfLandWebletRelays
from mrfland_weblet_devs   import MrfLandWebletDevs
from mrfland_weblet_store  import MrfLandWebletStore
from mrfland_weblet_hot_water  import MrfLandWebletHotWater
from mrfland_weblet_rad_pump import MrfLandWebletRadPump
from mrfland_weblet_ufh import MrfLandWebletUFH
from mrfland_weblet_history import MrfLandWebletHistory

from mrfland_server import MrflandServer
import install

from mrfland_regmanager import MrflandRegManager

if __name__ == '__main__':
    #mrf_log_init()
    define("mrfnet", type=int, help="mrfnet ID",default=0x25)  # for tornado  - do we still need this?

    parse_command_line()


    rm = MrflandRegManager( {
        'http_port'       : 8888,
        'db_uri'          : install.db_uri,
        'periods'  : ["RAD1", "RAD2","DHW1_HX","DHW1_IM", "DHW2_HX", "DHW2_IM", "UFH"]

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


    # RF devices

    DevRfmodtc(rm, "rfmodtc_21"  , 0x21,
               {
                   'relay' : ["RFTC1A_SWITCH","RFTC1B_SWITCH"],
                   'temp'  : ["RFTC1_AMBIENT"]
               })



    MrfLandWebletStore(rm,
                    {
                        'tag'        : 'store',
                        'label'      : 'Heatstore',
                        'acc_tag'    : 'ACC',
                        'acc_litres' : 2200

                    })

    MrfLandWebletUFH(rm,
                         {
                             'tag'        : 'ufh1',
                             'label'      : 'Lounge',
                             'period'     : 'UFH',
                             'ambient'    : 'LOUNGE_AMBIENT',
                             'outside'    : 'OUTSIDE_AMBIENT',
                             'pump'       : 'UFH_PUMP',
                             'storesens'  : 'ACC_100',
                             'flowsens'   : 'UFH_FLOW',
                             'retsens'    : 'UFH_RET',
                             'timers'     :  [ 'UFH_P0', 'UFH_P1' , 'UFH_P2']
                         })


    MrfLandWebletRadPump(rm,
                         {
                             'tag'        : 'rad1',
                             'label'      : 'Main Rads',
                             'rad'        : 'RAD1',
                             'pump'       : 'RAD1_PUMP',
                             'storesens'  : 'ACC_100',
                             'flowsens'   : 'HB1_FLOW',
                             'retsens'    : 'RAD1_RET',
                             'timers'     :  [ 'RAD1_P0', 'RAD1_P1' , 'RAD1_P2']
                         })

    MrfLandWebletRadPump(rm,
                         {
                             'tag'        : 'rad2',
                             'label'      : 'Guest Rads',
                             'rad'        : 'RAD2',
                             'pump'       : 'RAD2_PUMP',
                             'storesens'  : 'ACC_100',
                             'flowsens'   : 'HB2_FLOW',
                             'retsens'    : 'HB2_RET',
                             'timers'     :  [ 'RAD2_P0', 'RAD2_P1' , 'RAD2_P2']
                         })


    MrfLandWebletHotWater(rm,
                          {
                              'tag'        : 'DHW1',
                              'label'      : 'Main Hot Water',
                              'rad'        : 'RAD1',
                              'acctop'     : 'ACC_100',
                              'heatbox'    : 'HB1',
                              'tagperiods' : [{'name':'HX','pulse' :True}, {'name' : 'IM','pulse':True}],
                              'litres'     : 200

                          },
                          {
                              'target_temp': 60.0,
                              'delta_targ_rx' : 8.0,
                              'min_wait_hours' : 16.0,
                              'enabled'  : False
                          })

    MrfLandWebletHotWater(rm,
                          {
                              'tag'        : 'DHW2',
                              'label'      : 'Guest Hot Water',
                              'rad'        : 'RAD2',
                              'acctop'     : 'ACC_100',
                              'heatbox'    : 'HB2',
                              'tagperiods' : [{'name':'HX','pulse' :True}, {'name' : 'IM','pulse':True}],
                              'litres'     : 200
                          },
                          {
                              'target_temp'    : 60.0,
                              'delta_targ_rx'  : 8.0,
                              'min_wait_hours' : 16.0,
                              'enabled'  : False
                          })


    MrfLandWebletTemps(rm,
                       {
                           'tag'  : 'temps',
                           'label': 'Temperatures'
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

    MrfLandWebletHistory(rm,
                    {
                        'tag'        : 'history',
                        'label'      : 'History'

                    })

    ml =  MrflandServer(rm,
                        {
                            'mrfbus_host_port': install.mrfbus_host_port,
                            'mrf_netid'     : options['mrfnet'],
                            'tcp_test_port'   : install.tcpport ,
                            'console'         : { 'port' : 1234}

                        })
