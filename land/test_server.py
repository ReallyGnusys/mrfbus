
from tornado.options import  define, options, parse_command_line  #FIXME move mrfland_server.py

from . import mrfland
from .mrflog import mrflog
#from mrfdev_pt1000 import Pt1000Dev
#from mrfdev_heatbox import DevHeatbox
from .mrfdev_host import MrfDevHost

from .mrfdev_lnxtst import DevLnxtst

from .mrfland_server import MrflandServer
from .mrfland_regmanager import MrflandRegManager
from .mrfland_weblet_devs   import MrfLandWebletDevs
from .mrfland_weblet_relays import MrfLandWebletRelays
from .mrfland_weblet_mem   import MrfLandWebletMem
from .mrfland_weblet_period_test import MrfLandWebletPeriodTest
from . import install

if __name__ == '__main__':
    define("mrfnet", type=int, help="mrfnet ID",default=0x25)
    parse_command_line()
    mrflog.info('Application started')

    rm = MrflandRegManager(
        {
            'http_port'       : 8889
        })

    MrfDevHost(rm, "host", 1)

    sx02 = DevLnxtst(rm, "LT02", 0x02 ,
                     {
                         'memory' : ["LT02_MEM"],
                         'relay' : ["LT02_RAD_PUMP", "LT02_HX_PUMP" , "LT02_HEAT","LT02_LIGHT"]
                     }

    )
    sx20 = DevLnxtst(rm, "LT20", 0x20,
                     {
                         'memory' : ["LT20_MEM"],
                         'relay' : ["LT20_RAD_PUMP", "LT20_HX_PUMP" , "LT20_HEAT","LT20_LIGHT"]
                     }


    )
    sx21 = DevLnxtst(rm, "LT21", 0x21,
                     {
                         'memory' : ["LT21_MEM"],
                         'relay' : ["LT21_RAD_PUMP", "LT21_HX_PUMP" , "LT21_HEAT","LT21_LIGHT"]
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
                            'tag'  : 'CT02',
                            'label': 'Ctrl02',
                            'targ' : 'LT02'


                       })

    ml =  MrflandServer(rm,
                        {
                            'mrfbus_host_port': install.mrfbus_host_port,
                            'mrf_netid'     : 0x25,
                            'tcp_test_port'   : install.tcpport
                        })
