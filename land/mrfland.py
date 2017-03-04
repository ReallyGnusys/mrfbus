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
from datetime import datetime
import install
from mrflog import mrf_log
from mrfland_state import MrflandState

from mrf_structs import *

from mrflog import mrf_log

alog = mrf_log()

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
        self.closed = datetime.now()
        self.isopen = False
        
class mrf_comm(object):
    def __init__(self,log):
        self.log = log
        self.clients = dict()
        self.sockets = {}

    def prepare_socket(self,sid,wsid,sessid,username,stype,ip):
        self.sockets[wsid] = staff_socket(sid,wsid,sessid,username,stype,ip)
        
    def check_socket(self,wsid,ip):
        alog.info("checking socket id %s  ip %s"%(wsid,ip))
        if not wsid in self.sockets.keys():
            alog.warn("wsid not found...(%s)"%wsid)            
            return None
        skt = self.sockets[wsid]
        if skt.ip != ip:
            alog.warn("wsid (%s) ip mismatch - expected %s got %s"%(wsid,ip,skt.ip))
            return None
        else:
            return socket_info(skt)

        
        
    def add_client(self,wsid,data):
        #self.sockets[sid] = staff_socket(sid)        
        self.clients[wsid] = data
        
    def del_client(self,wsid):
        if wsid in self.clients:
            del self.clients[wsid]
            self.sockets[wsid].close()

    def _jso_broadcast(self,raw):
        alog.info( "mrf.comm._jso_broadcast : "+str(len(self.clients))+" clients")
        alog.info(" clients = "+str(self.clients))    
        for client in self.clients :   
            alog.info( "client:"+client)
            self.clients[client]['object'].write_message(raw + "\r\n\r\n")

    def send_object_to_client(self,id,obj):
        if not self.clients.has_key(id):
            errstr = "asa_comm:obj to client key error, key was "+str(id)+" obj was "+str(obj)
            alog.error(errstr)
            return
        username = self.clients[id]['username']
        alog.info( "send_object_to_client: sent cmd "+str(obj['cmd'])+ " to  "+username)
        msg = to_json(obj)
        alog.debug(msg)
        self.clients[id]['object'].write_message(msg+"\r\n\r\n")
    def broadcast(self,obj):
        msg = to_json(obj)
        alog.debug("broadcasting obj:"+msg)
        self._jso_broadcast(msg)
    def set_session_expire(self,id,seconds = install.session_timeout):
        self.sockets[id].expire = int(time.time() + seconds)

    def session_expire_notice(self,wsid):
        skt = self.sockets[wsid]
        data = { 'sid' : skt.sid , 'expire' : skt.expire }
        return mrf_cmd('session-expire',data)
        
    def session_isvalid(self,sessid):
        self.log.info("checking session_isvalid %s"%sessid)
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
            alog.error("mrf_comm: ro not ret_obj")
            return 0
        if id != None:
            for ob in ro.a():
                self.send_object_to_client(id,ob)  
            if True or ro.touch or ( len(ro.a()) > 0 ) :  # now touch session for every command received
                self.set_session_expire(id,1200) # 1200 = 20mins
                self.send_object_to_client(id,self.session_expire_notice(id))
        for ob in ro.b():
            self.broadcast(ob)                
        return 1

comm = mrf_comm(log=alog)   # FIXME! 
    
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
    alog.info('authenticate_staff : username = '+username+" , ip : "+ip)
    if username not in install.users.keys():
        return None

    uinf =  install.users[username]
    
    if password != uinf['password']:
        return None
    

    
    alog.info('authenticated')
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
    if isinstance(obj, datetime)
    or isinstance(obj, date)
    else None)


def to_json(obj):
    return json.dumps(obj,default = dt_handler)


def mrf_cmd(cmd,data):
    mcmd = {}
    mcmd['cmd'] = cmd
    mcmd['data'] = data
    return mcmd

# return utc time
def atime():    
    return mrf_cmd('datetime',datetime.utcfromtimestamp(time.time()))
 



if __name__ == "__main__":
    print "nothing"
