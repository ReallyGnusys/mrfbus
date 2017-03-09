test = True
installdir = '/v/share/projects/msp/mrfbus/land/'

instance = 'heat-1'
#domain = 'localdom'
domain = None   # save adding .localdom to req urls
host = 'ted'

port = 8888  # real port used by tornado webapp
proxy_port = 8888  # proxy port used by clients ( handled by nginx )


tcpport = 8912   # port used by tcp/json service

dbname = 'asa_sys'
dbuser = 'asa_sys'

from datetime import datetime
upsince = datetime.now()
wsprot = 'wss://'  # or 'wss://'

#wsprot = 'ws://'  # or 'wss://'

#hostdomain =  host+"."+domain
#userdomain =  hostdomain # 'user.'+domain

#publicdomain = 'www.'+domain
#helpline_webchat = 'https://'+publicdomain+'/chat'
sess_cookie = "MRFLANDSESSID"
public_cookie = "MFLANDPUBLICID"

session_timeout = 60*60*24  # seconds
import logging

log_level = logging.INFO
logger_name = 'mrfland'


logdir = installdir+'log/'
mrflog = 'mrfland.log'

#hard coded userdb
users = {
    'home'  : { 'sid' : 1, 'type' : 'sysadmin', 'password' : 'hpass' , 'username': 'home'},
    'guest' : { 'sid' : 2, 'type' : 'sysadmin', 'password' : 'gpass' , 'username': 'guest'},
    'q'     : { 'sid' : 3, 'type' : 'sysadmin', 'password' : 'q'     , 'username': 'q'}
}



_ssl_cert_file = installdir+'/certs/'+host+'/cert.pem' 
_ssl_key_file =  installdir+'/certs/'+host+'/private/key.pem'  
