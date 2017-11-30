#! /usr/bin/env python

from tornado.options import  options, parse_command_line  #FIXME move mrfland_server.py

import mrfland

from mrflog import mrflog, mrf_log_init
from mrfland_server import MrflandServer
from mrfland_weblet_history import MrfLandWebletHistory
from mrfland_regmanager import MrflandRegManager

import install
            
if __name__ == '__main__':
    #mrf_log_init()
    parse_command_line()
    

    rm = MrflandRegManager(
        {
            'http_port'       : 9999,
            'db_uri'          : install.db_uri
        })
     
    
    MrfLandWebletHistory(rm,
                    {
                        'tag'        : 'history',
                        'label'      : 'History'
                           
                    })
    MrfLandWebletHistory(rm,
                    {
                        'tag'        : 'history1',
                        'label'      : 'History1'
                           
                    })

    ml =  MrflandServer(rm,
                        {
                            'console'         : { 'port' : 1234}
                        })
