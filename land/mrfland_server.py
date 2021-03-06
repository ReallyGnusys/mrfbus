#! /usr/bin/env python

import tornado.ioloop
import tornado.web
import tornado.websocket
import tornado.httpserver
import tornado.tcpserver
import tornado.process
from tornado.options import define, options, parse_command_line
from tornado.concurrent import Future
import socket
import logging
import re
import sys
import install
import os
import time
import json
from urllib.parse import urlparse
import signal
import queue
from collections import OrderedDict

from mrflog import mrflog
from mainapp import mainapp
from public import publicapp
from pubsock import PubSocketHandler

from mrf_structs import *
import mrfland
import ipaddress


import pdb
oursubnet = ipaddress.ip_network(install.localnet)

"""
from mrfdev_pt1000 import Pt1000Dev
from mrfdev_heatbox import DevHeatbox
from mrfdev_host import MrfDevHost
from mrfland_weblet_temps import MrfLandWebletTemps
from mrfland_weblet_relays import MrfLandWebletRelays
from mrfland_weblet_timers import MrfLandWebletTimers
"""



clients = dict()


def broadcast(obj):
    msg = mrfland.to_json(obj)
    _jso_broadcast(msg)

def _jso_broadcast(raw):
    mrflog.debug( "_jso_broadcast : "+str(len(clients))+" clients")
    mrflog.debug(" clients = "+str(clients))
    #mrflog.debug("raw:"+raw);
    for client in clients :
        mrflog.debug( "client:"+client)
        clients[client]['object'].write_message(raw + "\r\n\r\n")


def send_object_to_client(id,obj):
    username = clients[id]['username']
    mrflog.debug( "send_object_to_client: "+username+" sent cmd "+str(obj['cmd']))
    msg = mrfland.to_json(obj)
    clients[id]['object'].write_message(msg+"\r\n\r\n")

#conn = Connection(host='localhost')
#conn.connect(on_connect)



#def comm(id,ro,touch = False):
#    return mrfland.comm.comm(id,ro)



#define("port", default=2201, help="run on the given port", type=int)
#define("port", default=install.port, help="run on the given port", type=int)



server = None  # FIXME this is shonky just to get to our own methods added to  server from handlers

class IndexHandler(tornado.web.RequestHandler):
    @tornado.web.asynchronous
    def get(self):
        self.render("index.html")

class WebSocketHandler(tornado.websocket.WebSocketHandler):
    def initialize(self,rm):
        self.rm = rm
        mrflog.warn("WebSocketHandler.initialize rm %s"%repr(self.rm))


    def open(self, *args, **kwargs):
        self.id = self.get_argument("Id")
        mrflog.warn("client attempt to open ws:Id="+self.id+" kwargs "+repr(kwargs))
        #mrflog.info("here comes websockhandler self")
        #print str(dir(self))
        #check id has been issued
        #self.rm = kwargs['rm']
        mrflog.warn("rm is %s"%repr(self.rm))
        mrflog.info("headers:"+str(self.request.headers))
        #mrflog.debug("staff_type = "+str(stype))

        self.set_req_host()

        """
        rs =  self._request_summary()
        mrflog.debug("req_sum:"+rs)
        regx = r'^GET /ws\?Id=%s \(([^\(]+)\)'%self.id
        #print "rs:"+rs
        #print "regx:"+regx
        mob = re.match(regx,rs)
        if mob:
            ip = mob.group(1)
            mrflog.info("client ip="+ip)
            sob =  self.rm.comm.check_socket(self.id,ip)
            if sob == None:
                mrflog.debug("check socket failed for ip %s  self.id %s"%(str(ip),self.id))
                return

        else:
            ip = 'none'
            mrflog.debug("client ip not found!")
            return

        """
        ip = self.ip
        mrflog.info("client ip="+ip)
        sob =  self.rm.comm.check_socket(self.id,ip)
        if sob == None:
            mrflog.debug("check socket failed for ip %s  self.id %s"%(str(ip),self.id))
            return



        cdata = {"id": self.id,
                 "sid":sob.sid,
                 "ip":sob.ip,
                 "username": sob.username,
                 "apps": sob.apps,
                 "object": self
                 }
        self.rm.comm.add_client(self.id,cdata)


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



    def on_message(self, message):
        mrflog.info("client message on wsid ="+self.id+" "+str(message))
        cobj = mrfland.json_parse(message)

        if cobj and "app" in cobj and "ccmd" in cobj:
            mrflog.info("decoded client command %s"%repr(cobj))
            self.rm.web_client_command(self.id,cobj["app"],cobj["ccmd"],cobj["data"])

    def on_close(self):
        self.id = self.get_argument("Id")
        mrflog.info("*************************")
        mrflog.info("client closed ws:Id="+self.id)
        mrflog.info("ws handler close")
        mrflog.info("*************************")
        self.rm.comm.del_client(self.id)

class NoCacheStaticFileHandler(tornado.web.StaticFileHandler):
    def set_extra_headers(self, path):
        self.set_header("Cache-control", "no-cache")



class AuthStaticFileHandler(tornado.web.StaticFileHandler):
    def get_current_user(self):
        return self.get_secure_cookie(install.sess_cookie)
    def get(self, path):
        mrflog.info("AuthStaticFileHandler.get: path = "+str(path))
        if self.current_user:
            mrflog.info("authorised : returning static content");
            tornado.web.StaticFileHandler.get(self, path)
        else:
            mrflog.info("unauthorised")
            self.send_error(403)
            return None

class SimpleTcpClient(object):
    client_id = 0

    def __init__(self, stream,  landserver,disconnect):
        #super(type(self)).__init__(self)
        SimpleTcpClient.client_id += 1
        self.landserver = landserver
        self.disconnect = disconnect
        self.id = SimpleTcpClient.client_id
        #print "SimpleTcpClient constuctor %d"%self.id
        mrflog.info("SimpleTcpClient constructor %d"%self.id)
        self.stream = stream

        self.stream.socket.setsockopt(
            socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.stream.socket.setsockopt(
            socket.IPPROTO_TCP, socket.SO_KEEPALIVE, 1)
        self.stream.set_close_callback(self.on_disconnect)


    @tornado.gen.coroutine
    def on_disconnect(self):
        mrflog.info("tcp conn %d disconnected"%self.id)
        self.disconnect(self.id)
        yield []

    @tornado.gen.coroutine
    def dispatch_client(self):
        try:
            while True:
                line = yield self.stream.read_until(b'\n')
                mrflog.info("got line %s"%line)
                #s = line.decode('utf-8').strip()
                #mrflog('got |%s|' % repr(s))
                line = line.decode('utf-8')
                ob = mrfland.json_parse(line)
                obdecoded = False
                if ob:
                    data = None
                    mrflog.info("json ob : %s"%repr(ob))

                    if 'sys_cmd' in ob:
                        if 'data' in ob:
                            data = ob['data']
                        self.landserver._sys_callback(self.id,ob['sys_cmd'],data)

                    if 'dest' in ob and 'cmd' in ob:
                        if ob['cmd'] in MrfSysCmds:
                            obdecoded = True
                            if MrfSysCmds[ob['cmd']]['param'] != None:
                                data = MrfSysCmds[ob['cmd']]['param']()
                        else:
                            appcmdset = self.landserver._dev_cmd_set(ob['dest'])
                            if appcmdset and ob['cmd'] in appcmdset:
                                obdecoded = True
                                if appcmdset[ob['cmd']]['param'] != None:
                                    data = appcmdset[ob['cmd']]['param']()




                        if obdecoded == False: #cmd code not found - error to client
                            mrflog.warn("cmd %d not found for device %d in ob  %s"%(ob['dest'],ob['cmd'],repr(ob)))
                            self.stream.write('{"error" : "not valid cmd for device" }')
                        else:
                            if 'data' in ob:
                                data.dic_set(ob['data'])
                            self.landserver._callback(self.id, ob['dest'],ob['cmd'],data)

                    else:
                        mrflog.info("no valid cmd decoded in  %s"%repr(line))
                else:
                    mrflog.warn("no json ob decoded in %s"%repr(line))
                yield self.stream.write(bytes("\n",'utf-8'))
        except tornado.iostream.StreamClosedError:
            pass

    @tornado.gen.coroutine
    def on_connect(self):
        raddr = 'closed'
        try:
            raddr = '%s:%d' % self.stream.socket.getpeername()
        except Exception:
            pass
        mrflog.info('new, %s' % raddr)

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
        self.receipt = None # get receipt from mrfnet once command is issued
    def __repr__(self):
        s = "%s: tag %s dest %d cmd_code %d dstruct %s receipt %s"%\
            (self.__class__.__name__,self.tag,self.dest,self.cmd_code,repr(self.dstruct),repr(self.receipt))
        return s


class MrfTcpServer(tornado.tcpserver.TCPServer):
    def __init__(self, landserver):
        tornado.tcpserver.TCPServer.__init__(self)
        self.landserver = landserver
        self.clients = {}


    def client_disconnect(self,id):
        self.clients.pop(id)
        mrflog.info("removed tcp client %s"%repr(id))
        mrflog.info("tcp clients now %s"%repr(list(self.clients.keys())))
    def tag_is_tcp_client(self,tag):
        return tag in self.clients

    def write_to_client(self,id , msg):
        if id not in self.clients:
            mrflog.error("%s no client with id %d"%(self.__class__.__name__,id))
            return
        self.clients[id].stream.write(bytes(msg,'utf-8'))
        mrflog.info("wrote_to client %d"%id)
    @tornado.gen.coroutine
    def handle_stream(self, stream, address):
        """
        Called for each new connection, stream.socket is
        a reference to socket object
        """
        connection = SimpleTcpClient(stream, self.landserver,self.client_disconnect)
        self.clients[connection.id] = connection
        yield connection.on_connect()

class AnyFut(Future):
    def __init__(self, futures):
        super(AnyFut, self).__init__()
        for future in futures:
            future.add_done_callback(self.done_callback)

    def done_callback(self, future):
        self.set_result(future)


class MrflandServer(object):

    def __init__(self,rm, config):
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            mrflog.warn( "CTRL-C pressed , quitting")
            sys.exit(0)
        global server
        self.rm = rm
        self.config = config
        self._timeout_handle = None
        self._minute_tick_handle = None
        self.timers = dict()
        server = self  # FIXME ouch
        self.rm.setserver(server)  # double OUCH

        self.q = queue.Queue()
        self.active_cmd = None
        self.active_timer = 0

        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)
        self._active = False

        if 'mrfbus_host_port' in self.config:
            self._start_mrfnet(self.config['mrfbus_host_port'],self.config['mrf_netid'])
        self.quiet_cnt = 0
        self._start_webapp()
        if 'tcp_test_port' in self.config:
            self._start_tcp_service( self.config['tcp_test_port'])
        self.ioloop = tornado.ioloop.IOLoop.instance()
        mrflog.warn("MrflandServer tornado IOLoop starting : IOLoop.time %s"%repr(self.ioloop.time()))
        #self._deactivate()  # start quiet

        if hasattr(self,'nsock'):
            self.ioloop.add_handler(self.nsock,self._resp_handler,self.ioloop.READ)



        if 'console' in self.config:
            from thirdparty.tornado_console import ConsoleServer
            #my_locals = {}
            self.console_server = ConsoleServer(locals())
            #my_locals['server'] = self.console_server
            self.console_server.listen(self.config['console']['port'])
        tornado.ioloop.IOLoop.instance().start()
        mrflog.error( "do we ever get here?")


    def _start_mrfnet (self, port,  netid):
        self.hostaddr = 1
        self.netid = netid



        self._connect_to_mrfnet(port)




    def subprocess(self, arglist, callback):  # FIXME - we just collect exit code for running unittest clients
        process = tornado.process.Subprocess(arglist, stdout=None, stderr=None)
        process.set_exit_callback(callback)

    def timer_callback(self, *args, **kwargs):

        if 'tag' not in kwargs or 'act' not in kwargs:
            mrflog.error("timer_callback got kwargs %s quitting"%repr(kwargs.keys))
            return
        mrflog.info("timer_callback got kwargs %s"%repr(kwargs.keys))
        app = kwargs['app']
        tag = kwargs['tag']
        act = kwargs['act']
        tid = app+"."+tag+"."+ act
        if tid not in self.timers:
            mrflog.error("timer_callback : no timer with tid  %s"%tid)
            return
        tinf = self.timers[tid]
        self.rm.timer_callback(app=app,tag=tag, act=act)
        #self.set_timer(tinf['tod'], tinf['tag'], tinf['act'])


    def set_timer(self, tod , app, tag , act ):
        """
        tod : time of day, type  datetime.time
        tag : could use for example relay tag
        act : on, off etc
        """
        if type(tod) != datetime.time:
            mrflog.error("set_timer tod must be datetime.time")
            return


        tid = app+"."+tag+"."+ act
        if tid in self.timers:
            tinf = self.timers[tid]
            mrflog.info("removing existing timeout for tid %s"%tid)
            tornado.ioloop.IOLoop.instance().remove_timeout(tinf['thandle'])

        now = datetime.datetime.now()

        nowtime = now.time()
        nowdate = now.date()


        tdt = datetime.datetime.combine(nowdate, tod)

        if nowtime >= tod:
            tdt += datetime.timedelta(days=1)

        tts = time.mktime(tdt.timetuple())
        mrflog.info("setting timer %s with ts %s"%(tid,tts))
        th =  tornado.ioloop.IOLoop.instance().add_timeout(tts,self.timer_callback , app=app, tag=tag, act=act)
        self.timers[tid]  = { 'thandle' : th,  'tod' : tod, 'app' : app, 'tag' : tag , 'act' : act }



    def _run_updates(self):
        for wup in self.rm.wups:
            ro = mrfland.RetObj()
            if 'wsid' in wup:
                ro.a(mrfland.mrf_cmd('web-update',wup))
                self.rm.comm.comm(wup['wsid'],ro)
            else:
                ro.b(mrfland.mrf_cmd('web-update',wup))
                self.rm.comm.comm(None,ro)
        self.rm.wups = []


        for dup in self.rm.dups:
            mrflog.info("%s calling _callback"%(self.__class__.__name__))
            self._callback(dup['tag'], dup['dest'] , dup['cmd'] , dup['data'])
        self.rm.dups = []


    def _set_next_min_timeout(self):
        now = time.time()
        nsecs = time.localtime(now).tm_sec  + (now % 1.0)
        wsecs = self._minute_tick_secs - nsecs
        if wsecs < 0.0:
            wsecs += 60.0

        ct = now + wsecs
        if self._minute_tick_handle:
            tornado.ioloop.IOLoop.instance().remove_timeout(self._minute_tick_handle)

        self._minute_tick_handle = tornado.ioloop.IOLoop.instance().add_timeout(ct,self._minute_timeout)

    def start_minute_tick(self,seconds,callback):
        self._minute_tick_secs = float(seconds)
        self._minute_callback = callback
        self._set_next_min_timeout()

    def _minute_timeout(self):

        self._set_next_min_timeout()
        t = time.time()

        mrflog.debug("minutetimeout %f %s"%(t,repr(time.localtime(t))))
        self._minute_callback()

    def _set_timeout(self,s):
        if self._timeout_handle:
            tornado.ioloop.IOLoop.instance().remove_timeout(self._timeout_handle)
        ct = time.time() + s
        self._timeout_handle = tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)

    def _activate(self):  # ramp up tick while responses or txqueue outstanding
        if True or not self._active:
            mrflog.info("activating! qsize %s"%self.q.qsize())
            self.active_timer = 0
            self._active = True
            self._set_timeout(0.02)

    def _deactivate(self):  # ramp down tick when quiet
        mrflog.info("deactivating!")
        self._active = False
        self.active_cmd = None
        self._set_timeout(5.0)
            #ct = time.time() + 5
            #tornado.ioloop.IOLoop.instance().add_timeout(ct,self.time_tick)


    def queue_cmd(self,tag, dest, cmd_code,dstruct=None):
        cobj = MrfCmdObj(tag, dest, cmd_code,dstruct)
        if self.active_cmd:
            mrflog.info("queuing cmd %s"%repr(cobj))
            self.q.put(cobj)
        else:
            mrflog.info("running cmd immediately %s"%repr(cobj))
            if self._next_cmd(cobj) == 0:
                self._activate()

    def _next_cmd(self,cobj):
        if self._run_cmd(cobj) == 0:
            self.resp_timer = 0
            self.active_cmd = cobj
            mrflog.info("self.active_cmd %s"%repr(self.active_cmd))
            return 0
        else:
            mrflog.warn("_run_cmd failed for %s"%repr(cobj))
            mrflog.info("sending empty response for tcp client %d"%cobj.tag)
            self.tcp_server.write_to_client(cobj.tag, mrfland.to_json({}))
            return -1

    def TBD_app_cmd_set(self,dest):
        for appn in list(self.apps.keys()):
            app = self.apps[appn]
            if self.apps[appn].i_manage(dest):
                return self.apps[appn].cmd_set(dest)

    def _dev_cmd_set(self,dest):
        if dest not in self.rm.devmap:
            mrflog.error("no device %d found in rm"%dest)
            return None

        return self.rm.devmap[dest]._cmdset

    def _run_cmd(self,cobj):
        mrflog.debug("cmd : dest %d cmd_code %s"%(cobj.dest,repr(cobj.cmd_code)))
        if cobj.dest > 255:
            print("dest > 255")
            return -1

        appcmds = self._dev_cmd_set(cobj.dest)
        if cobj.cmd_code in list(MrfSysCmds.keys()):
            paramtype = MrfSysCmds[cobj.cmd_code]['param']
        elif (appcmds and cobj.cmd_code in appcmds):
            paramtype = appcmds[cobj.cmd_code]['param']
            mrflog.info("app supplied command set for dest %d param is %s"%(cobj.dest,repr(paramtype)))

        else:
            mrflog.error("unrecognised cmd_code (01xs) %d"%cobj.cmd_code)
            return -1


        if type(cobj.dstruct) == type(None) and type(paramtype) != type(None):
            mrflog.error("No param sent , expected %s"%type(paramtype))
            return -1

        if type(paramtype) == type(None) and type(cobj.dstruct) != type(None):
            mrflog.error("Param sent ( type %s ) but None expected"%type(cobj.dstruct))
            return -1

        if type(cobj.dstruct) != type(None) and type(cobj.dstruct) != type(paramtype()):
            mrflog.error("Param sent ( type %s ) but  %s expected"%(type(cobj.dstruct),type(paramtype())))
            return -1

        mlen = 4
        if cobj.dstruct:
            mlen += len(cobj.dstruct)
        if mlen > 64:
            print("mlen = %d"%mlen)
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
        for i in range(mlen -1):
            csum += int(msg[i])
        csum = csum % 256
        msg[mlen - 1] = csum
        mrflog.info("msg len is %d"%len(msg))
        return self._app_fifo_write(msg)

    def _app_fifo_write(self,msg):
        if hasattr(self,'nsock'):
            n = self.nsock.send(msg)
            #self.nsock.flush()
            #self.app_fifo.close()
            mrflog.debug("written %d bytes to app_fifo"%n)
            return 0
        else:
            mrflog.error("failed to write message to app_fifo %s"%repr(msg))
            return -1

    def parse_input(self,resp):
        """
        check for valid looking header and types
        if found returns
           hdrpacket  , resppacket,  databuff
        else returns
           None
        """

        mrflog.debug( "parse_input len is %d"%len(resp))
        hdr = PktHeader()

        if len(resp) < len(hdr):  # no way valid
            mrflog.warn("ALERT len(resp) was %d!"%len(resp))
            return None,None,None


        hdr.load(bytes(resp)[0:len(hdr)])

        if hdr.netid != self.netid: # only looking for packets from this netid
            mrflog.warn("not our netid")
            mrflog.warn(("hdr is:\n%s\n"%repr(hdr)))
            return None,None,None

        if hdr.udest != 0: # only looking for packets destined for us, except for receipts

            if hdr.usrc != 0:
                mrflog.warn("not receipt for us %s"%repr(hdr))
                return None,None,None

            if self.active_cmd and hdr.udest == self.active_cmd.dest and hdr.type == self.active_cmd.cmd_code:
                mrflog.warn("got receipt for active cmd  msgid 0x%02x"%hdr.msgid)
                self.active_cmd.receipt = hdr
                mrflog.info(repr(hdr))

                return hdr, None,None


            else:
                mrflog.warn("unexpected receipt %s"%repr(hdr))
                if self.active_cmd:
                    mrflog.warn("active_cmd %s"%self.active_cmd)
            return None,None,None


        if  hdr.type == mrf_cmd_ndr:
            ndr = PktNDR()

            ndr.load(bytes(resp)[len(hdr):len(hdr)+len(ndr)])

            mrflog.warn("got NDR %s"%repr(ndr))
            mrflog.warn("got header was  %s"%repr(hdr))
            if self.active_cmd and self.active_cmd.receipt and self.active_cmd.receipt.msgid == ndr.msgid and self.active_cmd.dest == ndr.hdest:
                mrflog.warn("got NDR for active cmd %s"%repr(ndr))
                self.robj_for_active_cmd(ndr)
                self.active_cmd = None

            self.rm.ndr(hdr,ndr)
            param, rsp = ndr, None
        elif  hdr.type == mrf_cmd_resp or hdr.type == mrf_cmd_usr_resp or hdr.type == mrf_cmd_usr_struct:
            ## here just pass this to rm device model to handle
            mrflog.debug("server parse resp got hdr %s  len(resp) 0x%X"%(repr(hdr),len(resp)))

            # pass packet to rm , which runs device packet handler
            param, rsp = self.rm.packet(hdr,resp)
            mrflog.debug("called rm packet \n : hdr was \n%s)"%(repr(hdr)))

        else:
            mrflog.warn("not resp or struct or ndr")
            mrflog.warn("hdr : %s"%repr(hdr))
            return None,None,None

        # all webapp callbacks for packet have been run at this point
        # so empty anything that has accrued in the queues as a result
        self._run_updates()





        return hdr , param , rsp

    def handle_response(self,hdr,param, robj,response=False):
        # test if sys command response or data
        if response:
            mrflog.info("handle_response hdr %s",repr(hdr))

        # check response is for active_cmd

        if type(param) == type(None):
            mrflog.error("failed to get resp for packet with header %s"%repr(hdr))
        elif response and self.active_cmd and hdr.usrc == self.active_cmd.dest and param.type == self.active_cmd.cmd_code:  # FIXME - make queue items a bit nicer

            self.robj_for_active_cmd(robj)

            self.active_cmd = None


    def robj_for_active_cmd(self,robj):
        #mrflog.info("got response for active command %s"%repr(self.active_cmd))
        if hasattr(self,'tcp_server') and self.tcp_server.tag_is_tcp_client(self.active_cmd.tag):
            mrflog.info("response for tcp client %d"%self.active_cmd.tag)
            mrflog.info("robj dic is %s"%repr(robj.dic()))
            rstr = mrfland.to_json(robj.dic())
            self.tcp_server.write_to_client(self.active_cmd.tag,rstr)

    def _resp_handler(self,*args, **kwargs):
        rresp = os.read(self.rfd, 1024)

        mrflog.debug("Input on response pipe len %d "%(len(rresp)))
        while len(rresp):

            hdr , param, resp  = self.parse_input(rresp)
            #pdb.set_trace()
            if hdr:
                rresp=rresp[hdr.length:]
            else:
                mrflog.warn("no hdr len(rresp) %d type resp %s"%(len(rresp),str(type(resp))))
                mrflog.warn("type hdr "+str(type(hdr)))

                mrflog.error("resp :"+repr(rresp))
                sys.exit(-1)

            mrflog.debug("hdr on resp_pipe %s"%repr(hdr))

                #mrflog.warn("ccnt %d hdr %s parm %s resp %s"%(ccnt,repr(hdr),repr(param),repr(resp)))
            if hdr  and param:
                if resp:
                    mrflog.info("received object %s response from  %d robj %s"%(type(param),hdr.usrc,type(resp)))
                    self.handle_response(hdr, param , resp, response=True )


                    mrflog.debug("_resp_handler , got resp %s"%repr(resp))
                    mrflog.debug("_resp_handler , got hdr %s"%repr(hdr))
                    mrflog.debug("_resp_handler , got param %s"%repr(param))
                elif hdr.type != mrf_cmd_ndr:
                    mrflog.warn("failed to decode response - hdr %s \nparam %s"%(repr(hdr),repr(param)))

    def _struct_handler(self,*args, **kwargs):
        mrflog.debug("Input on data pipe")
        rresp = os.read(self.sfd, 2048)


        while len(rresp):
            hdr , param, resp = self.parse_input(rresp)

            if hdr:
                rresp=rresp[hdr.length:]

            else:
                mrflog.warn("no hdr")

            if hdr and param and resp :
                mrflog.debug("received object %s response from  %d"%(type(resp),hdr.usrc))
                self.handle_response(hdr, param , resp)
            else:
                mrflog.debug("_struct_handler , got resp %s"%repr(resp))
                mrflog.debug("_struct_handler , got hdr %s"%repr(hdr))


    def _connect_to_mrfnet(self,port):

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", port))
        sfno = s.fileno()

        mrflog.warn("_connect_to_mrfnet localhost port %d :  firststage fd  %d"%(port,s.fileno()))

        if sfno > 0:
            self.rfd = sfno
            self.nsock = s
            mrflog.warn("opened response socket %d"%self.rfd)
            mrflog.warn("_connect_to_mrfnet - got sfno %d"%sfno)
            self.queue_cmd('server', 1, mrf_cmd_device_info)
        else:
            mrflog.error("_connect_to_mrfnet - failed to connect got sfno %d"%sfno)
            sys.exit(-1)

    def _callback(self, tag, dest, cmd,data=None):
        mrflog.info("_callback tag %s dest %d cmd %s  data type %s  %s  "%(tag,dest,cmd,type(data), repr(data)))
        self.queue_cmd(tag, dest, cmd,data)

    def _sys_callback(self, tag,  cmd,data=None):
        mrflog.info("_callback tag %s  cmd %s  data type %s  %s  "%(tag,cmd,type(data), repr(data)))

        if cmd == 'reload_cert':
            self.ssl_reload_cert_chain()
        else:
            mrflog.warn("sys cmd not recognised "+repr(cmd))
    def _start_tcp_service(self, port):
        self.tcp_server = MrfTcpServer( landserver = self )
        self.tcp_server.listen(port)
        mrflog.info("started tcpserver on port %d"%port)
    def _start_webapp(self):

        if 'MRFBUS_HOME' in os.environ:
            mrfhome = os.environ['MRFBUS_HOME']
        else:
            mrflog.error("MRFBUS_HOME not defined, webapp unlikely to work...")
            mrfhome = ''


        if 'PWD' in os.environ:
            cwd = os.environ['PWD']
        else:
            mrflog.error("PWD not defined, webapp unlikely to work...")
            cwd = ''



        self._web_static_handler = NoCacheStaticFileHandler

        self._web_handlers = [(r'/(favicon.ico)', self._web_static_handler, {'path': os.path.join(mrfhome,'land','static/favicon.ico')}),
                              (r'/static/public/(.*)', self._web_static_handler, {'path': os.path.join(mrfhome,'land','static/public')}),
                              (r'/static/thirdparty/(.*)', self._web_static_handler, {'path': os.path.join(cwd,'static/thirdparty')}),
                              (r'/ws', WebSocketHandler, dict(rm=self.rm)), # need rm to track connections and auth
                              (r'/pws', PubSocketHandler),
                              (r'/static/secure/(.*)',AuthStaticFileHandler , {'path': os.path.join(mrfhome,'land','static/secure')}),
                              (r'((/)([^/?]*)(.*))', mainapp, dict(rm=self.rm) )
        ]

        self._webapp = tornado.web.Application(self._web_handlers,cookie_secret=install.cookie_secret)

        self.nsettings = dict()


        if install.https_server:
            import ssl


            self.ssl_ctx = ssl.create_default_context(purpose=ssl.Purpose.CLIENT_AUTH)
            self.ssl_ctx.check_hostname = False
            self.ssl_reload_cert_chain()
            self.nsettings["ssl_options"] = self.ssl_ctx
            """
            self.nsettings["ssl_options"] = {
                "certfile": install._ssl_cert_file,
                "keyfile" : install._ssl_key_file
            }

            """
            mrflog.info("starting https server certfile %s keyfile %s"%\
                      (install._ssl_cert_file,install._ssl_key_file))
        else:
            mrflog.info("starting http server")

        self.http_server = tornado.httpserver.HTTPServer(self._webapp, **self.nsettings)

        self.http_server.listen(self.rm.config['http_port'])


    def ssl_reload_cert_chain(self):
        mrflog.warn("reloading SSL cert chain")
        self.ssl_ctx.load_cert_chain(install._ssl_cert_file,keyfile=install._ssl_key_file)

    def _check_if_anything(self):
        if self.q.empty():
            mrflog.debug("_check_if_anything -queue empty")
            self._deactivate()
        else:
            mrflog.debug("_check_if_anything : running next command")
            self._next_cmd(self.q.get())


    def time_tick(self):

        if self._active:
            mrflog.info("time_tick active");
            if self.active_cmd == None:
                mrflog.info("tick can run another cmd")
            else:
                mrflog.info("time_tick active resp_timer %d"%self.resp_timer)
                self.resp_timer += 1
                if self.resp_timer > 7:
                    mrflog.warn("give up waiting for response for %s"%( self.active_cmd))
                    if hasattr(self,'tcp_server') and self.tcp_server.tag_is_tcp_client(self.active_cmd.tag):
                        mrflog.info("response for tcp client %d"%self.active_cmd.tag)
                        mrflog.info("sending empty dict")
                        rstr = mrfland.to_json({})
                        self.tcp_server.write_to_client(self.active_cmd.tag,rstr)
                    self.active_cmd = None


            if self.active_cmd == None:
                self._check_if_anything()



        if self._active:
            self._set_timeout(0.1)
            self.quiet_cnt = 0
        else:
            mrflog.info("inactive - setting 5 secs - quiet_cnt %d"%self.quiet_cnt)
            self.quiet_cnt += 1
            self._set_timeout(5.0)
            if self.quiet_cnt % 5 == 0  :
                #mrflog.info("calling state task qc %d"%self.quiet_cnt)
                self.rm.quiet_task()
                self.quiet_cnt = 0
