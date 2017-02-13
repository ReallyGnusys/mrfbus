test = True
installdir = '/v/share/projects/msp/mrfbus/land/'

instance = 'heat-1'
domain = 'localdom'
host = 'lily'

port = 8888  # real port used by tornado
proxy_port = 8888  # proxy port used by clients ( handled by nginx )
dbname = 'asa_sys'
dbuser = 'asa_sys'


#wsprot = 'wss://'  # or 'wss://'

wsprot = 'ws://'  # or 'wss://'

hostdomain =  host+"."+domain
userdomain =  hostdomain # 'user.'+domain

publicdomain = 'www.'+domain
helpline_webchat = 'https://'+publicdomain+'/chat'
sess_cookie = "MRFLANDSESSID"
public_cookie = "MFLANDPUBLICID"

session_timeout = 1200  # seconds
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



_ssl_cert_file = installdir+'/certs/cert.pem' 
_ssl_key_file =  installdir+'/certs/key.pem'  
