import tornado.web
import tornado.template
import re
import install
import os
import base64
import sys
sys.path.append('../lib')
from mrflog import mrf_log
import templates
import psycopg2
import mrfland
alog = mrf_log()

def print_everything(*args):
    alog.debug( "print *args")
    for count, thing in enumerate(args):
        alog.debug( '{0}. {1}'.format(count, thing))
def print_kwargs(**kwargs):
    alog.debug("print_kwargs")
    for name, value in kwargs.items():
        alog.debug('{0} = {1}'.format(name, value))


conn = None

remarg = re.compile(r'[?].*$')

login_tp = tornado.template.Template(templates.login_tp)
mrf_tp = tornado.template.Template(templates.mrf_tp)

def login_page(rh):
    rh.add_header("Expires",0)
    
    rh.write(login_tp.generate())
    #rh.write("Login page")

def logout_action(rh,sob,ip):

    ro = mrfland.staff_logout(sob['sid'],sob['username'],ip)
    rh.clear_cookie(install.sess_cookie)
    
    rh.redirect('/login')


_priv_js = { 
    'system' :  ['asa_admin_staff_on.js','asa_admin_case_on.js','asa_admin_settings_on.js'],
    'sysadmin' :  ['asa_admin_staff_on.js','asa_admin_case_on.js','asa_admin_settings_on.js'],
    'admin' :  ['asa_admin_staff_on.js','asa_admin_case_on.js','asa_admin_settings_on.js'],
    'supervisor' :  ['asa_admin_staff_off.js','asa_admin_case_on.js'],
    'staff' :  ['asa_admin_staff_off.js','asa_admin_case_off.js'],
    }

def _expand_priv_js(stype):
    ht = ''
    if _priv_js.has_key(stype):
        for js in _priv_js[stype] :
            ht += '<script type="text/javascript" src="/static/secure/js/'+js+'"></script>\n'
    return ht
            

def mrf_pills(weblets):
    s = ""
    first = True
    for wa in weblets.keys():
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
    for wa in weblets.keys():
        wl = weblets[wa]
        s += wl.html()
    return s

"""
Aiming to retire this - .js() method removed from apps now.
 Don't want apps writers to bother with JS
def mrf_js(weblets):
    s = ""
    for wa in weblets.keys():
        wl = weblets[wa]
        s += wl.js()
    return s
"""

def mrf_weblet_table(weblets):
    s = "// namespace table for weblets\n"
    s += "var _weblet_table = { "
    first = True
    for wa in weblets.keys():
        wl = weblets[wa]
        if not first:
            s += ', '
        first = False
        s += "'"+wa+"' : mrf_weblet_"+wa
    s += ' } ;\n'
    return s



def mrf_page(rh,sob,ip,wapps,sens_avg_js):
    alog.info("mrf_page: sob = "+str(sob))
    # they're in - they'll be served with a page with websocket url
    # prepared for them when they authenticated.....
    #pjs = _expand_priv_js(sob['type'])
    #alog.info("pjs:"+pjs)
    host = install.host
    rh.add_header("Expires",0)
    if install.domain:
        host += "."+install.domain

    upsince_str = install.upsince.strftime("%c")
    pills = mrf_pills(wapps)
    apphtml = mrf_html(wapps)
    apphtml += """
<script type="text/javascript">
%s
</script>
    """%sens_avg_js



    rh.write(mrf_tp.generate(ws_url = mrfland.ws_url(sob['wsid']), sob = sob,  pills = pills, apphtml = apphtml,  host = host, upsince = upsince_str))

def request_ip(rh):
        rs =  rh._request_summary()
        alog.debug("req_sum:"+rs)
        regx = r'^GET /ws\?Id=%s \(([^\(]+)\)'%rh.id
        #print "rs:"+rs
        #print "regx:"+regx
        mob = re.match(regx,rs)
        if mob:
            ip = mob.group(1)
            alog.info("client ip="+ip)
        else:
            ip = 'none'
            alog.info("client ip not found!")
            allow = False

        # handle nginx proxying
        return
        if rh.request.headers.has_key('X-Forwarded-For'):
            ip = rh.request.headers['X-Forwarded-For']
        
    

def login_fail(rh,error):
    result = { 'result' : 'fail',
               'error' : error,
               'redirect' : '/login'
               }    
    rh.write(mrfland.to_json(result))

def post_login(rh):
    alog.info('post_login : args')
    alog.info(rh.request.arguments)
    alog.info(rh.request)

    """
    if rh.request.headers.has_key('X-Real-Ip'):
        ip = rh.request.headers['X-Real-Ip']
    elif  rh.request.headers['X-Forwarded-For']:
        ip = rh.request.headers['X-Forwarded-For']
    else:
    """
    ip = rh.request.remote_ip

    if not 'username' in rh.request.arguments.keys():   
        return login_fail(rh,'invalid post data')
      
    username = rh.request.arguments['username']
    alog.info('username : '+str(username) + " type : "+str(type(username)))
    
    if type(username) != type([]):
        return login_fail(rh,'invalid post data')    
    if len(username) != 1:
        return login_fail(rh,'invalid post data')
 
    username = username[0]
    

    if not 'password' in rh.request.arguments.keys():   
        return login_fail(rh,'invalid post data')
      
    password = rh.request.arguments['password']
    alog.info('password : '+str(password) + " type : "+str(type(password)))
    
    if type(password) != type([]):
        return login_fail(rh,'invalid post data')    
    if len(password) != 1:
        return login_fail(rh,'invalid post data')
 
    password = password[0]

    alog.info('have username : '+str(username) + ' password : '+str(password)+" ip :"+ ip  )
 
    authres = mrfland.authenticate(username,password,ip)
    alog.info('authenticate result = '+str(authres) )

    if authres == None:
        return login_fail(rh,'invalid username or password')
    
    ## user is authenticated
    secure_url =  '/'
    alog.info('staff authenticated , id = '+str(authres) + " url = "+secure_url )
    
    ## set session id
    rh.set_secure_cookie(install.sess_cookie,authres['sessid'])

    #rh.set_secure_cookie(install.sess_cookie,authres['sessid'])

   
  
    
    result = { 'result' : 'success',
               'data' : authres,
               'redirect' : secure_url
               }    
    rh.write(mrfland.to_json(result))


    ro = mrfland.RetObj()
    ro.b(mrfland.staff_info())
    mrfland.comm.comm(None,ro)



    


class mainapp(tornado.web.RequestHandler):
    def __init__(self,*args, **kwargs):
        tornado.web.RequestHandler.__init__(self,*args, **kwargs)
        #self.log = log
    def initialize(self, mserv):
        self.mserv = mserv   # desperate times
    def post(self, *args, **kwargs):
        alog.info('post:'+str(self.request))
        alog.info("uri : "+self.request.uri)
        uri = remarg.sub('',self.request.uri)
        alog.info("uri : "+uri)
        reqa = uri.split('/')[1:]
        page = reqa[1]
        alog.info("page = "+page)
        if page == 'login':
            post_login(self)


        
    def get(self, *args, **kwargs):
        alog.info('get:'+str(self.request))
        alog.info("uri : "+self.request.uri)
        alog.info("host : "+self.request.host)
  
        uri = remarg.sub('',self.request.uri)
        ip = self.request.remote_ip

        # handle nginx proxying
        #if self.request.headers.has_key('X-Forwarded-For'):
        #    ip = self.request.headers['X-Forwarded-For']
        alog.info("ip: "+ip+" uri : "+uri)
        reqa = uri.split('/')[1:]
        
        alog.info("reqa = "+str(reqa)+ " len "+str(len(reqa)))
        page = reqa[0]
        alog.info("page = "+page)
 
        if len(reqa) > 1:
            action = reqa[1]
        else:
            action = None            
   
        sessid = self.get_secure_cookie(install.sess_cookie)        
        alog.info("page = "+page+"  action = "+str(action)+" sessid = "+str(sessid))
        if sessid == None:
            if page == 'login':
                alog.info("returning login page as requested")
                return login_page(self)  
            else:
                alog.info("no sessid - redirecting to login page")
                return self.redirect('/login')
  

        sob = mrfland.comm.session_isvalid(sessid)  # if session is valid you're in
        if sob == None :
            alog.info("session is invalid")
            if page == 'login':
                return login_page(self)  
            else:
                return self.redirect('/login')

        if  ( page == 'logout') :
            alog.info("calling logout_action")
            return logout_action(self,sob,ip)

        wapps = self.mserv.rm.weblets 
        sens_avg_js = self.mserv.rm.sensor_average_js()


        return mrf_page(self,sob,ip,wapps,sens_avg_js)
     
        
        

#static_handler = tornado.web.StaticFileHandler
