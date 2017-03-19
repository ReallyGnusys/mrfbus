import tornado.web
import tornado.template
import re
import install
import os
import base64
import sys
sys.path.append('../lib')
from asalog import asa_log
import templates
import psycopg2
alog = asa_log()

def print_everything(*args):
    alog.debug( "print *args")
    for count, thing in enumerate(args):
        alog.debug( '{0}. {1}'.format(count, thing))

def print_kwargs(**kwargs):
    alog.debug("print_kwargs")
    for name, value in kwargs.items():
        alog.debug('{0} = {1}'.format(name, value))


#conn = asa.Connection()

remarg = re.compile(r'[?].*$')

#redemohost = re.compile(r'^'+install.hostdomain+':'+str(install.port)+'$')

public_tp = tornado.template.Template(templates.public_tp)









def public_page(rh,ip,ws_url):
    alog.info("public_page")
    alog.info("pjs")
    rh.write(public_tp.generate(ws_url=ws_url,email=install.helplineaddr,sms=install.helpline_display_sms))








def gen_sessid():
    return base64.b64encode(os.urandom(18))

class publicapp(tornado.web.RequestHandler):
    def post(self, *args, **kwargs):
        alog.info('post:'+str(self.request))
        alog.info("uri : "+self.request.uri)
        uri = remarg.sub('',self.request.uri)
        alog.info("uri : "+uri)
        reqa = uri.split('/')[1:]
        page = reqa[1]
        alog.info("page = "+page)

        
    def get(self, *args, **kwargs):
        alog.info('get:'+str(self.request))
        alog.info("uri : "+self.request.uri)
        alog.info("host : "+self.request.host)
  
        uri = remarg.sub('',self.request.uri)
        ip = self.request.remote_ip

        # handle nginx proxying
        if self.request.headers.has_key('X-Forwarded-For'):
            ip = self.request.headers['X-Forwarded-For']
        alog.info("ip: "+ip+" uri : "+uri)
        reqa = uri.split('/')[1:]
        
        page = reqa[0]
        alog.info("page = "+page)
 
        if len(reqa) > 1:
            action = reqa[1]
        else:
            action = None
        cookie = self.get_secure_cookie(install.public_cookie)        
        alog.info("cookie = "+str(cookie))
        if cookie == None:
            #cookie = asa.gen_pub_cookie()
            #self.set_secure_cookie(install.public_cookie,cookie)
            alog.info("set cookie = "+str(cookie))
        wsid = os.urandom(16).encode('hex')
        #ws_url = asa.pws_url(conn,ip,cookie)
        return public_page(self,ip,ws_url)
     
        
        

#static_handler = tornado.web.StaticFileHandler
