#! /usr/bin/env python

from tornado.options import  options, parse_command_line  #yFIXME move mrfland_server.py

import mrfland

import mrflog
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_heatbox import DevHeatbox
from mrfdev_host import MrfDevHost
from mrfland_weblet_temps import MrfLandWebletTemps
from mrfland_weblet_relays import MrfLandWebletRelays
from mrfland_weblet_timers import MrfLandWebletTimers
from mrfland_server import MrflandServer

            
if __name__ == '__main__':
    parse_command_line()
    mrflog.info('Application started')
    mrflog.info("Mrfland web server starting on port "+str(options.port))
    
    hostlabels = {
        'timer' : [ "RAD1_P0", "RAD1_P1", "RAD2_P0", "RAD2_P1"]
        }

   
    hb0labels = {
        'temp' : ["ACC_TOP", "ACC_MID", "ACC_BOT", "ACC_FLOW", "ACC_RET", "MIX_1", "UFH_MIX"],
        'relay' : ["UFH_PUMP", "MAIN_RAD_PUMP"]
        }


    hb1labels = {
        'temp' : ["DHW1_TOP", "DHW1_MID", "DHW1_BOT", "HB1_FLOW", "RADS1_RET",  "DHW1_HX_RET", "HB1_AMBIENT"],
        'relay' : ["RADS1_PUMP", "DHW1_HX_PUMP"]
        }

    rm = mrfland.MrflandRegManager()
    
    host = MrfDevHost(rm, "host", 1, {})
    
    hb0 = Pt1000Dev(rm, "pt1000_boiler_room", 2, hb0labels)

    hb1 = DevHeatbox(rm, "heatbox_kitchen"  , 4, hb1labels)

    

    #rm.device_register(host)
    #rm.device_register(hb0)
    #rm.device_register(hb1)

    #wat = MrfLandWebletTemps(rm, alog, {'tag':'temps','label':'Temperatures'})
    #war = MrfLandWebletRelays(rm, alog, {'tag':'relays','label':'Relays'})


    rm.weblet_register(MrfLandWebletTemps(rm, 
                                          {'tag':'temps','label':'Temperatures'}))

    rm.weblet_register(MrfLandWebletRelays(rm,
                                           {'tag':'relays','label':'Relays'}))
    
    rm.weblet_register(MrfLandWebletTimers(rm, 
                                           {'tag':'timers','label':'Timers'}))
    
    
    
    ml =  MrflandServer(rm)


