#!/usr/bin/env python
'''  Copyright (c) 2012-16 Gnusys Ltd

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

import os
import time
import sys
import select
import signal
import logging
import socket
import Queue
import json
import traceback
import base64
import datetime

#from datetime import datetime 
import install
import mrflog
from mrfland_state import MrflandState
from mrfland_weblet import MrflandWeblet

from mrf_structs import *



def is_mrf_obj(ob):
    if ob == None:
        return False
    if type(ob) != type({}):
        return False
    return True

class socket_info(object):
    def __init__(self,other):
        self.sid = other.sid
        self.wsid = other.wsid
        self.username = other.username
        self.ip  = other.ip

        
class staff_socket(object):
    def __init__(self,sid,wsid,sessid,username,stype,ip):
        self.opened = None
        self.closed = None
        self.isopen = True
        self.sid = sid
        self.wsid = wsid
        self.sessid = sessid
        self.username = username
        self.stype = stype
        self.ip = ip
        self.expire = None
    def __repr__(self):
        s = "%s wsid %s sessid %s username %s"%(self.__class__.__name__,self.wsid,self.sessid,self.username)
        return s
    def close(self):
        self.closed = datetime.datetime.now()
        self.isopen = False
        
class mrf_comm(object):
    def __init__(self,log):
        self.log = log
        self.clients = dict()
        self.sockets = {}

    def prepare_socket(self,sid,wsid,sessid,username,stype,ip):
        mrflog.info("mrf_comm.prepare_socket sid %s wsid %s sessid %s ip %s"%(repr(sid),repr(wsid),repr(sessid),repr(ip)))
        self.sockets[wsid] = staff_socket(sid,wsid,sessid,username,stype,ip)
        
    def check_socket(self,wsid,ip):
        mrflog.info("checking socket id %s  ip %s"%(wsid,ip))
        if not wsid in self.sockets.keys():
            mrflog.warn("wsid not found...(%s)"%wsid)            
            return None
        skt = self.sockets[wsid]
        if skt.ip != ip:
            mrflog.warn("wsid (%s) ip mismatch - expected %s got %s"%(wsid,ip,skt.ip))
            return None
            
        return socket_info(skt)

        
        
    def add_client(self,wsid,data):
        #self.sockets[sid] = staff_socket(sid)        
        mrflog.info("adding wsid %s"%wsid)
        self.clients[wsid] = data
        
    def del_client(self,wsid):
        mrflog.info("deleting wsid %s"%wsid)
        if wsid in self.clients:
            del self.clients[wsid]
            self.sockets[wsid].close()

    def _jso_broadcast(self,raw):
        mrflog.debug( "mrf.comm._jso_broadcast : "+str(len(self.clients))+" clients")
        mrflog.debug(" clients = "+str(self.clients))    
        for client in self.clients :   
            mrflog.debug( "client:"+client)
            self.clients[client]['object'].write_message(raw + "\r\n\r\n")

    def send_object_to_client(self,id,obj):
        if not self.clients.has_key(id):
            errstr = "asa_comm:obj to client key error, key was "+str(id)+" obj was "+str(obj)
            mrflog.error(errstr)
            return
        username = self.clients[id]['username']
        mrflog.info( "send_object_to_client: sent cmd "+str(obj['cmd'])+ " to  "+username)
        msg = to_json(obj)
        mrflog.debug(msg)
        self.clients[id]['object'].write_message(msg+"\r\n\r\n")
    def broadcast(self,obj):
        msg = to_json(obj)

        data = obj['data']

        if not data.has_key('tag'):
            mrflog.error("broadcast obsolete attempt of %s"%repr(data))
            return
        tag = data['tag']
        if tag['app'] == 'timers':
            mrflog.warn("broadcasting timer obj:"+msg)
            mrflog.info( "mrf.comm._jso_broadcast : "+str(len(self.clients))+" clients")
            mrflog.info(" clients = "+str(self.clients))
    
        self._jso_broadcast(msg)
    def set_session_expire(self,id,seconds = install.session_timeout):
        self.sockets[id].expire = int(time.time() + seconds)

    def session_expire_notice(self,wsid):
        skt = self.sockets[wsid]
        data = { 'sid' : skt.sid , 'expire' : skt.expire }
        return mrf_cmd('session-expire',data)
        
    def session_isvalid(self,sessid):
        self.log.info("checking session_isvalid %s"%sessid)
        self.log.info("self.sockets.keys %s"%repr(self.sockets.keys()))
        self.log.info("self.sockets %s"%repr(self.sockets))
        
        for wsid in self.sockets.keys():
            skt = self.sockets[wsid]
            self.log.info("wsid %s  skt %s"%(wsid,skt))
            if skt.sessid == sessid:
                self.log.info("sessid matched for wsid %s"%wsid)
                if skt.expire > int(time.time()):
                    return {'sid' : skt.sid , 'wsid' : skt.wsid, 'expire' :  skt.expire, 'type': skt.stype , 'username' : skt.username}
                else:
                    return None
        return None
        
    def comm(self,id,ro):
        if not is_ret_obj(ro):
            mrflog.error("mrf_comm: ro not ret_obj")
            return 0
        if id != None:
            for ob in ro.a():
                self.send_object_to_client(id,ob)  
            if True or ro.touch or ( len(ro.a()) > 0 ) :  # now touch session for every command received
                self.set_session_expire(id,1200) # 1200 = 20mins
                self.send_object_to_client(id,self.session_expire_notice(id))
        bobs= ro.b()
        nobs = len(bobs)
        mrflog.info("have %d obs to broadcast"%nobs)
        for ob in ro.b():
            mrflog.info(repr(ob))
            self.broadcast(ob)                
        return 1

comm = mrf_comm(log=mrflog)   # FIXME! 
    
class RetObj:
    def __init__ (self,touch = False):
        self._a = []
        self._b = []
        self.touch = touch
    def a(self,ob=None):
        if is_mrf_obj(ob):
            self._a.append(ob)
        elif type(ob) == type([]):
            for obe in ob:
                if is_mrf_obj(obe):
                    self._a.append(obe)
        elif ( ob == None):
            return self._a
    def b(self,ob=None):
        if is_mrf_obj(ob):
            self._b.append(ob)
        elif type(ob) == type([]):
            for obe in ob:
                if is_mrf_obj(obe):
                    self._b.append(obe)
        elif ( ob == None):
            return self._b
    def info(self):
        st = "retob %d sc  %d bc"%(len(self._a),len(self._b))
        return st

    def __str__(self):
        st = "retob .a="+str(self._a)+" .b="+str(self._b)
        return st

def is_ret_obj(ob):
    return type(ob) == type(RetObj())

    
def gen_sessid():
    return base64.b64encode(os.urandom(18))




def tbd_set_session_expire(uname,seconds = install.session_timeout):
    install.users[uname]['expire'] = int(time.time() + seconds)


def search_userdb(key,value):
    for un in install.users.keys():
        uinf = install.users[un]
        if uinf.has_key(key) and uinf[key] == value:
            return uinf
    return None
    
    

    
def ws_url(wsid):
    url = install.wsprot+install.host
    if install.domain:
        url += '.'+install.domain
    url += ':'+str(install.proxy_port)+'/ws?Id='+wsid
    #url = install.wsprot+install.host+'.'+install.domain+':'+str(install.proxy_port)+'/ws?Id='+wsid
    return url

def staff_info():
    rob = {}
    rob['cmd'] = 'staff-info'
    rob['data'] = {}
    return rob

def authenticate(username,password,ip):             
    mrflog.info('authenticate_staff : username = '+username+" , ip : "+ip)
    if username not in install.users.keys():
        return None

    uinf =  install.users[username]
    
    if password != uinf['password']:
        return None
    

    
    mrflog.info('authenticated ip '+ip)
    wsid = os.urandom(16).encode('hex')  
    sessid = gen_sessid()

    comm.prepare_socket(uinf['sid'],wsid,sessid,uinf['username'],uinf['type'],ip)    
    comm.set_session_expire(wsid)
    #install.users[username]['sessid'] = sessid
    #install.users[username]['ip'] = ip

    return { 'sid' : uinf['sid'] , 'sessid' : sessid} 


def staff_logout(sid,username,ip):
    del(install.users[username]['sessid'])
    ro = RetObj()
    return ro


dt_handler = lambda obj: (
    obj.isoformat()
    if isinstance(obj, datetime.datetime) #or isinstance(obj, date)
    else None)


def mob_handler(obj):
    if isinstance(obj, datetime.datetime): #or isinstance(obj, date)
        return obj.isoformat()
    elif isinstance(obj, datetime.time): #or isinstance(obj, date)
        return str(obj)
    elif isinstance(obj, bool): #or isinstance(obj, date)
        return str(obj)
    else:
        return obj
"""
dt_handler = lambda obj: (
    obj.isoformat()
    if isinstance(obj, datetime)
    or isinstance(obj, date)
    else None)
"""


def to_json(obj):
    try:
        js = json.dumps(obj,default = mob_handler)
        return js
    except:
        mrflog.error("to_json error obj was %s"%repr(obj))
        return "{}"

def json_parse(str):
    try:
        ob = json.loads(str)        
    except:
        return None
    return ob

def mrf_cmd(cmd,data):
    mcmd = {}
    mcmd['cmd'] = cmd
    mcmd['data'] = data
    return mcmd

# return utc time
def atime():    
    return mrf_cmd('datetime',datetime.datetime.utcfromtimestamp(time.time()))
 

        
class MrflandRegManager(object):
    """ mrfland device and sensor/actuator registration manager """
    def __init__(self):
        self.labels = {}
        self.devices = {}  ### hash devices by label - must be unique
        self.devmap  = {} ## hash devices by address
        self.sensors = {}  ### hash sensors by label
        self.senstypes = {} ### hash lists of sensors by sensor type
        self.actuators = {}
        self.sensmap = {}  ## keep physical mapping here - addr, chan dict returned for each sensor label
        self.addresses = {}  ### hash devices by address - must be unique
        self.wups = []  ## webupdates from weblets to send to browsers
        self.dups = []  ## device updates : from weblets to send to devices
        self.weblets = OrderedDict()  # has weblets by tag
        self.timers = {}
        self.server = None

    def setserver(self,server):  # OUCH , we let the server set a reference to itself..for now
        self.server = server

        ## run postinit
        for wtag in self.weblets.keys():
            wl = self.weblets[wtag]
            if hasattr( wl, 'run_init'):
                mrflog.warn( "calling run_init for weblet %s "%wtag)
                getattr(wl, 'run_init')() 
        
    def set_timer(self, tod, tag, act):
        self.server.set_timer(tod,tag,act)
        tid = tag+"."+ act
        if not self.timers.has_key(tid):
            self.timers[tid] = []  ## callback list

    def timer_reg_callback(self, tag, act, callback):
        tid = tag+"."+ act
        if not self.timers.has_key(tid):
            mrflog.error("timer_reg_callback no tid %s"%tid)
            return
        self.timers[tid].append(callback)
        
    def timer_callback(self, tag , act):
        mrflog.warn("RegManager timer_callback, tag %s act %s",tag,act)
        tid = tag+"."+ act
        if not self.timers.has_key(tid):
            mrflog.error("timer_callback no tid %s"%tid)
            return
        for f in self.timers[tid]:
            f(tag,act)
    def senslookup(self,label):
        if self.sensmap.has_key(label):
            return self.sensmap[label]

    def senslink(self, label, addr, chan):
        if self.sensmap.has_key(label):
            mrflog.error("regman.senslink - already have tag %s"%label)
            return
        self.sensmap[label] = { 'addr' : addr, 'chan' : chan }

    def webupdate(self, tag , data):
        self.wups.append({ 'tag': tag , 'data': data})

    def cmdcode(self, dest, cmdname):
        if not self.devmap.has_key(dest):
            mrflog.error("cmdcode - no device at address %d"%dest)
            return None
        dev = self.devmap[dest]
        if not dev.cmdnames.has_key(cmdname):
            mrflog.error("cmdcode - device at address %d has no command %s"%(dest,cmdname))
            return None
        return dev.cmdnames[cmdname]
                           
    def devupdaten(self,tag, dest, cmdname, data = {}):

        cmd = self.cmdcode(dest,cmdname)  # lookup cmdcode from name
        if not cmd:
            return
        
        mrflog.warn("%s devupdaten dest %s"%(self.__class__.__name__, dest))
        self.dups.append({ 'tag': tag , 'dest': dest, 'cmd' : cmd , 'data': data})
        
        
    def devupdate(self,tag, dest, cmd, data = {}):
        
        mrflog.warn("%s devupdate dest %s"%(self.__class__.__name__, dest))
        self.dups.append({ 'tag': tag , 'dest': dest, 'cmd' : cmd , 'data': data})
        
        
    def packet(self,hdr,resp):
        if self.devmap.has_key(hdr.usrc):
            return self.devmap[hdr.usrc].packet(hdr,resp)
        else:
            return None, None

    def weblet_register(self, weblet):
        if self.weblets.has_key(weblet.tag):
            mrflog.error("weblet_register - key error %s"%weblet.tag)
            return

        self.weblets[weblet.tag] = weblet
        mrflog.warn("weblet_register -registered new weblet %s"%weblet.tag)

        
    def device_register(self, dev):
        """ register new MrfDevice"""
        if self.devices.has_key(dev.label):
            mrflog.error("%s device_register duplicate device label %s"%dev.label)
            return
        self.devices[dev.label] = dev

        self.devmap[dev.address] = dev
        ### now enumerate device sensors
        for cap in dev.caps.keys():
            for ch in range(len(dev.caps[cap])):
                sens = dev.caps[cap][ch]
                if self.sensors.has_key(sens.label):
                    mrflog.error("%s device_register duplicate sensor label %s"%sens.label)
                    return
                self.sensors[sens.label] = sens
                if not self.senstypes.has_key(type(sens)):
                    self.senstypes[type(sens)] = []
                self.senstypes[type(sens)].append(sens)

    def subprocess(self, arglist, callback):
        mrflog.warn("RegManager subprocess call : arglist %s "%repr(arglist))        
        return self.server.subprocess(arglist, callback)
                
if __name__ == "__main__":
    print "nothing"
