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
import queue
import json
import traceback
import base64
import datetime
import re

#from datetime import datetime
from . import install
from .mrflog import mrflog
#from mrf_structs import *
from collections import OrderedDict
import ipaddress


#comm = mrf_comm(log=mrflog)   # FIXME!


DateTimeFormat = '%Y-%m-%dT%H:%M:%S'




def tbd_set_session_expire(uname,seconds = install.session_timeout):
    install.users[uname]['expire'] = int(time.time() + seconds)


def search_userdb(key,value):
    for un in list(install.users.keys()):
        uinf = install.users[un]
        if key in uinf and uinf[key] == value:
            return uinf
    return None


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

def is_mrf_obj(ob):
    if ob == None:
        return False
    if type(ob) != type({}):
        return False
    return True

def staff_info():
    rob = {}
    rob['cmd'] = 'staff-info'
    rob['data'] = {}
    return rob


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


def sensor_null_val(stype):
    if stype == 'temp': #ouch
        initval = -273.16
    elif stype == 'relay':  # intval
        initval = -1

    else:  # assume float val
        initval = -1.0
    return initval


def sensor_round_val(stype):
    if stype == 'temp': #ouch
        initval = 2
    elif stype == 'relay':  # intval
        initval = None

    else:  # assume float val
        initval = 2
    return initval


def new_sensor_day_doc(sensor_id, stype, docdate):
    """ base doc for mongodb convering a day of minute averages """
    if not docdate.__class__.__name__ == 'datetime':
        mrflog.error("invalid docdate %s"%repr(docdate))
        return None

    initval = sensor_null_val(stype)



    doc = {
        'sensor_id' : sensor_id,
        'stype' : stype,
        'docdate' : docdate ,
        'nullval' : initval,
        'data' : []

    }

    if stype == 'temp': #ouch
        initval = -273.16
    elif stype == 'relay':  # intval
        initval = -1

    else:  # assume float val
        initval = -1.0



    for hour in range(24):
        doc['data'].append([])
        for minute in range(60):
            doc['data'][hour].append(initval)

    return doc


## coroutines



if __name__ == "__main__":
    print("nothing")
