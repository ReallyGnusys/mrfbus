import tornado.web
import tornado.template
import re
import install
import os
import base64
import sys
sys.path.append('../lib')
#from mrflog import mrf_log
import templates
import mrfland
import ipaddress

#alog = mrf_log()
from mrflog import mrflog

oursubnet = ipaddress.ip_network(install.localnet)


def print_everything(*args):
    mrflog.debug( "print *args")
    for count, thing in enumerate(args):
        mrflog.debug( '{0}. {1}'.format(count, thing))
def print_kwargs(**kwargs):
    mrflog.debug("print_kwargs")
    for name, value in list(kwargs.items()):
        mrflog.debug('{0} = {1}'.format(name, value))


conn = None

remarg = re.compile(r'[?].*$')

login_tp = tornado.template.Template(templates.login_tp)
mrf_tp = tornado.template.Template(templates.mrf_tp)

def login_page(rh,css_html,js_html):
    rh.add_header("Expires",0)

    rh.write(login_tp.generate(css_html=css_html,js_html=js_html))
    #rh.write("Login page")

def logout_action(rh,sob,ip):

    ro = mrfland.staff_logout(sob['sid'],sob['username'],sob['wsid'],ip)
    rh.clear_cookie(install.sess_cookie)

    rh.redirect('/login')



def mrf_pills(weblets):
    s = ""
    first = True
    for wa in list(weblets.keys()):
        wl = weblets[wa]
        if first:
            lic = ' class="active"'
            first = False
        else:
            lic = ''

        #s += '    <li%s><a data-toggle="pill" href="#%s">%s</a></li>\n'%(lic,wl.tag,wl.label)
        s += '    <li><a data-toggle="pill" href="#%s">%s</a></li>\n'%(wl.tag,wl.label)
    return s

def mrf_html(weblets):
    s = ""
    for wa in list(weblets.keys()):
        wl = weblets[wa]
        s += wl.html()
    return s

def mrf_weblet_table(weblets):
    s = "// namespace table for weblets\n"
    s += "var _weblet_table = { "
    first = True
    for wa in list(weblets.keys()):
        wl = weblets[wa]
        if not first:
            s += ', '
        first = False
        s += "'"+wa+"' : mrf_weblet_"+wa
    s += ' } ;\n'
    return s



def mrf_page(rh,sob,ws_url, ip,html_body):
    mrflog.info("mrf_page: sob = "+str(sob))
    # they're in - they'll be served with a page with websocket url
    # prepared for them when they authenticated.....
    host = install.host
    rh.add_header("Expires",0)
    if install.domain:
        host += "."+install.domain


    rh.write(mrf_tp.generate(ws_url = ws_url, sob = sob,  html_body=html_body))

def request_ip(rh):
        rs =  rh._request_summary()
        mrflog.debug("req_sum:"+rs)
        regx = r'^GET /ws\?Id=%s \(([^\(]+)\)'%rh.id
        #print "rs:"+rs
        #print "regx:"+regx
        mob = re.match(regx,rs)
        if mob:
            ip = mob.group(1)
            mrflog.info("client ip="+ip)
        else:
            ip = 'none'
            mrflog.info("client ip not found!")
            allow = False

        # handle nginx proxying
        return
        if 'X-Forwarded-For' in rh.request.headers:
            ip = rh.request.headers['X-Forwarded-For']



def login_fail(rh,error):
    result = { 'result' : 'fail',
               'error' : error,
               'redirect' : '/login'
               }
    rh.write(mrfland.to_json(result))

def post_login(rh):
    mrflog.info('post_login : args')
    mrflog.info(rh.request.arguments)
    mrflog.info(rh.request)

    """
    if rh.request.headers.has_key('X-Real-Ip'):
        ip = rh.request.headers['X-Real-Ip']
    elif  rh.request.headers['X-Forwarded-For']:
        ip = rh.request.headers['X-Forwarded-For']
    else:
    """
    ip = rh.ip #rh.request.remote_ip

    if not 'username' in list(rh.request.arguments.keys()):
        return login_fail(rh,'invalid post data')

    username = rh.request.arguments['username']
    mrflog.warn('username : '+str(username) + " type : "+str(type(username)))

    if type(username) != type([]):
        return login_fail(rh,'invalid post data')
    if len(username) != 1:
        return login_fail(rh,'invalid post data')

    username = username[0]


    if not 'password' in list(rh.request.arguments.keys()):
        return login_fail(rh,'invalid post data')

    password = rh.request.arguments['password']
    mrflog.info('password : '+str(password) + " type : "+str(type(password)))

    if type(password) != type([]):
        return login_fail(rh,'invalid post data')
    if len(password) != 1:
        return login_fail(rh,'invalid post data')

    password = password[0]

    mrflog.warn('have username : '+str(username) + ' password : '+str(password)+" ip :"+ ip  )

    authres = rh.rm.authenticate(username,password,ip,rh.request_host)
    mrflog.warn('authenticate result = '+str(authres) )

    if authres == None:
        return login_fail(rh,'invalid username or password')

    ## user is authenticated
    secure_url =  '/'
    mrflog.warn('staff authenticated , id = '+str(authres) + " url = "+secure_url )

    ## set session id
    rh.set_secure_cookie(install.sess_cookie,authres['sessid'])
    mrflog.warn('staff authenticated , id = '+str(authres) + " url = "+secure_url )

    #rh.set_secure_cookie(install.sess_cookie,authres['sessid'])


    result = { 'result' : 'success',
               'data'   : authres,
               'redirect' : secure_url
               }


    wdat = mrfland.to_json(result)

    mrflog.warn("writing result "+wdat)


    rh.write(mrfland.to_json(result))

    ro = mrfland.RetObj()
    ro.b(mrfland.staff_info())
    rh.rm.comm.comm(None,ro)


class mainapp(tornado.web.RequestHandler):
    def __init__(self,*args, **kwargs):
        self.prox_re = re.compile(r'^[^\/]+\/\/([^\/]*)')
        tornado.web.RequestHandler.__init__(self,*args, **kwargs)
        mrflog.warn("mainapp init kwargs %s"%repr(kwargs))
        self.rm = kwargs['rm']
        #self.log = log
    def initialize(self, rm):
        self.rm = rm   # desperate times

    def set_req_host(self):
        # handle nginx proxying
        mrflog.warn("req headers "+repr(self.request.headers))
        if 'X-Real-Ip' in  list(self.request.headers.keys()):
            self.ip = self.request.headers['X-Real-Ip']

        elif 'X-Forwarded-For' in list(self.request.headers.keys()):
            self.ip = self.request.headers['X-Forwarded-For']

            #self.request_host = self.request.headers['Host']+":"+self.request.headers['Port']
        else:
            self.ip = self.request.remote_ip
        self.request_host = self.request.headers['Host']

        ipaddr = ipaddress.ip_address(self.ip)

        if ipaddr.is_loopback:
            self.localreq = True
        elif ipaddr in oursubnet:
            self.localreq = True
        else:
            self.localreq = False


    def post(self, *args, **kwargs):
        mrflog.info('post:'+str(self.request))
        mrflog.warn("mainapp post kwargs %s"%repr(kwargs))
        self.set_req_host()

        mrflog.info("uri : "+self.request.uri)
        uri = remarg.sub('',self.request.uri)
        mrflog.info("uri : "+uri)
        reqa = uri.split('/')[1:]
        page = reqa[1]
        mrflog.info("page = "+page)
        if page == 'login':
            post_login(self)



    def get(self, *args, **kwargs):
        mrflog.warn('get req:'+str(self.request))
        mrflog.warn("uri : "+self.request.uri)
        mrflog.info("host : "+self.request.host)

        uri = remarg.sub('',self.request.uri)
        mrflog.warn("get headers is %s"%repr(list(self.request.headers.keys())))

        for hn in ['Host','Port','X-Forwarded-For','Referer','X-Real-Ip']:
            if hn in list(self.request.headers.keys()):
                mrflog.warn("%s %s"%(hn,repr(self.request.headers[hn])))


        self.set_req_host()
        ip = self.ip

        mrflog.warn("got request_host "+self.request_host+" from ip: "+ip+" uri : "+uri)



        reqa = uri.split('/')[1:]

        mrflog.info("reqa = "+str(reqa)+ " len "+str(len(reqa)))
        page = reqa[0]
        mrflog.info("page = "+page)

        if len(reqa) > 1:
            action = reqa[1]
        else:
            action = None

        static_cdn = (self.localreq==False)  # dish out TP statics from CDNs if not local network request

        mrflog.warn("localreq = %s : static_cdn = %s"%(repr(self.localreq),repr(static_cdn)))

        sessid = self.get_secure_cookie(install.sess_cookie).decode('utf-8')
        mrflog.warn("page = "+page+"  action = "+str(action)+" sessid = "+str(sessid))

        if sessid == None:
            if page == 'login':
                mrflog.info("returning login page as requested")
                return login_page(self,
                                  css_html=self.rm.tp_static_mgr.html(login=True,css=True,static_cdn=static_cdn),
                                  js_html=self.rm.tp_static_mgr.html(login=True,static_cdn=static_cdn),
                )
            else:
                mrflog.warn("no sessid set - redirecting to login page")
                return self.redirect('/login')


        sob = self.rm.comm.session_isvalid(sessid)  # if session is valid you're in
        if sob == None :
            mrflog.warn("session is invalid %s - giving them login page again"%sessid)
            if page == 'login':
                return login_page(self,
                                  css_html=self.rm.tp_static_mgr.html(login=True,css=True,static_cdn=static_cdn),
                                  js_html=self.rm.tp_static_mgr.html(login=True,static_cdn=static_cdn),
                )


            else:
                return self.redirect('/login')

        if  ( page == 'logout') :
            mrflog.info("calling logout_action")
            wsid = sob['wsid']
            self.rm.comm.staff_logout(sob,ip)
            self.clear_cookie(install.sess_cookie)

            self.redirect('/login')
            return

        html_body = self.rm.html_body(sob['apps'],static_cdn=static_cdn)
        #static_thirdparty_html = self.rm.tp_static_mgr.local_html()
        ws_url = self.rm.ws_url(sob['wsid'],sob['req_host'])
        return mrf_page(self,sob,ws_url,ip,html_body)




#static_handler = tornado.web.StaticFileHandler
