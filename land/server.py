#! /usr/bin/env python

import tornado.ioloop
import tornado.web
import tornado.websocket
import tornado.httpserver
from tornado.options import define, options, parse_command_line
import logging
import re
import sys
sys.path.append('../lib')
import install
import psycopg2
import os
from datetime import datetime
import time
import json
import signal
import Queue

from mrflog import mrflog_init
from mainapp import mainapp
from public import publicapp
from pubsock import PubSocketHandler

from mrfland_state import MrflandState
from mrf_structs import *
from mrfland_app_heating import MrflandAppHeating

import mrfland

clients = dict()
alog = mrflog_init()

dt_handler = lambda obj: (
    obj.isoformat()
    if isinstance(obj, datetime)
    or isinstance(obj, date)
    else None)


def to_json(obj):
    return json.dumps(obj,default = dt_handler)




def broadcast(obj):
    msg = to_json(obj)
    _jso_broadcast(msg)

def _jso_broadcast(raw):
    alog.debug( "_jso_broadcast : "+str(len(clients))+" clients")
    alog.debug(" clients = "+str(clients))
    #alog.debug("raw:"+raw);
    for client in clients :   
        alog.debug( "client:"+client)
        clients[client]['object'].write_message(raw + "\r\n\r\n")


def send_object_to_client(id,obj):
    username = clients[id]['username']
    alog.debug( "send_object_to_client: "+username+" sent cmd "+str(obj['cmd']))
    msg = to_json(obj)
    clients[id]['object'].write_message(msg+"\r\n\r\n")

#conn = Connection(host='localhost')
#conn.connect(on_connect)



def comm(id,ro,touch = False):
    return mrfland.comm.comm(id,ro)

 

#define("port", default=2201, help="run on the given port", type=int)
define("port", default=install.port, help="run on the given port", type=int)

def cmd_nobbit():
    print "nobbit"

dcmds = {
    'nobbit': cmd_nobbit,

    }


def action_client_cmd(id,username,msgob):
    if not msgob.has_key('ccmd'):
        errmsg = "socket id "+id+" assigned to "+username+" sent invalid packet:"+str(msgob)
        return

    cmd = msgob['ccmd']
    if dcmds.has_key(cmd):
        alog.debug(cmd+":evaluating action")
        #ro = eval(ccmds[cmd])
        try:
            msgob['data']['ssid'] = id
            ro = dcmds[cmd](aconn,mrfland.comm.clients[id]['sid'],msgob['data'] )
        except Exception,e: 
            import traceback
            details = traceback.format_exc()

            errmsg = "despatcher exception : socket id "+id+" assigned to "+username+" obj :"+str(msgob)+", exception:"+str(e)+"\n+Details:\n"+details
            alog.error(errmsg)
            #mrfland.asa_error(aconn,'py','general',errmsg)
            return

        if asa_prims.is_asa_ret_obj(ro):
            mrfland.comm.comm(id,ro)
        else:
            errmsg = "no retobj returned from command"+cmd
            alog.error(errmsg)
            #mrfland.asa_error(aconn,'py','general',errmsg)
    else:
        errmsg = "socket id "+id+" assigned to "+username+" sent unrecognised command :"+str(cmd)
        alog.error(errmsg)
        #mrfland.asa_error(aconn,'py','general',errmsg)
        return
      
    return

def json_parse(str):
    try:
        ob = json.loads(str)        
    except:
        return None
    return ob

class IndexHandler(tornado.web.RequestHandler):
    @tornado.web.asynchronous
    def get(self):
        #self.write("This is your response")
        self.render("index.html")
        #we don't need self.finish() because self.render() is fallowed by self.finish() inside tornado
        #self.finish()

class WebSocketHandler(tornado.websocket.WebSocketHandler):
    def open(self, *args):        
        self.id = self.get_argument("Id")
        alog.info("client attempt to open ws:Id="+self.id)
        #alog.info("here comes websockhandler self")
        #print str(dir(self))
        #check id has been issued
        alog.info("headers:"+str(self.request.headers))
        #alog.debug("staff_type = "+str(stype))

        rs =  self._request_summary()
        alog.debug("req_sum:"+rs)
        regx = r'^GET /ws\?Id=%s \(([^\(]+)\)'%self.id
        #print "rs:"+rs
        #print "regx:"+regx
        mob = re.match(regx,rs)
        if mob:
            ip = mob.group(1)
            alog.debug("client ip="+ip)
            sob =  mrfland.comm.check_socket(self.id,ip)
        else:
            ip = 'none'
            alog.debug("client ip not found!")
            return
            
        
        cdata = {"id": self.id,
                 "sid":sob.sid,
                 "ip":sob.ip,
                 "username": sob.username,
                 "object": self 
                 }
        mrfland.comm.add_client(self.id,cdata)

        ro = mrfland.RetObj()
        ro.b(mrfland.atime())
        
        mrfland.comm.comm(self.id,ro)


    def on_message(self, message):
        alog.info("client message on wsid ="+self.id+" "+str(message))

        return
    
    def on_close(self):
        alog.info("ws handler close")
        return
class NoCacheStaticFileHandler(tornado.web.StaticFileHandler):
    def set_extra_headers(self, path):
        self.set_header("Cache-control", "no-cache")



class AuthStaticFileHandler(tornado.web.StaticFileHandler):
    def get_current_user(self):
        return self.get_secure_cookie(install.sess_cookie)  
    def get(self, path):
        alog.info("AuthStaticFileHandler.get: path = "+str(path))
        if self.current_user:
            alog.info("authorised : returning static content");
            tornado.web.StaticFileHandler.get(self, path) 
        else:
            alog.info("unauthorised")
            self.send_error(403)
            return None


class MrflandServer(object):

    def __init__(self,log,apps={}):
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            self.log.warn( "CTRL-C pressed , quitting")
            sys.exit(0)
        self.log = log
        self.apps = apps
        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)

        self._start_mrfnet(apps=apps)

        self._start_webapp()
        ct = time.time() + 5  # start tick
        self.ioloop = tornado.ioloop.IOLoop.instance()
        self.ioloop.add_timeout(ct,self.time_tick)
        self.ioloop.add_handler(self.rfd,self._resp_handler,self.ioloop.READ)
        self.ioloop.add_handler(self.sfd,self._struct_handler,self.ioloop.READ)
        
        tornado.ioloop.IOLoop.instance().start()
        print "do we ever get here?"
        
    def _start_mrfnet (self, stub_out_resp_path='/tmp/mrf_bus/0-app-out',stub_out_str_path='/tmp/mrf_bus/0-app-str',apps = {}, netid = 0x25):
        self.hostaddr = 1
        self.netid = netid
        self.log.info("apps are %s"%repr(apps))
        papps = {}
        for appn in apps.keys():
            papps[appn] = apps[appn](log=self.log)
            #papps[appn].setlog(self.log)
        self.apps = papps

        self.registrations = {}
        self.registrations["main"] = []
        for appn in apps.keys():
            self.registrations[appn] = []
    
        self.q = Queue.Queue()
        self.active_cmd = None
        self.active_timer = 0

        self.state = MrflandState(self)
        self.stub_out_resp_path = stub_out_resp_path
        self.stub_out_str_path = stub_out_str_path

        self._connect_to_mrfnet()

    def parse_input(self,resp):
        """
        check for valid looking header and types
        if found returns
           hdrpacket  , resppacket,  databuff
        else returns
           None
        """
    
        #print "parse_input len is %d"%len(resp)
        hdr = PktHeader()

        if len(resp) < len(hdr):  # no way valid
            return None,None,None


        hdr.load(bytes(resp)[0:len(hdr)])
        #print "hdr is:\n%s\n"%repr(hdr)

        if hdr.netid != self.netid: # only looking for packets from this netid
            self.log.info("not our netid")
            return None,None,None
        
        if hdr.udest != 0: # only looking for packets destined for us
            self.log.info("not our us")
            return None,None,None

        
        if not (hdr.type == mrf_cmd_usr_resp or hdr.type == mrf_cmd_usr_struct):
            self.log.info("not resp or struct")
            return None,None,None

        
        param = MrfSysCmds[hdr.type]['param']()
        #print "have param type %s"%type(param)
        param_data = bytes(resp)[len(hdr):len(hdr)+len(param)]
        param.load(param_data)
        #print "resp should be %s"%repr(param)
        respdat = bytes(resp)[len(hdr)+len(param):]
        
        return hdr , param , respdat

    def handle_response(self,hdr,rsp, rdata):
        # test if sys command response or data
        sysobj = mrf_decode_buff(rsp.type,rdata)

        #if sysobj:            
        rv = self.state.fyi(hdr,rsp, sysobj)  # state sees everything
        if rv:
            self.log.warn("we have response from main app fyi %s"%repr(rv))
            ro = mrfland.RetObj()
            for mcmd in rv.keys():
                ro.b(mrfland.mrf_cmd(mcmd,rv[mcmd]))
            mrfland.comm.comm(None,ro)
            for fd in self.registrations["main"]:  # send to registered clients
                c = self.conns[fd]
                c.send(rv)
        for appn in self.apps.keys(): # apps see everything
            rv = self.apps[appn].fyi(hdr,rsp, sysobj, rdata)
            if rv:
                self.log.info("app %s fyi returned"%appn)
                self.log.info(repr(rv))
                
                ro = mrfland.RetObj()
                for mcmd in rv.keys():
                     ro.b(mrfland.mrf_cmd(mcmd,rv[mcmd]))
                mrfland.comm.comm(None,ro)
                
                for fd in self.registrations[appn]:  # send to registered clients
                    c = self.conns[fd]
                    c.send(rv)
                
        # check response is for active_cmd
        if self.active_cmd and hdr.usrc == self.active_cmd[1]:  # FIXME - make queue items a bit nicer
            self.active_cmd = None
    def _resp_handler(self,*args, **kwargs):
        self.log.debug("Input on response pipe")
        resp = os.read(self.rfd, MRFBUFFLEN)

        hdr , rsp, rdata = self.parse_input(resp)

        if hdr:                    
            self.log.debug("received object %s response from  %d"%(type(rsp),hdr.usrc))
            self.handle_response(hdr, rsp , rdata)

    def _struct_handler(self,*args, **kwargs):
        self.log.debug("Input on data pipe")
        resp = os.read(self.sfd, MRFBUFFLEN)

        hdr , rsp, rdata = self.parse_input(resp)

        if hdr:                    
            self.log.debug("received object %s response from  %d"%(type(rsp),hdr.usrc))
            self.handle_response(hdr, rsp , rdata)
            
    def _connect_to_mrfnet(self):
        self.rfd =  os.open(self.stub_out_resp_path, os.O_RDONLY | os.O_NONBLOCK)
        if self.rfd == -1:
            self._error_exit("could not open response fifo %s"%self.stub_out_resp_path)
        else:
            self.log.info("opened response fifo %d"%self.rfd)

        self.sfd =  os.open(self.stub_out_str_path, os.O_RDONLY | os.O_NONBLOCK)
        if self.sfd == -1:
            self._error_exit("could not open structure fifo %s"%self.stub_out_str_path)
        else:
            self.log.info("opened data fifo %d"%self.sfd)


        self.log.info( "opened pipes, trying to open pipe to stub")
        self.app_fifo = open("/tmp/mrf_bus/0-app-in","w+")
        self.log.info( "app_fifo opened %d"%self.app_fifo.fileno())
        
    def _start_webapp(self):        
        
        self._web_static_handler = NoCacheStaticFileHandler

        self._web_handlers = [(r'/(favicon.ico)', self._web_static_handler, {'path': 'public/css/asa/images'}),
                         (r'/static/public/(.*)', self._web_static_handler, {'path': 'static/public'}),
                         (r'/ws', WebSocketHandler), 
                         (r'/pws', PubSocketHandler), 
                         (r'/static/secure/(.*)',AuthStaticFileHandler , {'path': 'static/secure'}),
                         (r'((/)([^/?]*)(.*))', mainapp)
        ]

        self._webapp = tornado.web.Application(self._web_handlers,cookie_secret="dighobalanamsamarosaddhammamavijanatam")

        
        self.nsettings = dict(
        )
        """
        self.nsettings = dict({  "ssl_options" : 
                           { "certfile": install._ssl_cert_file,
                     "keyfile": install._ssl_key_file
                   }
        }
        )
        """


        self.http_server = tornado.httpserver.HTTPServer(self._webapp, **self.nsettings)

        self.http_server.listen(options.port)

        

    def time_tick(self):
        ct = time.time() + 5
        tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)
        alog.debug("time_tick")

alog.info('Application started')
if __name__ == '__main__':
    parse_command_line()
    alog.info("Mrfland web server starting on port "+str(options.port))
    

    ml =  MrflandServer(alog,apps = {'heating' : MrflandAppHeating })


