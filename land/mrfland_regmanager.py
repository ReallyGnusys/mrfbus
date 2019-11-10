import tornado.gen
import datetime
import re
import os
import time
import base64
from collections import OrderedDict
import socket
import install
import pdb
from mrf_sens_period import MrfSensPeriod
from mrflog import mrflog
from mrf_structs import PktTimeDate
from thirdparty_static import ThirdPartyStaticMgr
import mrfland
import ipaddress
#from mrfland import to_json




class socket_info(object):
    def __init__(self,other):
        self.sid = other.sid
        self.wsid = other.wsid
        self.username = other.username
        self.ip  = other.ip
        self.req_host  = other.req_host
        self.apps = other.apps


class staff_socket(object):
    def __init__(self,sid,wsid,sessid,username,stype,apps,ip,req_host):
        self.opened = None
        self.closed = None
        self.isopen = True
        self.sid = sid
        self.wsid = wsid
        self.req_host = req_host
        self.sessid = sessid
        self.username = username
        self.stype = stype
        self.apps = apps
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

    def prepare_socket(self,sid,wsid,sessid,username,stype,apps,ip,req_host):
        mrflog.warn("mrf_comm.prepare_socket sid %s wsid %s sessid %s ip %s"%(repr(sid),repr(wsid),repr(sessid),repr(ip)))
        self.sockets[wsid] = staff_socket(sid,wsid,sessid,username,stype,apps,ip,req_host)

    def check_socket(self,wsid,ip):  # FIXME should have timeout if socket not opened in v short time after prepare_socket
        mrflog.info("checking socket id %s  ip %s"%(wsid,ip))
        if not wsid in self.sockets.keys():
            mrflog.warn("wsid not found...(%s)"%wsid)
            return None
        skt = self.sockets[wsid]
        if skt.ip != ip:
            mrflog.error("wsid (%s) ip mismatch - expected %s got %s"%(wsid,ip,skt.ip))
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

    def staff_logout(self,sob,ip):
        wsid = sob['wsid']
        mrflog.warn("staff_logout wsid "+wsid)
        self.del_client(wsid)

    def _jso_broadcast(self,app,raw):
        mrflog.debug( "mrf.comm._jso_broadcast : app"+str(app)+" "+str(len(self.clients))+" clients")
        mrflog.debug(" clients = "+str(self.clients))
        for wsid in self.clients.keys() :
            mrflog.debug( "client wsid:"+wsid)
            client = self.clients[wsid]
            if (app == 'auto_graph') or app in client['apps']:
                client['object'].write_message(raw + "\r\n\r\n")

    def send_object_to_client(self,id,obj):
        if not self.clients.has_key(id):
            errstr = "asa_comm:obj to client key error, key was "+str(id)+" obj was "+str(obj)
            mrflog.error(errstr)
            return
        client = self.clients[id]

        #username = self.clients[id]['username']
        username = client['username']

        if not obj.has_key('data'):
            mrflog.error("send obj to client ... no data in %s"%repr(obj))
            return
        data = obj['data']

        if not data.has_key('tag'):
            mrflog.error("send obj to client ... no tag in %s"%repr(data))
            app = None
        else:
            app = data['tag']['app']

        mrflog.info( "send_object_to_client: sent cmd "+str(obj['cmd'])+ " to  "+username)
        msg = mrfland.to_json(obj)
        mrflog.debug(msg)
        mrflog.warn("checking client %s"%repr(client))
        if app and ((app ==  'auto_graph')  or app in client['apps']):
            client['object'].write_message(msg+"\r\n\r\n")
    def broadcast(self,obj):
        msg = mrfland.to_json(obj)

        data = obj['data']

        if not data.has_key('tag'):
            mrflog.error("broadcast obsolete attempt of %s"%repr(data))
            return
        tag = data['tag']
        app = tag['app']
        if app == 'timers':
            mrflog.warn("broadcasting timer obj:"+msg)
            mrflog.info( "mrf.comm._jso_broadcast : "+str(len(self.clients))+" clients")
            mrflog.info(" clients = "+str(self.clients))

        self._jso_broadcast(app,msg)
    def set_session_expire(self,id,seconds = install.session_timeout):
        self.sockets[id].expire = int(time.time() + seconds)

    def session_expire_notice(self,wsid):
        skt = self.sockets[wsid]
        data = { 'sid' : skt.sid , 'expire' : skt.expire }
        return mrfland.mrf_cmd('session-expire',data)

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
                    return {'sid' : skt.sid , 'wsid' : skt.wsid, 'expire' :  skt.expire,
                            'type': skt.stype , 'username' : skt.username , 'apps' : skt.apps, 'req_host':skt.req_host}
                else:
                    return None
        return None

    def comm(self,id,ro):
        if not mrfland.is_ret_obj(ro):
            mrflog.error("mrf_comm: ro not ret_obj")
            return 0
        if id != None:
            mrflog.warn("comm id = %s ro %s"%(repr(id),repr(ro)))
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


class MrflandRegManager(object):
    _HISTORY_SECONDS_ = 60*60*24
    """ mrfland device and sensor/actuator registration manager """
    def __init__(self,config):
        self.labels = {}
        self.devices = {}  ### hash devices by label - must be unique
        self.devmap  = {} ## hash devices by address
        self.sensors = {}  ### hash sensors by label
        self.senstypes = {} ### hash lists of sensors by sensor type
        self.senscaps = {}  ## hash lists by cap string
        self.actuators = {}
        self.sensmap = {}  ## keep physical mapping here - addr, chan dict returned for each sensor label
        self.addresses = {}  ### hash devices by address - must be unique
        self.wups = []  ## webupdates from weblets to send to browsers
        self.dups = []  ## device updates : from weblets to send to devices
        self.hostname = socket.gethostname()
        self.weblets = OrderedDict()  # has weblets by tag
        self.all_weblets = [] # list of weblet keys - convenience
        self.timers = {}
        self.sgraphs = {}  # support graph data for these sensors during this mrfland session
        self.graph_insts = 0
        self.config = config
        self.comm = mrf_comm(log=mrflog)
        if self.config.has_key('db_uri') and type(self.config['db_uri'])==type("") and self.config['db_uri'] != "" and not 'password' in self.config['db_uri']:
            from motor import motor_tornado
            self.db = motor_tornado.MotorClient(self.config['db_uri']).mrfbus
            mrflog.warn("opened db connection %s"%repr(self.db))

        self.period_timers = {}
        self.period_lut = {}

        self.tp_static_mgr = ThirdPartyStaticMgr()

        """
        if self.config.has_key('periods') and type(self.config['periods'])==type([]):
            mrflog.warn("setting up period sensors")
            pchan = 0
            for pr in self.config['periods']:
                sn = pr + '_PERIOD'  # sensor name
                self.period_timers[pr] = {} # have dict of MrfTimer components for each period
                psens = MrfSensPeriod(sn, None, 0, pchan)
                pchan += 1
                self.add_sensor(psens,'period')
        """
        self.server = None


    def add_period(self,pr):
        sn = pr + '_PERIOD'  # sensor name

        if pr in self.period_timers:
            mrflog.warn("period %s already registered"%pr)
            return

        self.period_timers[pr] = {} # have dict of MrfTimer components for each period
        psens = MrfSensPeriod(sn, None, 0, 0)
        self.add_sensor(psens,'period')
        mrflog.warn("period %s  registered"%pr)



    def setserver(self,server):  # OUCH , we let the server set a reference to itself..for now
        self.server = server   # Please only let a mrfland_server or subclass ever call this function
        ## run postinit
        for wtag in self.weblets.keys():
            wl = self.weblets[wtag]
            if hasattr( wl, 'run_init'):
                mrflog.warn( "calling run_init for weblet %s "%wtag)
                getattr(wl, 'run_init')()
        ## rm should connect graph callbacks here
        """ regmanager now keeps history for registered sensors """
        self.shist = {}
        self.shist_ts = []
        self.shist_start_time = time.time()  ## should be consistent with self.shist_ts[0]
        self.shist_last_time = self.shist_start_time ## last time reading arrived
        for sl in self.sgraphs.keys():
            mrflog.warn("graph subscribe %s"%sl)
            flds = self.sgraphs[sl]
            self.shist[sl] = {}
            for fld in flds:
                self.shist[sl][fld] = []

            #self.sensors[sl].subscribe_minutes(self.graph_callback)

        # get sensors that have data in db
        if hasattr(self,'db'):
            tornado.ioloop.IOLoop.current().run_sync(lambda: self.db_get_sensors())

        ## start minute tick

        self.server.start_minute_tick(0,self._minute_tick)

    def _minute_tick(self):
        #print("%s _minute_tick "%(self.__class__.__name__))
        mrflog.debug("%s _minute_tick "%(self.__class__.__name__))
        now = time.time()
        lnow = time.localtime(now)
        ltime = time.localtime(self.shist_last_time)

        mrflog.debug("%s _minute_tick ltime %s "%(self.__class__.__name__,repr(ltime)))

        ts = time.strftime("%Y-%m-%dT%H:%M:%S",ltime)
        self.shist_ts.append(ts)

        mrflog.debug("now %s"%(repr(lnow)))
        #pdb.set_trace()
        #self.shist_ts.append
        mrflog.debug("sgraphs %s "%repr(self.sgraphs.keys()))

        gdata = { 'ts' : ts,
                  'sensors' : {}
                  }
        for sname in self.sgraphs.keys():
            flds = self.sgraphs[sname]
            savg = self.sensors[sname].gen_average()
            for fld in flds:
                if fld in savg.keys():
                    savg[fld] = round(savg[fld],2)
                    self.shist[sname][fld].append(savg[fld])  #limit to 2 dp precision
            #savg['ts'] = ts
            gdata['sensors'][sname] = savg
        mrflog.debug("self.shist_ts[0] %s shist_start_time %s"%(repr(self.shist_ts[0]),repr(time.localtime(self.shist_start_time))))
        mrflog.debug("gdata "+repr(gdata))

        self.graph_callback(gdata)

        self.shist_last_time = now
        mrflog.debug("shist_last_time %s graph_callback called with  %s"%(repr(self.shist_last_time),repr(gdata)))
        #mrflog.debug("len wups %d"%(len(self.wups)))
        while self.shist_last_time - self.shist_start_time  > self. _HISTORY_SECONDS_:
            mrflog.info("exceeded history secs with %s"%repr(time.localtime(self.shist_start_time)))

            for sname in self.sgraphs.keys():
                flds = self.sgraphs[sname]
                for fld in flds:
                    del self.shist[sname][fld][0]
            del self.shist_ts[0]
            self.shist_start_time = time.mktime(time.strptime(self.shist_ts[0],"%Y-%m-%dT%H:%M:%S"))
            mrflog.info("new start_time %s",repr(time.localtime(self.shist_start_time)))


    def authenticate(self,username,password,ip,req_host):
        mrflog.info('authenticate_staff : username = '+username+" , ip : "+ip)
        if username not in install.users.keys():
            return None

        uinf =  install.users[username]

        if password != uinf['password']:
            return None


        if type(uinf['apps']) == type([]):
            apps = uinf['apps']
        elif type(uinf['apps']) == type('') and uinf['apps'] == '*':
            apps = self.all_weblets
        else:
            mrflog.error("failed to setup apps for "+username)


        mrflog.warn('authenticated ip '+ip)
        wsid = os.urandom(16).encode('hex')
        sessid = gen_sessid()

        self.comm.prepare_socket(uinf['sid'],wsid,sessid,uinf['username'],uinf['type'],apps,ip,req_host)
        self.comm.set_session_expire(wsid)

        return { 'sid' : uinf['sid'] , 'sessid' : sessid}


    def ws_url(self, wsid,req_host):
        """
        url = install.wsprot+install.host
        if install.domain:
            url += '.'+install.domain
        if self.config.has_key('http_proxy_port'):
            pport =  self.config['http_proxy_port']
        else:
            pport =  self.config['http_port']

        url += ':'+str(pport)+'/ws?Id='+wsid
        """
        url = install.wsprot+req_host+'/ws?Id='+wsid


        return url



    def web_client_command(self,wsid,app,cmd,data):
        if app not in self.weblets.keys():
            mrflog.error("web_client_command unknown app %s from wsid %s"%(app,wsid))
            return
        mrflog.warn("%s web_client_command cmd is %s data %s"%(self.__class__.__name__,cmd, repr(data)))
        self.weblets[app].cmd(cmd,data,wsid)


        self.server._run_updates()


    def graph_callback(self, data):
        tag =  { 'app' : 'auto_graph', 'tab' : 'all' , 'row' : 'all' }
        mrflog.debug("%s graph_callback  data %s "%(self.__class__.__name__,repr(data)))
        mrflog.debug("tag : "+repr(tag))
        self.webupdate(tag,data)

        self.server._run_updates()

        for slabel in data['sensors']:
            sdata = data['sensors'][slabel]
            stype = self.sensors[slabel]._stype_
            if data.has_key('ts') and sdata.has_key(stype) and self.server:
                dt = datetime.datetime.strptime(data['ts'],mrfland.DateTimeFormat)
                mrflog.debug("trying db insert sensor_id "+slabel+" stype "+stype+ " "+repr(dt)+"  data "+ repr(sdata[stype]))
                self.db_sensor_data_insert(sensor_id=slabel, stype=stype, dt = dt, value=sdata[stype])

    def graph_req(self,slab,fld):  #for weblets to request graph data capture for sensors
        if not self.sensors.has_key(slab):
            mrflog.error("%s graph_req no sensor named  %s "%(self.__class__.__name__,slab))
            return

        if not self.sgraphs.has_key(slab):
            self.sgraphs[slab] = [fld] # no need for sensor ref here
            mrflog.warn("%s graph_req added for sensor  %s  fld %s"%(self.__class__.__name__,slab,fld))
        elif not fld in self.sgraphs[slab]:
            mrflog.warn("%s graph_req appended for sensor  %s  fld %s"%(self.__class__.__name__,slab,fld))
            self.sgraphs[slab].append(fld)
    def graph_inst(self,sensors, width = "80%", height = "80%"): ## FIXME should move to weblet
        snames = []

        graphs = ""
        for stype in sensors.keys():
            for sname in sensors[stype]:
                graphs += "mrf-graphs-"+sname+" "


        s = """
     <div id="mrf-graph-%d" class="mrf-graph %s" data-sensors='%s' width="%s" height="%s"> </div>
"""%(self.graph_insts,graphs,mrfland.to_json(sensors),width,height)
        self.graph_insts += 1  # they need unique ids for plotly
        return s

    def add_timer(self,app, name, tmr):  # timers control period sensors now - controls are in app, but evaluation is here
        if tmr.period not in self.period_timers:
            mrflog.error("adding timer %s for period %s failed - as not in period_timers"%(name,tmr.period))
            return
        if name in self.period_timers[tmr.period]:
            mrflog.error("%s already registered"%name)
            return
        self.period_timers[tmr.period][name] = tmr
        self.period_lut[name] = tmr.period


    def eval_period(self,pn):
        if not pn in self.period_timers:
            mrflog.error("%s not found in period_timers"%pn)
        psn = pn + '_PERIOD'
        persens =  self.sensors[psn]
        mrflog.warn("evaluating period for sensor %s"%repr(persens))
        was_active = persens.output['active']
        is_active = False

        for tn in self.period_timers[pn]:
            tmr = self.period_timers[pn][tn]
            tn_act = tmr.is_active()
            is_active = is_active or tn_act
            mrflog.warn("%s tn is_active %s "%(tn,repr(tn_act)))

        mrflog.warn("evaluated period for sensor %s was_active %s is_active %s"%(repr(persens),repr(was_active),repr(is_active)))

        if was_active != is_active:
            mrflog.warn("sensor changing to %s"%repr(is_active))
            inp = {}
            inp['active'] = int(is_active)
            td = PktTimeDate()
            td.set(datetime.datetime.now())
            inp['date']   = td
            persens.input(inp)

    def timer_changed(self,app,name, act=None):
        if not name in self.period_lut:
            mrflog.error("%s not found in period_lut"%name)
            return
        pn = self.period_lut[name]
        mrflog.warn("timer_changed %s for period %s"%(name,pn))
        tmr = self.period_timers[pn][name]
        mrflog.warn(repr(tmr))
        self.eval_period(pn)

        if tmr.pulse and act and act == 'off': # clear one shots when they  go inactive
            mrflog.warn("trying to clear pulse timer "+tmr.name)
            onv = tmr.__dict__['on']
            offv = tmr.__dict__['off']
            env = tmr.__dict__['en']

            onv.set("00:00")
            offv.set("00:00")
            env.set(False)

        if tmr.en.val :   # make sure timers are set
            for act in ['on','off']:
                aval = tmr.__dict__[act]
                self.set_timer( aval.tod , app, name , act)


    def set_timer(self, tod, app, tag, act):
        self.server.set_timer(tod,app,tag,act)

    def tbd_timer_reg_callback(self, tag, act, callback):
        tid = tag+"."+ act
        if not self.timers.has_key(tid):
            mrflog.error("timer_reg_callback no tid %s"%tid)
            return
        self.timers[tid].append(callback)
        mrflog.error("%s registering callback for  tid %s ( total cbs %d )"%(self.__class__.__name__,tid,len(self.timers[tid])))

    def timer_callback(self, app, tag , act):
        mrflog.warn("RegManager timer_callback, app %s tag %s act %s",app, tag,act)

        if tag in self.period_lut:
            return self.timer_changed(app,tag,act=act)  # calling this will do

        if not self.weblets.has_key(app):
            mrflog.error("%s timer_callback no app  %s "%(self.__class__.__name__,app, tid,len(self.timers[tid])))
            return
        self.weblets[app]._timer_callback(tag, act)

    def senslookup(self,label):
        if self.sensmap.has_key(label):
            return self.sensmap[label]

    def senslink(self, label, addr, chan):
        if self.sensmap.has_key(label):
            mrflog.error("regman.senslink - already have tag %s"%label)
            return
        self.sensmap[label] = { 'addr' : addr, 'chan' : chan }

    def sens_search(self,label):
        if self.sensors.has_key(label):
            return self.sensors[label]
        else:
            return None

    def sens_search_vector(self,stype,label):
        svtmp = {}
        sv = OrderedDict()
        reh =  re.compile(r'%s_([0-9]+)'%label)
        if self.senstypes.has_key(stype):
            sl = self.senstypes[stype]
        else:
            return None

        levels = []
        for s in sl:
            mob = reh.match(s.label)
            if mob:
                level = int(mob.group(1))
                levels.append(level)
                svtmp[level] = s

        levels.sort(reverse=True)

        for l in levels:
            sv[l] = svtmp[l]
        return sv

    def sens_search_vector_max(self,stype,label):
        sv = self.sens_search_vector(stype,label)
        if sv == None or len(sv) == 0:
            return None
        return sv[sv.keys()[0]]

    def webupdate(self, tag , data , wsid=None):
        if wsid:
            update = { 'tag': tag , 'data': data , 'wsid': wsid}
            #mrflog.warn("webupdate %s"%repr(update))
            self.wups.append(update)

        else:
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


    def ndr(self,hdr,ndr):

        mrflog.warn("got NDR %s"%repr(ndr))
        mrflog.warn("header was  %s"%repr(hdr))
        return None


    def packet(self,hdr,resp):

        if self.devmap.has_key(hdr.usrc):
            return self.devmap[hdr.usrc].packet(hdr,resp)
        else:
            mrflog.warn("got packet but no dev registered for usrc %d"%hdr.usrc)
            return None, None

    def weblet_register(self, weblet):
        if self.weblets.has_key(weblet.tag):
            mrflog.error("weblet_register - key error %s"%weblet.tag)
            return

        self.weblets[weblet.tag] = weblet
        self.all_weblets.append(weblet.tag)
        mrflog.warn("weblet_register -registered new weblet %s"%weblet.tag)
        self.db_app_load_cfg_data(apptag=weblet.tag)
    def html_pills(self,apps):
        """ generate bootstrap pills """
        s = ""
        first = True
        for wa in self.weblets.keys():
            if wa in apps:
                wl = self.weblets[wa]
                if first:
                    lic = ' class="active"'
                    first = False
                else:
                    lic = ''

                s += '    <li %s><a data-toggle="pill" href="#%s">%s</a></li>\n'%(lic,wl.tag,wl.label)
        return s

    def html_body(self,apps,static_cdn=False):
        """ generate the html body """
        self.graph_insts = 0

        css_html = self.tp_static_mgr.html(css=True,static_cdn=static_cdn)
        js_html  = self.tp_static_mgr.html(static_cdn=static_cdn)
        s = """
  <body>
    <!-- Thirdparty CSS
    ================================================== -->
""" + css_html+"""

    <div class="container">
       <ul class="nav nav-pills" id="mrf-tabs">
"""+self.html_pills(apps)+"""
       </ul>

       <div class="tab-content">
"""
        active = True  # first in list is active bootstrap pane
        for wa in self.weblets.keys():
            if wa in apps:
                wl = self.weblets[wa]
                s += wl.html(active)
                active = False
        s += """
       </div> <!-- /tab-content -->

    </div> <!-- /container -->

    <!-- Sensor graph data  -->
    <script type="text/javascript">
"""+self.sensor_average_js()+"""
    </script>
    <!-- Thirdparty JavaScript
    ================================================== -->
""" + js_html+"""


    <script src="static/secure/js/mrf_sock.js"></script>


  </body>
"""
        return s
    def device_register(self, dev):
        """ register new MrfDevice"""
        if self.devices.has_key(dev.label):
            mrflog.error("%s device_register duplicate device label %s"%dev.label)
            return
        self.devices[dev.label] = dev

        self.devmap[dev.address] = dev
        ### now enumerate device sensors
        for cap in dev.caps.keys():
            mrflog.warn("enumerating sensors type %s"%cap)
            for ch in range(len(dev.caps[cap])):
                sens = dev.caps[cap][ch]
                self.add_sensor(sens,cap)

                mrflog.warn("chan %d type  %s"%(ch,type(sens)))
    def add_sensor(self, sens,cap):
        if self.sensors.has_key(sens.label):
            mrflog.error("%s device_register duplicate sensor label %s"%sens.label)
            return
        self.sensors[sens.label] = sens
        if not self.senstypes.has_key(type(sens)):
            self.senstypes[type(sens)] = []
        self.senstypes[type(sens)].append(sens)

        if not self.senscaps.has_key(cap):
            self.senscaps[cap] = []
        self.senscaps[cap].append(sens)

    def subprocess(self, arglist, callback):
        mrflog.warn("RegManager subprocess call : arglist %s "%repr(arglist))
        return self.server.subprocess(arglist, callback)

    def quiet_task(self):  # server calls this task periodically
        mrflog.debug("quiet task")
        for wapn in self.weblets.keys():
            wap = self.weblets[wapn]
            if wap._cfg_touched:
                mrflog.warn("wap %s cfg change"%wapn)
                doc = wap.var.cfg_doc()
                wap._cfg_touched = False
                doc['hostname'] = self.hostname
                doc['tag'] = wap.tag
                doc['label'] = wap.label
                doc['cls'] = wap.__class__.__name__

                mrflog.warn(repr(doc))
                self.db_app_cfg_data_replace(apptag=wapn,doc=doc)

    def sensor_average_js(self):  # generate sensor average history js
        s = ""

        if len(self.sgraphs.keys()) == 0:
            return s

        s += """
var _sensor_hist_seconds = %d;"""%self._HISTORY_SECONDS_
        s += """
var _sensor_ts = %s ;"""%mrfland.to_json(self.shist_ts)
        s += """
var _sensor_averages = {"""

        for slab in self.sgraphs.keys():

            flds = self.sgraphs[slab]

            sens = self.sensors[slab]
            s += """
     %s : {\n"""%(slab)
            for hfld in flds:
                s+= """
         %s :  {"""%hfld
                s+= """
                ts : _sensor_ts,
                %s : %s """%('value',mrfland.to_json(self.shist[slab][hfld]))
                s+= """
               }
           },"""
        s += """
         }"""

        return s




    """
    @tornado.gen.coroutine
    def db_period_graph(self,**kwargs):

        sensor_ids = kwargs['sensor_ids']
        stype = kwargs['stype']
        dt_start = kwargs['date_start']
        dt_end = kwargs['date_end']

        wtag = kwargs['wtag']
        wsid = kwargs['wsid']



        start_date = dt_start.date()
        end_date = dt_end.date()



        docdate = datetime.datetime.combine(start_date,datetime.time())

        gdata = {}

        for sensor_id in sensor_ids:

            coll = self.db.get_collection('sensor.%s.%s'%(stype,sensor_id))
            doc = yield coll.find_one({'docdate' : docdate})
            stype = doc['stype']

            mrflog.warn("got doc for %s  %s"%(sensor_id,repr(doc)))
            gdata = self.graph_day_data(gdata,sensor_id,stype,doc)
            mrflog.warn("gdata %s %s"%(sensor_id,repr(gdata)))

        mrflog.warn("gdata %s"%repr(gdata))

        self.webupdate(wtag,
                       gdata,
                       wsid)

        self.server._run_updates()

    """


    def graph_day_data(self,gdata,sensor_id, stype, doc):
        """ unwinds 2 day hour/min array data and breaks when un-initialised value found"""

        if not gdata.has_key(sensor_id):
            gdata[sensor_id] = {}




        if not gdata[sensor_id].has_key(stype):
            gdata[sensor_id][stype] = {
                'ts'   : [],
                'value': []
            }
        #mrflog.warn("graph_day_data - stype is %s starting with gdata %s"%(stype,repr(gdata)))
        gvals =  gdata[sensor_id][stype]
        gtime = doc['docdate']

        for hour in xrange(24):
            for minute in xrange(60):
                if not (doc['data'][hour][minute] == doc['nullval']):
                    gvals['ts'].append(gtime.strftime(mrfland.DateTimeFormat))
                    gvals['value'].append(doc['data'][hour][minute])
                #else:
                #    print "got nullval hour %d min %d val %s"%(hour,minute,repr(doc['data'][hour][minute]))

                gtime = gtime + datetime.timedelta(minutes=1)
        #mrflog.warn("graph_day_data - finish with gdata %s"%(repr(gdata)))

        return gdata


    @tornado.gen.coroutine
    def db_period_graph(self,**kwargs):

        sensor_ids = kwargs['sensor_ids']
        stype = kwargs['stype']
        dt_start = kwargs['date_start']
        dt_end = kwargs['date_end']

        wtag = kwargs['wtag']
        wsid = kwargs['wsid']



        start_date = dt_start.date()
        end_date = dt_end.date()



        docdate = datetime.datetime.combine(start_date,datetime.time())

        gdata = {}

        for sensor_id in sensor_ids:

            coll = self.db.get_collection('sensor.%s.%s'%(stype,sensor_id))
            doc = yield coll.find_one({'docdate' : docdate})

            mrflog.warn("got doc for %s  %s"%(sensor_id,repr(doc)))
            stype = doc['stype']
            gdata  = self.graph_day_data(gdata,sensor_id,stype,doc)
            mrflog.warn("gdata %s %s"%(sensor_id,repr(gdata)))

        mrflog.warn("gdata %s"%repr(gdata))

        self.webupdate(wtag,
                       gdata,
                       wsid)

        self.server._run_updates()




    @tornado.gen.coroutine
    def db_days_graph(self,**kwargs):

        sensor_ids = kwargs['sensor_ids']
        stype = kwargs['stype']
        dt = kwargs['docdate']
        wtag = kwargs['wtag']
        wsid = kwargs['wsid']

        if kwargs.has_key('days'):
            days = kwargs['days']
        else:
            days = 1

        startdate = datetime.datetime.combine(dt.date(),datetime.time())

        gdata = {}

        for sensor_id in sensor_ids:
            docdate = startdate - datetime.timedelta(days=days-1)
            coll = self.db.get_collection('sensor.%s.%s'%(stype,sensor_id))

            for day in xrange(days):
                mrflog.warn("%s %d) %s"%(sensor_id,day,repr(docdate)))
                doc = yield coll.find_one({'docdate' : docdate})



                try:
                    stype = doc['stype']

                    mrflog.warn("got doc %s  %s"%(repr(docdate),sensor_id))

                    gdata  = self.graph_day_data(gdata, sensor_id,stype, doc)
                    #mrflog.warn("gdata %s %s"%(sensor_id,repr(gdata)))
                except:
                    mrflog.warn("no doc for %s %s"%(sensor_id,repr(docdate)))

                docdate += datetime.timedelta(days=1)

        #mrflog.warn("gdata %s"%repr(gdata))

        self.webupdate(wtag,
                       gdata,
                       wsid)

        self.server._run_updates()

    @tornado.gen.coroutine
    def db_get_sensors(self,**kwargs):


        cnames = yield self.db.collection_names()


        mrflog.warn("got cnames %s"%repr(cnames))

        sensors = dict()

        for cn in cnames:
            fld = cn.split('.')
            if (fld[0] == 'sensor') and (len(fld) == 3):
                stype = fld[1]
                sname = fld[2]
                if not sensors.has_key(stype):
                    sensors[stype] = []
                sensors[stype].append(sname)

        for stype in sensors.keys():
            sensors[stype].sort()

        mrflog.warn("got sensors %s"%repr(sensors))
        self.db_sensors = sensors

    @tornado.gen.coroutine
    def db_sensor_data_insert(self, **kwargs):

        if not hasattr(self,'db'):
            return

        mrflog.info("sensor_db_insert : kwargs %s"%repr(kwargs))
        sensor_id = kwargs['sensor_id']
        stype = kwargs['stype']
        dt = kwargs['dt']
        minute = dt.minute
        hour = dt.hour
        value  = kwargs['value']


        #docdate should always have zero time ..let's do this now
        docdate = datetime.datetime.combine(dt.date(),datetime.time())


        coll = self.db.get_collection('sensor.%s.%s'%(stype,sensor_id))

        future = coll.update({'docdate':docdate},
                                 { "$set" : { 'data.'+str(hour)+'.'+str(minute) : value } })
        result = yield future


        if result['updatedExisting']:
            mrflog.info("doc update result good %s "%repr(result))
        else:
            mrflog.warn("result dodgy %s "%repr(result))
            doc = mrfland.new_sensor_day_doc(sensor_id, stype, docdate)
            future = coll.insert_one(doc)
            result = yield future
            mrflog.warn("insert result was %s"%repr(result))

            future = coll.update({'docdate':docdate},
                                 { "$set" : { 'data.'+str(hour)+'.'+str(minute) : value } })

            result = yield future
            if result['updatedExisting']:
                mrflog.warn("doc update(2) result good %s "%repr(result))
            else:
                mrflog.error("result(2) bad %s "%repr(result))


    @tornado.gen.coroutine
    def db_app_cfg_data_replace(self, **kwargs):

        if not hasattr(self,'db'):
            return

        mrflog.warn("db_app_cfg_data_replace : kwargs %s"%repr(kwargs))
        apptag = kwargs['apptag']
        doc    = kwargs['doc']

        if kwargs.has_key('sid'):
            sid = kwargs['sid']
        else:
            sid = -1

        doc['sid'] = sid



        docdate = datetime.datetime.now()
        doc['docdate'] = docdate

        collname =  '%s.cfg.webapps'%(self.hostname)
        coll = self.db.get_collection(collname)

        filt = {'tag' : apptag }



        future = coll.replace_one(filt, doc, True)
        result = yield future

        mrflog.warn("result %s"%repr(result))


    @tornado.gen.coroutine
    def db_app_load_cfg_data(self, **kwargs):

        if not hasattr(self,'db'):
            return

        mrflog.warn("db_app_load_cfg_data : kwargs %s"%repr(kwargs))
        apptag = kwargs['apptag']





        collname =  '%s.cfg.webapps'%(self.hostname)
        coll = self.db.get_collection(collname)

        filt = {'tag' : apptag }



        future = coll.find_one(filt)
        result = yield future

        mrflog.warn("loaded config  for app %s result %s"%(apptag,repr(result)))

        if result.has_key('data'):
            self.weblets[apptag].restore_cfg(result['data'])

def gen_sessid():
    return base64.b64encode(os.urandom(18))
