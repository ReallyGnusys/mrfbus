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

#redemohost = re.compile(r'^'+install.hostdomain+':'+str(install.port)+'$')

login_tp = tornado.template.Template(templates.login_tp)
asa_tp = tornado.template.Template(templates.asa_tp)

def login_page(rh):
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
            

def mrf_page(rh,sob,ip):
    alog.info("mrf_page: sob = "+str(sob))
    # they're in - they'll be served with a page with websocket url
    # prepared for them when they authenticated.....
    #pjs = _expand_priv_js(sob['type'])
    #alog.info("pjs:"+pjs)
    rh.write(mrf_tp.generate(ws_url = mrfland.ws_url(sob['wsid']), sob = sob,email="email.tbd",sms='number.tbd',webchat='webchat.tbd'))

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

   
  
    
    result = { 'result' : 'success',
               'data' : authres,
               'redirect' : secure_url
               }    
    rh.write(mrfland.to_json(result))


    ro = mrfland.RetObj()
    ro.b(mrfland.staff_info())
    mrfland.comm.comm(None,ro)



    


class mainapp(tornado.web.RequestHandler):
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
                return login_page(self)  
            else:
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



        return mrf_page(self,sob,ip)
     
        
        

#static_handler = tornado.web.StaticFileHandler
