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
#import asa_prims
#import asa
import psycopg2
from datetime import datetime
import time
import json
from mrflog import mrf_log
from mainapp import mainapp
from public import publicapp
#from asa_pcomm import asa_pcomm
import hashlib

clients = dict()

alog = mrf_log()
#aconn = asa.Connection()
#dbc = aconn.cursor()

#pcomm = asa_pcomm(aconn,alog)

#asa.set_pcomm(pcomm)

class PubSocketHandler(tornado.websocket.WebSocketHandler):
    def open(self, *args):        
        self.id = self.get_argument("Id")
        self.key = self.get_argument("Key",default = None)
        alog.info("client attempt to open ws:Id="+self.id+" Key="+str(self.key))
        #alog.info("here comes websockhandler self")
        #print str(dir(self))
        #check id has been issued
        alog.info("headers:"+str(self.request.headers))
        allow = True
        opened = datetime.now()
        rs =  self._request_summary()
        alog.debug("req_sum:"+rs)
        regx = r'^GET /pws\?Id=%s \(([^\(]+)\)'%self.id
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
        if self.request.headers.has_key('X-Forwarded-For'):
            ip = self.request.headers['X-Forwarded-For']
            alog.info("real ip="+ip)
        
 
        if self.key:
            mh = hashlib.sha1() 
            asakey = '012sf32Sxx8z'
            mh.update(ip+asakey)
            hk = mh.hexdigest()
            alog.info("hk="+hk)
            if hk == self.key:
                allow = True
                res = asa.query_obj(dbc,"select ip,cookie,open from pub_socket where id = %s",(self.id,))   
                if res:
                    dbc.execute("delete from pub_socket * where id = %s",(self.id,))
                qstr = 'insert into pub_socket(id,ip,cookie) values(%s,%s,%s)'
                dbc.execute(qstr,(self.id,ip,'NOCOOKIEPUBSOCK-'+self.id))

            else:
                allow = False
        else:
            res = asa.query_obj(dbc,"select ip,cookie,open from pub_socket where id = %s",(self.id,))        
            if ( res  == None):
                alog.error("PubSocketHandler.open - no socket allocated for this ws url:"+self.id)
                return
            alog.info("pub_socket opened : "+str(res))
            if  res['open'] :
                alog.error("PubSocketHandler.open -  socket  for this ws url:"+self.id+" already opened")
                return           

        dbc.execute("update pub_socket set open = %s , opened = %s , headers = %s where id = %s",
                        (True,opened,str(self.request.headers),self.id))

        cdata = {"id": self.id,
                 "ip":ip,
                 "opened":opened,
                 "cid" : asa.anon_webchat_cid(aconn),
                 "object": self 
                 }
        alog.info("cdata:"+str(cdata))
        self.stream.set_nodelay(True)
        alog.info("adding client : "+self.id)

        pcomm.add_client(self.id,cdata)
        alog.info("PubSocketHandler  ws.open:Id="+self.id +" (%d clients)"%len(pcomm.clients.keys()))

        asa.lookup_psock_ip(aconn,self.id,ip)
        
        ro = asa.RetObj()
        ro.b(asa.chat_status(aconn))
        asa.comm.comm(None,ro)

        ro = asa.RetObj()
        ro.b(asa.chat_service_status(aconn))
        pcomm.comm(None,ro)

        
    
    def on_message(self, message):        
        """
        when we receive some message we want some message handler..
        for this example i will just print message to console
        """
        
        if pcomm.clients.has_key(self.id):
            alog.info("psock %s sent a message : %s" % (self.id, message))
            #msgob = asa_prims.json_parse(message)
            if msgob and hasattr(msgob,'has_key') and msgob.has_key('ccmd'):
                alog.info("recieved client cmd %s" % (msgob['ccmd']))
                action_pcommand(self.id,msgob)
                #action_client_cmd(self.id,username,msgob)
            else:
                alog.info("failed to parse json : got"+str(msgob))
                
                
        else:
            alog.info("ws unauthorised ID %s attempted to send %s" % (self.id, message))
            self.close()
 

        #clients[self.id]['object'].write_message(msg)


        #clients[self.id]['object'].write_message('Thanks for your msg : '+ message )

    def on_close(self):
        alog.info("on_close id :"+self.id)
        #alog.info("close_reason:"+str(self.close_reason))
        #alog.info("close_code:"+str(self.close_code))      
        pcomm.del_client(self.id)
        # should close any pchat_session used by this socket
        ss = asa.query_obj_list(dbc,"select id,sid,ssid from pchat_session where psid = %s for update",(self.id,))
        reason = "client closed browser window"
        alog.info("chatsess using socket:"+str(ss))
        for ses in ss:
            asa.client_closed_chat(aconn,ses['id'],reason)
        dbc.execute("delete from pub_socket * where id = %s",
                    (self.id,))   
        aconn.commit()       
        ro = asa.RetObj()
        ro.b(asa.chat_status(aconn))
        asa.comm.comm(None,ro)

        
        
def nobbit():
    print "nobbit"
    return "nobbit"

pcmds = {
    'request-chat': nobbit,
    'chat-ipmsg': nobbit
}

def action_pcommand(id,msgob):
    if not msgob.has_key('ccmd'):
        errmsg = "psocket id "+id+" sent invalid packet:"+str(msgob)
        alog.error(errmsg)
        return
    cmd = msgob['ccmd']
    if pcmds.has_key(cmd):
        alog.info(cmd+":evaluating action")
        try:
            ro = pcmds[cmd](aconn,id,msgob['data'])
        except Exception,e: 
            import traceback
            details = traceback.format_exc()

            errmsg = "despatcher exception : psocket id "+id+"  obj :"+str(msgob)+", exception:"+str(e)+"\n+Details:\n"+details
            alog.error(errmsg)
            return
    else:
        errmsg = "socket id "+id+"  sent unrecognised command :"+str(cmd)
        alog.error(errmsg)
        return
      
    return