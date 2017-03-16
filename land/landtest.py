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
import threading
import Queue
import time
import sys
import traceback
from mrf_structs import *
import ctypes
import unittest
import select
import socket
import signal
from mrfland import to_json, json_parse


def buffToObj(resp,app_cmds):
    hdr = PktHeader()
    print "buffToObj len is %d"%len(resp)
    if len(resp) >= len(hdr):
        hdr_data = bytes(resp)[0:len(hdr)]
        hdr.load(bytes(resp)[0:len(hdr)])
        print "hdr is %s"%repr(hdr)
        if hdr.type in MrfSysCmds.keys():
            if MrfSysCmds[hdr.type]['param'] and ( MrfSysCmds[hdr.type]['name'] == 'USR_RESP' or MrfSysCmds[hdr.type]['name'] == 'USR_STRUCT' ):  # testing aye
                param = MrfSysCmds[hdr.type]['param']()
                param_data = bytes(resp)[len(hdr):len(hdr)+len(param)]
                param.load(param_data)
                if param.type in MrfSysCmds.keys() and MrfSysCmds[param.type]['resp']:
                    respobj = MrfSysCmds[param.type]['resp']()
                    respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
                    respobj.load(respdat)
                    return respobj
                elif param.type in app_cmds.keys() and app_cmds[param.type]['resp']:
                    respobj = app_cmds[param.type]['resp']()
                    respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
                    respobj.load(respdat)
                    return respobj
                else:
                    print "got param type %d "%param.type
            else:
                print "got hdr.type %d"%hdr.type
    return None

                
    

import pdb

def endjsonstr(s):
    st = ''
    end = 0
    while end == 0:
        c = s.recv(1)
        if c == '}':
            end = 1
        st += c
    print "found end st = %s"%st
    return st


 
def readjsonstr(s):
    st = ''
    depth = 0
    while depth == 0:
        c = s.recv(1)
        st += c
        if c == '{':
            depth = 1
    #print "found start st = %s depth %d"%(st,depth)
    while depth != 0:
        c = s.recv(1)
        st += c
        if c == '{':
            depth += 1
            #print "found { st = %s depth %d"%(st,depth)
        elif c == '}':
            depth -= 1
            #print "found } st = %s depth %d"%(st,depth)
    #print "found end st = %s"%st
    return st

        

    

class LandTestCase(unittest.TestCase):
    def setUp(self):
        print "StubTestCase setUp : entry"
        def exit_nicely(signum,frame):
            signal.signal(signal.SIGINT, self.original_sigint)
            print "CTRL-C pressed , quitting"
            self.tearDown()            
        self.original_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT, exit_nicely)
        #unittest.installHandler()
        self.timeout = 0.4
        self.app_cmds = {}  # need to override in some ugly way to allow app command testing for now

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect(('ted',8912))  #FIXME need config param or something here
        #self.sock.setblocking(0)

    def tearDown(self):
        print "tearDown closing socket"
        self.sock.close()

    def cmd(self,dest,cmd_code,dstruct=None):

        if dest > 255:
            print "dest > 255"
            return -1


        if cmd_code in MrfSysCmds.keys():
            paramtype = MrfSysCmds[cmd_code]['param']

        elif cmd_code in self.app_cmds.keys():
            paramtype = self.app_cmds[cmd_code]['param']
            #print "got app command - cmd_code %d  paramtype %s :\n   %s"%(cmd_code,
            #                                                              repr(paramtype),
            #                                                              repr(dstruct),
            #)
        else:
            print "unrecognised cmd_code (01xs) %d"%cmd_code
            return -1
            
         
        if type(dstruct) == type(None) and type(paramtype) != type(None):
            print "No param sent , expected %s"%type(paramtype)
            return -1
        
        if type(paramtype) == type(None) and type(dstruct) != type(None):
            print "Param sent ( type %s ) but None expected"%type(dstruct)
            return -1

        if type(dstruct) != type(None) and type(dstruct) != type(paramtype()):
            print "Param sent ( type %s ) but  %s expected"%(type(dstruct),type(paramtype()))
            return -1



        mobj = { 'dest' : dest , 'cmd' : cmd_code }
        

        if dstruct:
            mobj['data'] = dstruct


        mstr = to_json(mobj)
        print "trying to send str %s"%mstr
        
        self.sock.send(mstr+"\n")
        return 0
    def response(self,timeout = 0.2):
        elapsed = 0.0
        tinc = 0.1


        resp = readjsonstr(self.sock)

    
        print "got response %s"%resp
        
        robj = json_parse(resp)

        print "after parse it's %s"%repr(robj)

        return robj
    
        return buffToObj(resp,self.app_cmds)
                        
    def quit(self):
        print "landtest quit...doing nothing"

    def check_resp(self,rsp,exp):  # rsp is a dict
        edic = exp.dic()
        for at in exp.iter_fields():
            if rsp[at] != edic[at]:
                return False
        return True
        
    def cmd_test(self,dest,cmd_code,expected,dstruct=None):
        """
        simple test interface to send cmds to destinations, supplying expected values for checking
        """
        rv = self.cmd(dest,cmd_code)
        if (rv != 0):
            return rv
        rsp = self.response()

        print "received:\n %s"%repr(rsp)
        print "expected:\n %s"%repr(expected)        
        if self.check_resp(rsp,expected) == False:
            print "ERROR "
            print "cmd_test failed"
            return -1
        else:
            print "cmd_test passed"
            return 0

