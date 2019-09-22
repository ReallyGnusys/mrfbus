## normally copy this your run dir  and name install.py
## setup as required

## THIS FILE IS IN PROCESS OF DEPRECATION
## Trying to move most of these to config for weblets in server.py
## so watch out if changes here don't seem to do anything


test = True

instance = 'heat-1'
#domain = 'localdom'
domain = None   # save adding .localdom to req urls

import socket
host = socket.gethostname()
#host = 'ted'

http_port = 8888  # real port served by tornado webapp
http_proxy_port = 8888  # proxy port used by clients ( handled by nginx )

cookie_secret = "dighobalanamsamarosaddhammamavijanatam"


tcpport = 8912   # port used by tcp/json test service
mrfbus_host_port = 8915  # port of mrfbus host device ( addr 1 )


db_uri = "mongodb://username:password@host:27017/mrfbus?authSource=admin"


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



logdir = 'var/log/'
mrflog = 'mrfland.log'

#temp hard coded userdb for now
users = {
    'home'  : { 'sid' : 1, 'type' : 'sysadmin', 'password' : 'hpass' , 'username': 'home'},
    'guest' : { 'sid' : 2, 'type' : 'sysadmin', 'password' : 'gpass' , 'username': 'guest',
                'apps' : ['temps','history']},


    'q'     : { 'sid' : 3, 'type' : 'sysadmin', 'password' : 'q'     , 'username': 'q',
                'apps' : '*'},
    'p'     : { 'sid' : 4, 'type' : 'sysadmin', 'password' : 'p'     , 'username': 'p',
                'apps' : '*' },
    'j'     : { 'sid' : 5, 'type' : 'sysadmin', 'password' : 'j'     , 'username': 'j',
                'apps' : ['rad2','history']}
}



_ssl_cert_file = 'certs/'+host+'/cert.pem'
_ssl_key_file =  'certs/'+host+'/private/key.pem'
