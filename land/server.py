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
import time
import json
import signal
import Queue
from collections import OrderedDict

from mrflog import mrf_log_init
from mainapp import mainapp
from public import publicapp
from pubsock import PubSocketHandler

from mrf_structs import *
import mrfland
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_heatbox import DevHeatbox
from mrfdev_host import MrfDevHost
from mrfland_weblet_temps import MrfLandWebletTemps
from mrfland_weblet_relays import MrfLandWebletRelays
from mrfland_weblet_timers import MrfLandWebletTimers



clients = dict()
alog = mrf_log_init()


def broadcast(obj):
    msg = mrfland.to_json(obj)
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
    msg = mrfland.to_json(obj)
    clients[id]['object'].write_message(msg+"\r\n\r\n")

#conn = Connection(host='localhost')
#conn.connect(on_connect)



def comm(id,ro,touch = False):
    return mrfland.comm.comm(id,ro)

 

#define("port", default=2201, help="run on the given port", type=int)
define("port", default=install.port, help="run on the given port", type=int)



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
            alog.info("client ip="+ip)
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

        

        
    def on_message(self, message):
        alog.info("client message on wsid ="+self.id+" "+str(message))
        cobj = mrfland.json_parse(message)

        if cobj and cobj.has_key("app") and cobj.has_key("ccmd"):
            alog.info("decoded client command %s"%repr(cobj))
            server.web_client_command(self.id,cobj["app"],cobj["ccmd"],cobj["data"])
    
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
 
    def __init__(self, stream,log, landserver,disconnect):
        #super(type(self)).__init__(self)
        SimpleTcpClient.client_id += 1
        self.log = log
        self.landserver = landserver
        self.disconnect = disconnect
        self.id = SimpleTcpClient.client_id
        #print "SimpleTcpClient constuctor %d"%self.id
        self.log.info("SimpleTcpClient constructor %d"%self.id)
        self.stream = stream
 
        self.stream.socket.setsockopt(
            socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.stream.socket.setsockopt(
            socket.IPPROTO_TCP, socket.SO_KEEPALIVE, 1)
        self.stream.set_close_callback(self.on_disconnect)
 
 
    @tornado.gen.coroutine
    def on_disconnect(self):
        self.log.info("tcp conn %d disconnected"%self.id)
        self.disconnect(self.id)
        yield []
 
    @tornado.gen.coroutine
    def dispatch_client(self):
        try:
            while True:
                line = yield self.stream.read_until(b'\n')
                self.log.info("got line %s"%line)
                #s = line.decode('utf-8').strip()
                #self.log('got |%s|' % repr(s))
                ob = mrfland.json_parse(line)
                if ob:
                    data = None
                    self.log.info("json ob : %s"%repr(ob))
                    if ob.has_key('dest') and ob.has_key('cmd'):
                        if ob.has_key('data'):
                            if ob['cmd'] in MrfSysCmds:
                                if MrfSysCmds[ob['cmd']]['param'] != None:
                                    data = MrfSysCmds[ob['cmd']]['param']()
                                    data.dic_set(ob['data'])
                            else:
                                appcmdset = self.landserver._app_cmd_set(ob['dest'])
                                if appcmdset:
                                    if appcmdset[ob['cmd']]['param'] != None:
                                        data = appcmdset[ob['cmd']]['param']()
                                        data.dic_set(ob['data'])
                                
                            
                        self.landserver._callback(self.id, ob['dest'],ob['cmd'],data)
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
    def __init__(self,log,landserver):
        tornado.tcpserver.TCPServer.__init__(self)
        self.log = log
        self.landserver = landserver
        self.clients = {}


    def client_disconnect(self,id):
        self.clients.pop(id)
        self.log.info("removed tcp client %s"%repr(id))
        self.log.info("tcp clients now %s"%repr(self.clients.keys()))
    def tag_is_tcp_client(self,tag):
        return self.clients.has_key(tag)
    
    def write_to_client(self,id , msg):
        if not self.clients.has_key(id):
            self.log.error("%s no client with id %d"%(self.__class__.__name__,id))
            return
        self.clients[id].stream.write(msg)
        self.log.info("wrote_to client %d"%id)
    @tornado.gen.coroutine
    def handle_stream(self, stream, address):
        """
        Called for each new connection, stream.socket is
        a reference to socket object
        """
        connection = SimpleTcpClient(stream,self.log, self.landserver,self.client_disconnect)
        self.clients[connection.id] = connection
        yield connection.on_connect()  

class MrflandServer(object):

    def __init__(self,rm,log):
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            self.log.warn( "CTRL-C pressed , quitting")
            sys.exit(0)
        global server
        self.log = log
        self.rm = rm
        self._timeout_handle = None
        server = self  # FIXME ouch
        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)
        self._active = False
            
        self._start_mrfnet()
        self.quiet_cnt = 0
        self._start_webapp()
        self._start_tcp_service()
        self.ioloop = tornado.ioloop.IOLoop.instance()
        self._deactivate()  # start quiet

        #ct = time.time() + 5  # start tick
        #self.ioloop.add_timeout(ct,self.time_tick)
        self.log.warn("adding self.rfd = %d to ioloop"%self.rfd)
        #self.ioloop.add_handler(self.rfd,self._resp_handler,self.ioloop.READ)
        self.ioloop.add_handler(self.nsock,self._resp_handler,self.ioloop.READ)
        #self.ioloop.add_handler(self.sfd,self._struct_handler,self.ioloop.READ)
        
        tornado.ioloop.IOLoop.instance().start()
        print "do we ever get here?"


    def _start_mrfnet (self,  netid = 0x25):
        self.hostaddr = 1
        self.netid = netid


        self.q = Queue.Queue()
        self.active_cmd = None
        self.active_timer = 0

        self._connect_to_mrfnet()





                

    
    def web_client_command(self,wsid,app,cmd,data):
        if app not in self.rm.weblets.keys():
            self.log.error("web_client_command unknown app %s from wsid %s"%(app,wsid))
            return
        self.rm.weblets[app].cmd(cmd,data)
        for dup in self.rm.dups:
            self.log.warn("%s calling _callback"%(self.__class__.__name__))
            self._callback(dup['tag'], dup['dest'] , dup['cmd'] , dup['data'])
        self.rm.dups = []

    def _set_timeout(self,s):
        if self._timeout_handle:
            tornado.ioloop.IOLoop.instance().remove_timeout(self._timeout_handle)
        ct = time.time() + s
        self._timeout_handle = tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)
          
    def _activate(self):  # ramp up tick while responses or txqueue outstanding
        if True or not self._active:
            self.log.warn("activating! qsize %s"%self.q.qsize())
            self.active_timer = 0
            self._active = True
            self._set_timeout(0.02)
            
    def _deactivate(self):  # ramp down tick when quiet
        self.log.warn("deactivating!")
        self._active = False
        self.active_cmd = None
        self._set_timeout(5.0)
            #ct = time.time() + 5
            #tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)


    def queue_cmd(self,tag, dest, cmd_code,dstruct=None):
        cobj = MrfCmdObj(tag, dest, cmd_code,dstruct)
        if self.active_cmd:
            self.log.warn("queuing cmd %s"%repr(cobj))
            self.q.put(cobj)
        else:
            self.log.warn("running cmd immediately %s"%repr(cobj))
            self._next_cmd(cobj)
            self._activate()

    def _next_cmd(self,cobj):
        if self._run_cmd(cobj) == 0:
            self.resp_timer = 0
            self.active_cmd = cobj            
        else:
            self.log.warn("_run_cmd failed for %s"%repr(cobj))

    def _app_cmd_set(self,dest):
        for appn in self.apps.keys():
            app = self.apps[appn]
            if self.apps[appn].i_manage(dest):
                return self.apps[appn].cmd_set(dest)

    def _dev_cmd_set(self,dest):
        if not self.rm.devmap.has_key(dest):
            self.log.error("no device %d found in rm"%dest)
            return None

        return self.rm.devmap[dest]._cmdset
    
    def _run_cmd(self,cobj):
        self.log.debug("cmd : dest %d cmd_code %s"%(cobj.dest,repr(cobj.cmd_code)))
        if cobj.dest > 255:
            print "dest > 255"
            return -1

        appcmds = self._dev_cmd_set(cobj.dest)
        if cobj.cmd_code in MrfSysCmds.keys():
            paramtype = MrfSysCmds[cobj.cmd_code]['param']
        elif (appcmds):
            paramtype = appcmds[cobj.cmd_code]['param']
            self.log.info("app supplied command set for dest %d param is %s"%(cobj.dest,repr(paramtype)))
            
        else:
            self.log.error("unrecognised cmd_code (01xs) %d"%cobj.cmd_code)
            return -1

        
        if type(cobj.dstruct) == type(None) and type(paramtype) != type(None):
            self.log.error("No param sent , expected %s"%type(paramtype))
            return -1
        
        if type(paramtype) == type(None) and type(cobj.dstruct) != type(None):
            self.log.error("Param sent ( type %s ) but None expected"%type(cobj.dstruct))
            return -1

        if type(cobj.dstruct) != type(None) and type(cobj.dstruct) != type(paramtype()):
            self.log.error("Param sent ( type %s ) but  %s expected"%(type(cobj.dstruct),type(paramtype())))
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
        n = self.nsock.send(msg)
        #self.nsock.flush()
        #self.app_fifo.close()
        self.log.warn("written %d bytes to app_fifo"%n)
        

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

        if not (hdr.type == mrf_cmd_resp or hdr.type == mrf_cmd_usr_resp or hdr.type == mrf_cmd_usr_struct):
            self.log.info("not resp or struct")
            self.log.info("hdr : %s"%repr(hdr))
            return None,None,None
        ## here just pass this to rm device model to handle
        self.log.info("server parse input got hdr %s"%repr(hdr))
        param, rsp = self.rm.packet(hdr,resp)
        
        for wup in self.rm.wups:
            ro = mrfland.RetObj()
            ro.b(mrfland.mrf_cmd('web-update',wup))
            mrfland.comm.comm(None,ro)
        self.rm.wups = []


        for dup in self.rm.dups:
            self.log.warn("%s calling _callback"%(self.__class__.__name__))
            self._callback(dup['tag'], dup['dest'] , dup['cmd'] , dup['data'])
        self.rm.dups = []


        

        
        return hdr , param , rsp 

    def handle_response(self,hdr,param, robj,response=False):
        # test if sys command response or data
        if response:
            self.log.info("handle_response hdr %s",repr(hdr))

        # check response is for active_cmd

        if type(param) == type(None):
            self.log.error("failed to get resp for packet with header %s"%repr(hdr))            
        elif response and self.active_cmd and hdr.usrc == self.active_cmd.dest and param.type == self.active_cmd.cmd_code:  # FIXME - make queue items a bit nicer
            self.log.info("got response for active command %s"%repr(self.active_cmd))
            self.log.info("rsp %s"%(param))
            if self.tcp_server.tag_is_tcp_client(self.active_cmd.tag):
                self.log.info("response for tcp client %d"%self.active_cmd.tag)
                self.log.info("robj dic is %s"%repr(robj.dic()))
                rstr = mrfland.to_json(robj.dic())
                self.tcp_server.write_to_client(self.active_cmd.tag,rstr)

            self.active_cmd = None
                                
    def _resp_handler(self,*args, **kwargs):
        self.log.info("Input on response pipe")
        resp = os.read(self.rfd, MRFBUFFLEN)

        hdr , param, resp  = self.parse_input(resp)

        if hdr  and param and resp:                    
            self.log.info("received object %s response from  %d robj %s"%(type(param),hdr.usrc,type(resp)))
            self.handle_response(hdr, param , resp, response=True )

    def _struct_handler(self,*args, **kwargs):
        self.log.info("Input on data pipe")
        resp = os.read(self.sfd, MRFBUFFLEN)

        hdr , param, resp = self.parse_input(resp)

        if hdr and param and resp :                    
            self.log.info("received object %s response from  %d"%(type(resp),hdr.usrc))
            self.handle_response(hdr, param , resp)
            
    def _connect_to_mrfnet(self):

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", install.host_mrfbus_port))
        sfno = s.fileno()

        self.log.warn("_connect_to_mrfnet - firststage  %d"%s.fileno())

        if sfno > 0:
            self.rfd = sfno
            self.nsock = s
            self.log.warn("opened response socket %d"%self.rfd)
            self.log.warn("_connect_to_mrfnet - got sfno %d"%sfno)
            self.queue_cmd('server', 1, mrf_cmd_device_info)
        else:
            self.log.error("_connect_to_mrfnet - failed to connect got sfno %d"%sfno)
            sys.exit(-1)

    def _callback(self, tag, dest, cmd,data=None):
        self.log.warn("_callback tag %s dest %d cmd %s  data type %s  %s  "%(tag,dest,cmd,type(data), repr(data)))
        self.queue_cmd(tag, dest, cmd,data)

    def _start_tcp_service(self):
        self.tcp_server = MrfTcpServer(log = self.log, landserver = self )
        self.tcp_server.listen(install.tcpport)
        self.log.info("started tcpserver on port %d"%install.tcpport)
    def _start_webapp(self):        
        
        self._web_static_handler = NoCacheStaticFileHandler

        self._web_handlers = [(r'/(favicon.ico)', self._web_static_handler, {'path': 'public/css/asa/images'}),
                         (r'/static/public/(.*)', self._web_static_handler, {'path': 'static/public'}),
                         (r'/ws', WebSocketHandler), 
                         (r'/pws', PubSocketHandler), 
                         (r'/static/secure/(.*)',AuthStaticFileHandler , {'path': 'static/secure'}),
                         (r'((/)([^/?]*)(.*))', mainapp, dict(mserv=self) ) #desperate times!
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
            self.log.warn("_check_if_anything -queue empty")
            self._deactivate()
        else:
            self.log.warn("_check_if_anything : running next command")
            self._next_cmd(self.q.get())
      

    def time_tick(self):
        
        if self._active:
            self.log.info("time_tick active");
            if self.active_cmd == None:
                self.log.info("tick can run another cmd")
            else:
                self.log.warn("time_tick active resp_timer %d"%self.resp_timer)
                self.resp_timer += 1
                if self.resp_timer > 20:
                    self.log.warn("give up waiting for response for %s"%( self.active_cmd))
                    if self.tcp_server.tag_is_tcp_client(self.active_cmd.tag):
                        self.log.info("response for tcp client %d"%self.active_cmd.tag)
                        self.log.info("sending empty dict")
                        rstr = mrfland.to_json({})
                        self.tcp_server.write_to_client(self.active_cmd.tag,rstr)
                    self.active_cmd = None

                    
            if self.active_cmd == None:
                self._check_if_anything()



        if self._active:
            self._set_timeout(0.1)
            self.quiet_cnt = 0
        else:
            self.log.info("inactive - setting 5 secs - quiet_cnt %d"%self.quiet_cnt)
            self.quiet_cnt += 1
            self._set_timeout(5.0)
            if self.quiet_cnt % 5 == 0  :
                #self.log.info("calling state task qc %d"%self.quiet_cnt)
                self.quiet_cnt = 0
            
alog.info('Application started')
if __name__ == '__main__':
    parse_command_line()
    alog.info("Mrfland web server starting on port "+str(options.port))
    
    hostlabels = {
        'timer' : ["UFH_P0","UFH_P1","RAD1_P0", "RAD1_P1", "RAD2_P0", "RAD2_P1"]
        }

   
    hb0labels = {
        'temp' : ["ACC_TOP", "ACC_MID", "ACC_BOT", "ACC_FLOW", "ACC_RET", "MIX_1", "UFH_MIX"],
        'relay' : ["UFH_PUMP", "MAIN_RAD_PUMP"]
        }


    hb1labels = {
        'temp' : ["DHW1_TOP", "DHW1_MID", "DHW1_BOT", "HB1_FLOW", "RADS1_RET",  "DHW1_HX_RET", "HB1_AMBIENT"],
        'relay' : ["RADS1_PUMP", "DHW1_HX_PUMP"]
        }

    rm = mrfland.MrflandRegManager(alog)
    
    host = MrfDevHost(rm, "host", 1, hostlabels, alog)
    
    hb0 = Pt1000Dev(rm, "pt1000_boiler_room", 2, hb0labels, alog)

    hb1 = DevHeatbox(rm, "heatbox_kitchen"  , 4, hb1labels, alog)

    

    #rm.device_register(host)
    #rm.device_register(hb0)
    #rm.device_register(hb1)

    #wat = MrfLandWebletTemps(rm, alog, {'tag':'temps','label':'Temperatures'})
    #war = MrfLandWebletRelays(rm, alog, {'tag':'relays','label':'Relays'})


    rm.weblet_register(MrfLandWebletTemps(rm, alog,
                                          {'tag':'temps','label':'Temperatures'}))

    rm.weblet_register(MrfLandWebletRelays(rm, alog,
                                           {'tag':'relays','label':'Relays'}))
    
    rm.weblet_register(MrfLandWebletTimers(rm, alog,
                                           {'tag':'timers','label':'Timers'}))
    
    
    
    ml =  MrflandServer(rm, alog )


