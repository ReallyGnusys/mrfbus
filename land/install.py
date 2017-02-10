test = True
installdir = '/v/share/projects/msp/mrfbus/land/'

instance = 'heat-1'
domain = 'yourorganisation.org'
host = 'nobbit'

port = 8888  # real port used by tornado
proxy_port = 443  # proxy port used by clients ( handled by nginx )
dbname = 'asa_sys'
dbuser = 'asa_sys'


wsprot = 'wss://'  # or 'wss://'


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
