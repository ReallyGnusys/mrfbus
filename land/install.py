test = True
#installdir = '/v/share/projects/msp/mrfbus/land/'

instance = 'heat-1'
#domain = 'localdom'
domain = None   # save adding .localdom to req urls

import socket
host = socket.gethostname()
#host = 'ted'

http_port = 8888  # real port served by tornado webapp
http_proxy_port = 8888  # proxy port used by clients ( handled by nginx )


tcpport = 8912   # port used by tcp/json test service
mrfbus_host_port = 8915  # port of mrfbus host device ( addr 1 )


db_uri = "mongodb://mrfbus:sanghamnamami@bolt:27017/mrfbus?authSource=admin"


https_server = False


if https_server:
    wsprot = 'wss://'  # or 'wss://'
else:
    wsprot = 'ws://'

sess_cookie = "MRFLANDSESSID"
public_cookie = "MFLANDPUBLICID"

session_timeout = 60*60*24  # seconds
import logging

#log_level = logging.INFO
log_level = logging.WARN
logger_name = 'mrfland'



logdir = 'log/'
mrflog = 'mrfland.log'

#hard coded userdb
users = {
    'home'  : { 'sid' : 1, 'type' : 'sysadmin', 'password' : 'hpass' , 'username': 'home'},
    'guest' : { 'sid' : 2, 'type' : 'sysadmin', 'password' : 'gpass' , 'username': 'guest',
                'apps' : ['temps','history']},

    
    'q'     : { 'sid' : 3, 'type' : 'sysadmin', 'password' : 'q'     , 'username': 'q',
                'apps' : ['store','rad1', 'rad2', 'DHW1','DHW2','temps','timers','relays','devs','history']},
    'p'     : { 'sid' : 4, 'type' : 'sysadmin', 'password' : 'p'     , 'username': 'p',
                'apps' : ['store','rad1','rad2','DHW1','DHW2','temps','timers','relays','devs','history']},
    'j'     : { 'sid' : 5, 'type' : 'sysadmin', 'password' : 'j'     , 'username': 'j',
                'apps' : ['rad2','history']}
}



_ssl_cert_file = 'certs/'+host+'/cert.pem' 
_ssl_key_file =  'certs/'+host+'/private/key.pem'  
