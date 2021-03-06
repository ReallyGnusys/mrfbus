import tornado.web
import tornado.template
import re
import install
import os
import base64
import sys
sys.path.append('../lib')
from mrflog import mrflog
import templates

#alog = mrf_log()

def print_everything(*args):
    mrflog.debug( "print *args")
    for count, thing in enumerate(args):
        mrflog.debug( '{0}. {1}'.format(count, thing))

def print_kwargs(**kwargs):
    mrflog.debug("print_kwargs")
    for name, value in list(kwargs.items()):
        mrflog.debug('{0} = {1}'.format(name, value))


#conn = asa.Connection()

remarg = re.compile(r'[?].*$')

#redemohost = re.compile(r'^'+install.hostdomain+':'+str(install.port)+'$')

public_tp = tornado.template.Template(templates.public_tp)









def public_page(rh,ip,ws_url):
    mrflog.info("public_page")
    mrflog.info("pjs")
    rh.write(public_tp.generate(ws_url=ws_url,email=install.helplineaddr,sms=install.helpline_display_sms))








def gen_sessid():
    return base64.b64encode(os.urandom(18))

class publicapp(tornado.web.RequestHandler):
    def post(self, *args, **kwargs):
        mrflog.info('post:'+str(self.request))
        mrflog.info("uri : "+self.request.uri)
        uri = remarg.sub('',self.request.uri)
        mrflog.info("uri : "+uri)
        reqa = uri.split('/')[1:]
        page = reqa[1]
        mrflog.info("page = "+page)


    def get(self, *args, **kwargs):
        mrflog.info('get:'+str(self.request))
        mrflog.info("uri : "+self.request.uri)
        mrflog.info("host : "+self.request.host)

        uri = remarg.sub('',self.request.uri)
        ip = self.request.remote_ip

        # handle nginx proxying
        if 'X-Forwarded-For' in self.request.headers:
            ip = self.request.headers['X-Forwarded-For']
        mrflog.info("ip: "+ip+" uri : "+uri)
        reqa = uri.split('/')[1:]

        page = reqa[0]
        mrflog.info("page = "+page)

        if len(reqa) > 1:
            action = reqa[1]
        else:
            action = None
        cookie = self.get_secure_cookie(install.public_cookie)
        mrflog.info("cookie = "+str(cookie))
        if cookie == None:
            #cookie = asa.gen_pub_cookie()
            #self.set_secure_cookie(install.public_cookie,cookie)
            mrflog.info("set cookie = "+str(cookie))
        wsid = os.urandom(16).encode('hex')
        #ws_url = asa.pws_url(conn,ip,cookie)
        return public_page(self,ip,ws_url)




#static_handler = tornado.web.StaticFileHandler
