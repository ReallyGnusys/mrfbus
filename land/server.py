#! /usr/bin/env python

import tornado.ioloop
import tornado.web
import tornado.websocket
import tornado.httpserver
import tornado.tcpserver
from tornado.options import define, options, parse_command_line
import socket
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
        alog.error(errmsg)
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


server = None  # FIXME this is shonky just to get to our own methods added to  server from handlers

class IndexHandler(tornado.web.RequestHandler):
    @tornado.web.asynchronous
    def get(self):
        self.render("index.html")

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

        ro = server.welcomepack()  # FIXME little ugly
        
        mrfland.comm.comm(self.id,ro)

        
    def on_message(self, message):
        alog.info("client message on wsid ="+self.id+" "+str(message))

        return
    
    def on_close(self):
        self.id = self.get_argument("Id")
        alog.info("*************************")
        alog.info("client closed ws:Id="+self.id)
        alog.info("ws handler close")
        alog.info("*************************")
        mrfland.comm.del_client(self.id)
    
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

class SimpleTcpClient(object):
    client_id = 0
 
    def __init__(self, stream,log, callback):
        #super(type(self)).__init__(self)
        SimpleTcpClient.client_id += 1
        self.log = log
        self.callback = callback
        self.id = SimpleTcpClient.client_id
        #print "SimpleTcpClient constuctor %d"%self.id
        self.log.info("SimpleTcpClient constuctor %d"%self.id)
        self.stream = stream
 
        self.stream.socket.setsockopt(
            socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.stream.socket.setsockopt(
            socket.IPPROTO_TCP, socket.SO_KEEPALIVE, 1)
        self.stream.set_close_callback(self.on_disconnect)
 
 
    @tornado.gen.coroutine
    def on_disconnect(self):
        self.log.info("tcp conn %d disconnected"%self.id)
        yield []
 
    @tornado.gen.coroutine
    def dispatch_client(self):
        try:
            while True:
                line = yield self.stream.read_until(b'\n')
                self.log.info("got line %s"%line)
                #s = line.decode('utf-8').strip()
                #self.log('got |%s|' % repr(s))
                ob = json_parse(line)
                if ob:
                    data = None
                    self.log.info("json ob : %s"%repr(ob))
                    if ob.has_key('dest') and ob.has_key('cmd'):
                        if ob.has_key('data') and ob['cmd'] in MrfSysCmds:
                            if MrfSysCmds[ob['cmd']]['param'] != None:
                                data = MrfSysCmds[ob['cmd']]['param']()
                                data.dic_set(ob['data'])
                            
                        self.callback(self.id, ob['dest'],ob['cmd'],data)
                    else:
                        self.log.info("no valid cmd decoded in  %s"%repr(line))
                else:
                    self.log.warn("no json ob decoded in %s"%repr(line))
                yield self.stream.write("\n")
        except tornado.iostream.StreamClosedError:
            pass
 
    @tornado.gen.coroutine
    def on_connect(self):
        raddr = 'closed'
        try:
            raddr = '%s:%d' % self.stream.socket.getpeername()
        except Exception:
            pass
        self.log.info('new, %s' % raddr)
 
        yield self.dispatch_client()
 
    #def log(self, msg, *args, **kwargs):
    #    print('[connection %d] %s' % (self.id, msg.format(*args, **kwargs)))
 

# little nicer to reference if we queue Cmds as objects    
class MrfCmdObj(object):
    def __init__(self, tag, dest, cmd_code,dstruct=None):
        self.tag = tag
        self.dest = dest
        self.cmd_code = cmd_code
        self.dstruct = dstruct
    def __repr__(self):
        s = "%s: tag %s dest %d cmd_code %d dstruct %s"%\
            (self.__class__.__name__,self.tag,self.dest,self.cmd_code,repr(self.dstruct))
        return s


class MrfTcpServer(tornado.tcpserver.TCPServer):
    def __init__(self,log,callback):
        tornado.tcpserver.TCPServer.__init__(self)
        self.log = log
        self.callback = callback
        self.clients = {}
    def tag_is_tcp_client(self,tag):
        return self.clients.has_key(tag)
    
    def write_to_client(self,id , msg):
        if not self.clients.has_key(id):
            self.log.error("%s no client with id %d"%(self.__class__.__name__,id))
            return
        self.clients[id].stream.write(msg)
        self.log.info("wrote_to client %id"%id)
    @tornado.gen.coroutine
    def handle_stream(self, stream, address):
        """
        Called for each new connection, stream.socket is
        a reference to socket object
        """
        connection = SimpleTcpClient(stream,self.log, self.callback)
        self.clients[connection.id] = connection
        yield connection.on_connect()  

class MrflandServer(object):

    def __init__(self,log,apps={}):
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            self.log.warn( "CTRL-C pressed , quitting")
            sys.exit(0)
        global server
        self.log = log
        server = self  # FIXME ouch
        self.apps = apps
        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)
        self._active = False

        self._start_mrfnet(apps=apps)

        self._start_webapp()
        self._start_tcp_service()
        self.ioloop = tornado.ioloop.IOLoop.instance()
        self._deactivate()  # start quiet
        #ct = time.time() + 5  # start tick
        #self.ioloop.add_timeout(ct,self.time_tick)
        self.ioloop.add_handler(self.rfd,self._resp_handler,self.ioloop.READ)
        self.ioloop.add_handler(self.sfd,self._struct_handler,self.ioloop.READ)
        
        tornado.ioloop.IOLoop.instance().start()
        print "do we ever get here?"


    def _start_mrfnet (self, stub_out_resp_path='/tmp/mrf_bus/0-app-out',stub_out_str_path='/tmp/mrf_bus/0-app-str',apps = {}, netid = 0x25):
        self.hostaddr = 1
        self.netid = netid


        self.q = Queue.Queue()
        self.active_cmd = None
        self.active_timer = 0

        self.state = MrflandState(self)
        self.stub_out_resp_path = stub_out_resp_path
        self.stub_out_str_path = stub_out_str_path
        

        self._connect_to_mrfnet()


        self.log.info("apps are %s"%repr(apps))
        papps = {}
        for appn in apps.keys():
            papps[appn] = apps[appn]("app_"+appn,log=self.log,cmd_callback=self._callback)
            #papps[appn].setlog(self.log)
        self.apps = papps
        self.log.info("apps instanced are %s"%repr(apps))
        for appn in self.apps.keys():
            self.apps[appn].post_init()

        self.registrations = {}
        self.registrations["main"] = []
        for appn in self.apps.keys():
            self.registrations[appn] = []
    

        
    def _activate(self):  # ramp up tick while responses or txqueue outstanding
        if not self._active:
            self.log.warn("activating!")
            self.active_timer = 0
            self._active = True
            ct = time.time() + 0.02
            tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)
            
    def _deactivate(self):  # ramp down tick when quiet
        self.log.warn("deactivating!")
        self._active = False
        self.active_cmd = None
            #ct = time.time() + 5
            #tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)


    def queue_cmd(self,tag, dest, cmd_code,dstruct=None):
        cobj = MrfCmdObj(tag, dest, cmd_code,dstruct)
        if self.active_cmd:
            self.log.info("queuing cmd %s"%repr(cobj))
            self.q.put(cobj)
        else:
            self.log.info("running cmd %s"%repr(cobj))
            self._next_cmd(cobj)
            self._activate()

    def _next_cmd(self,cobj):
        self.resp_timer = 0
        self.active_cmd = cobj            
        self._run_cmd(cobj)


    def _app_cmd_set(self,dest):
        for appn in self.apps.keys():
            app = self.apps[appn]
            if self.apps[appn].i_manage(dest):
                return self.apps[appn].cmd_set(dest)
        
    def _run_cmd(self,cobj):
        self.log.debug("cmd : dest %d cmd_code %s"%(cobj.dest,repr(cobj.cmd_code)))
        if cobj.dest > 255:
            print "dest > 255"
            return -1

        appcmds = self._app_cmd_set(cobj.dest)
        if cobj.cmd_code in MrfSysCmds.keys():
            paramtype = MrfSysCmds[cobj.cmd_code]['param']
        elif (appcmds):
            paramtype = appcmds[cobj.cmd_code]['param']
            self.log.info("app supplied command set for dest %d param is %s"%(cobj.dest,repr(paramtype)))
            
        else:
            self.log.info("unrecognised cmd_code (01xs) %d"%cobj.cmd_code)
            return -1

        if type(cobj.dstruct) == type(None) and type(paramtype) != type(None):
            self.log.info("No param sent , expected %s"%type(paramtype))
            return -1
        
        if type(paramtype) == type(None) and type(cobj.dstruct) != type(None):
            self.log.info("Param sent ( type %s ) but None expected"%type(cobj.dstruct))
            return -1

        if type(cobj.dstruct) != type(None) and type(cobj.dstruct) != type(paramtype()):
            self.log.info("Param sent ( type %s ) but  %s expected"%(type(cobj.dstruct),type(paramtype())))
            return -1

        mlen = 4
        if cobj.dstruct:
            mlen += len(cobj.dstruct)
        if mlen > 64:
            print "mlen = %d"%mlen
            return -1
        msg = bytearray(mlen)
        msg[0] = mlen
        msg[1] = cobj.dest
        msg[2] = cobj.cmd_code
        
        n = 3
        if cobj.dstruct:
            dbytes = cobj.dstruct.dump()
            for b in dbytes:
                msg[ n ] =  b
                n += 1

        csum = 0
        for i in xrange(mlen -1):
            csum += int(msg[i])
        csum = csum % 256
        msg[mlen - 1] = csum
        self.log.info("msg len is %d"%len(msg))
        self._app_fifo_write(msg)
        return 0
    def _app_fifo_write(self,msg):
        self.app_fifo.write(msg)
        self.app_fifo.flush()
        #self.app_fifo.close()
        self.log.info("written to app_fifo")
        
    def welcomepack(self):  #  build welcome pack of objects from each app
        ro = mrfland.RetObj()
        
        for appn in self.apps.keys():
            ob = self.apps[appn].curr_state()
            self.log.info("welcomepack app %s curr_state %s"%(appn,ob))
            for cmdobj in ob:
                for mcmd in cmdobj.keys():
                    ro.a(mrfland.mrf_cmd(mcmd,cmdobj[mcmd]))

            return ro

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


        #self.log.info(" have response or struct object %s"%repr(param))


        resp = mrf_decode_buff(param.type,respdat)

        if not resp:
            appcmds = self._app_cmd_set(hdr.usrc)
            if appcmds:
                resp = mrf_decode_buff(param.type,respdat,cmdset=appcmds)
 
        
            
        #FIXME - this should be decoded here...somehow .. or passed to applications
        
        return hdr , param , resp 

    def handle_response(self,hdr,rsp, robj,response=False):
        # test if sys command response or data
        if response:
            self.log.info("handle_response hdr %s",repr(hdr))
        rv = self.state.fyi(hdr,rsp, robj)  # state sees everything
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
            rv = self.apps[appn].fyi(hdr,rsp, robj)
            if rv:
                #self.log.info("app %s fyi returned"%appn)
                #self.log.info(repr(rv))
                
                ro = mrfland.RetObj()
                for mcmd in rv.keys():
                     ro.b(mrfland.mrf_cmd(mcmd,rv[mcmd]))
                mrfland.comm.comm(None,ro)
                
                for fd in self.registrations[appn]:  # send to registered clients
                    c = self.conns[fd]
                    c.send(rv)
                
        # check response is for active_cmd
        if response and self.active_cmd and hdr.usrc == self.active_cmd.dest:  # FIXME - make queue items a bit nicer
            self.log.info("got response for active command %s"%repr(self.active_cmd))
            self.log.info("rsp %s"%(rsp))
            if self.tcp_server.tag_is_tcp_client(self.active_cmd.tag):
                self.log.info("response for tcp client %d"%self.active_cmd.tag)
                self.log.info("robj dic is %s"%repr(robj.dic()))
                rstr = mrfland.to_json(robj.dic())
                self.tcp_server.write_to_client(self.active_cmd.tag,rstr)

            self.active_cmd = None
                                
    def _resp_handler(self,*args, **kwargs):
        #self.log.info("Input on response pipe")
        resp = os.read(self.rfd, MRFBUFFLEN)

        hdr , rsp, robj  = self.parse_input(resp)

        if hdr:                    
            self.log.info("received object %s response from  %d"%(type(rsp),hdr.usrc))
            self.handle_response(hdr, rsp , robj, response=True )

    def _struct_handler(self,*args, **kwargs):
        self.log.debug("Input on data pipe")
        resp = os.read(self.sfd, MRFBUFFLEN)

        hdr , rsp, robj = self.parse_input(resp)

        if hdr:                    
            self.log.debug("received object %s response from  %d"%(type(rsp),hdr.usrc))
            self.handle_response(hdr, rsp , robj)
            
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

        self.log.info( "trying to open pipe to stub")
        self.app_fifo = open("/tmp/mrf_bus/0-app-in","w")
        self.log.info( "app_fifo opened %d"%self.app_fifo.fileno())

    def _callback(self, tag, dest, cmd,data=None):
        self.log.info("_callback tag %s dest %d cmd %s  data %s"%(tag,dest,cmd,repr(data)))
        self.queue_cmd(tag, dest, cmd,data)

    def _start_tcp_service(self):
        self.tcp_server = MrfTcpServer(log = self.log, callback = self._callback )
        self.tcp_server.listen(install.tcpport)
        self.log.info("started tcpserver on port %d"%install.tcpport)
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

        self.nsettings = dict()

        
        if install.https_server:
            self.nsettings["ssl_options"] = {
                "certfile": install._ssl_cert_file,
                "keyfile" : install._ssl_key_file
            }


            self.log.info("starting https server certfile %s keyfile %s"%\
                      (install._ssl_cert_file,install._ssl_key_file))
        else:
            self.log.info("starting http server")
                           
        self.http_server = tornado.httpserver.HTTPServer(self._webapp, **self.nsettings)

        self.http_server.listen(options.port)


    def _check_if_anything(self):
        if self.q.empty():
            self.log.info("_check_if_anything -queue empty")
            self._deactivate()
        else:
            self.log.info("_check_if_anything : running next command")
            self._next_cmd(self.q.get())
      

    def time_tick(self):
        
        if self._active:
            self.log.info("time_tick active");
            if self.active_cmd == None:
                self.log.info("tick can run another cmd")
            else:
                self.log.info("time_tick active resp_timer %d"%self.resp_timer)
                self.resp_timer += 1
                if self.resp_timer > 15:
                    self.log.info("give up waiting for response for %s"%( self.active_cmd))
                    if self.tcp_server.tag_is_tcp_client(self.active_cmd.tag):
                        self.log.info("response for tcp client %d"%self.active_cmd.tag)
                        self.log.info("sending empty dict")
                        rstr = mrfland.to_json({})
                        self.tcp_server.write_to_client(self.active_cmd.tag,rstr)
                    self.active_cmd = None

                    
            if self.active_cmd == None:
                self._check_if_anything()



        if self._active:
            ct = time.time() + 0.2
            tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)
        else:
            ct = time.time() + 5
            tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)
                    
            
alog.info('Application started')
if __name__ == '__main__':
    parse_command_line()
    alog.info("Mrfland web server starting on port "+str(options.port))
    

    ml =  MrflandServer(alog,apps = {'heating' : MrflandAppHeating })


